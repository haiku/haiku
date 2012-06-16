/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "Inode.h"
#include "NFS4Server.h"
#include "Request.h"


NFS4Server::NFS4Server(RPC::Server* serv)
	:
	fThreadCancel(true),
	fLeaseTime(0),
	fCIDUseCount(0),
	fOpenFiles(NULL),
	fServer(serv)
{
	mutex_init(&fLock, NULL);
	mutex_init(&fOpenLock, NULL);
}


NFS4Server::~NFS4Server()
{
	fThreadCancel = true;
	fCIDUseCount = 0;
	interrupt_thread(fThread);
	status_t result;
	wait_for_thread(fThread, &result);

	mutex_destroy(&fLock);
	mutex_destroy(&fOpenLock);
}


uint64
NFS4Server::ServerRebooted(uint64 clientId)
{
	if (clientId != fClientId)
		return fClientId;

	fClientId = ClientId(clientId, true);

	// reclaim all open files
	mutex_lock(&fOpenLock);
	OpenFileCookie* current = fOpenFiles;
	while (current != NULL) {
		_ReclaimOpen(current);
		current = current->fNext;
	}
	mutex_unlock(&fOpenLock);

	return fClientId;
}


status_t
NFS4Server::_ReclaimOpen(OpenFileCookie* cookie)
{
	if (cookie->fClientId == fClientId)
		return B_OK;
	
	cookie->fClientId = fClientId;

	Request request(fServer);
	RequestBuilder& req = request.Builder();

	req.PutFH(cookie->fHandle);
	req.Open(CLAIM_PREVIOUS, cookie->fSequence++, OPEN4_SHARE_ACCESS_READ,
		cookie->fClientId, OPEN4_NOCREATE, cookie->fOwnerId, NULL);

	status_t result = request.Send();;
	if (result != B_OK)
		return result;

	ReplyInterpreter& reply = request.Reply();

	result = reply.PutFH();
	if (result != B_OK)
		return result;

	bool confirm;
	result = reply.Open(cookie->fStateId, &cookie->fStateSeq, &confirm);
	if (result != B_OK)
		return result;

	if (confirm) {
		request.Reset();

		req.PutFH(cookie->fHandle);
		req.OpenConfirm(cookie->fSequence++, cookie->fStateId,
			cookie->fStateSeq);

		result = request.Send();
		if (result != B_OK)
			return result;

		result = reply.PutFH();
		if (result != B_OK)
			return result;

		result = reply.OpenConfirm(&cookie->fStateSeq);
		if (result != B_OK)
			return result;
	}

	return B_OK;
}


void
NFS4Server::AddOpenFile(OpenFileCookie* cookie)
{
	mutex_lock(&fOpenLock);
	cookie->fPrev = NULL;
	cookie->fNext = fOpenFiles;
	if (fOpenFiles != NULL)
		fOpenFiles->fPrev = cookie;
	fOpenFiles = cookie;
	mutex_unlock(&fOpenLock);
}


void
NFS4Server::RemoveOpenFile(OpenFileCookie* cookie)
{
	mutex_lock(&fOpenLock);
	if (cookie == fOpenFiles)
		fOpenFiles = cookie->fNext;

	if (cookie->fNext)
		cookie->fNext->fPrev = cookie->fPrev;
	if (cookie->fPrev)
		cookie->fPrev->fNext = cookie->fNext;
	mutex_unlock(&fOpenLock);
}


uint64
NFS4Server::ClientId(uint64 prevId, bool forceNew)
{
	mutex_lock(&fLock);
	if ((forceNew && fClientId == prevId) || fCIDUseCount == 0) {
		Request request(fServer);
		request.Builder().SetClientID(fServer);

		status_t result = request.Send();
		if (result != B_OK)
			goto out_unlock;

		uint64 ver;
		result = request.Reply().SetClientID(&fClientId, &ver);
		if (result != B_OK)
			goto out_unlock;

		request.Reset();
		request.Builder().SetClientIDConfirm(fClientId, ver);

		result = request.Send();
		if (result != B_OK)
			goto out_unlock;

		result = request.Reply().SetClientIDConfirm();
		if (result != B_OK)
			goto out_unlock;

		_StartRenewing();
	}

	fCIDUseCount++;

out_unlock:
	mutex_unlock(&fLock);
	return fClientId;
}


void
NFS4Server::ReleaseCID(uint64 cid)
{
	mutex_lock(&fLock);
	fCIDUseCount--;
	mutex_unlock(&fLock);
}


status_t
NFS4Server::_GetLeaseTime()
{
	Request request(fServer);
	request.Builder().PutRootFH();
	Attribute attr[] = { FATTR4_LEASE_TIME };
	request.Builder().GetAttr(attr, sizeof(attr) / sizeof(Attribute));

	status_t result = request.Send();
	if (result != B_OK)
		return result;

	ReplyInterpreter& reply = request.Reply();

	result = reply.PutRootFH();
	if (result != B_OK)
		return result;

	AttrValue* values;
	uint32 count;
	result = reply.GetAttr(&values, &count);
	if (result != B_OK)
		return result;

	// FATTR4_LEASE_TIM is mandatory
	if (count < 1 || values[0].fAttribute != FATTR4_LEASE_TIME) {
		delete[] values;
		return B_BAD_VALUE;
	}

	fLeaseTime = values[0].fData.fValue32 * 1000000;

	return B_OK;
}


status_t
NFS4Server::_StartRenewing()
{
	if (!fThreadCancel)
		return B_OK;


	if (fLeaseTime == 0) {
		status_t result = _GetLeaseTime();
		if (result != B_OK)
			return result;
	}

	fThreadCancel = false;
	fThread = spawn_kernel_thread(&NFS4Server::_RenewalThreadStart,
									"NFSv4 Renewal", B_NORMAL_PRIORITY, this);
	if (fThread < B_OK)
		return fThread;

	status_t result = resume_thread(fThread);
	if (result != B_OK) {
		kill_thread(fThread);
		return result;
	}

	return B_OK;
}


status_t
NFS4Server::_Renewal()
{
	while (!fThreadCancel) {
		// TODO: operations like OPEN, READ, CLOSE, etc also renew leases
		snooze_etc(fLeaseTime - 2, B_SYSTEM_TIMEBASE, B_RELATIVE_TIMEOUT
			| B_CAN_INTERRUPT);
		mutex_lock(&fLock);

		uint64 clientId = fClientId;

		if (fCIDUseCount == 0) {
			fThreadCancel = true;
			mutex_unlock(&fLock);
			return B_OK;
		}

		Request request(fServer);
		request.Builder().Renew(fClientId);
		request.Send();
		mutex_unlock(&fLock);

		if (request.Reply().NFS4Error() == NFS4ERR_STALE_CLIENTID)
			ServerRebooted(clientId);
	}

	return B_OK;
}


status_t
NFS4Server::_RenewalThreadStart(void* ptr)
{
	NFS4Server* server = reinterpret_cast<NFS4Server*>(ptr);
	return server->_Renewal();
}

