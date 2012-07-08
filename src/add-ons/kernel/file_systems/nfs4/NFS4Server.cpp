/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "Filesystem.h"
#include "Inode.h"
#include "NFS4Server.h"
#include "Request.h"


NFS4Server::NFS4Server(RPC::Server* serv)
	:
	fThreadCancel(true),
	fWaitCancel(create_sem(1, NULL)),
	fLeaseTime(0),
	fClientIdLastUse(0),
	fUseCount(0),
	fFilesystems(NULL),
	fServer(serv)
{
	mutex_init(&fClientIdLock, NULL);
	mutex_init(&fFSLock, NULL);
}


NFS4Server::~NFS4Server()
{
	fThreadCancel = true;
	fUseCount = 0;
	release_sem(fWaitCancel);
	status_t result;
	wait_for_thread(fThread, &result);

	delete_sem(fWaitCancel);
	mutex_destroy(&fClientIdLock);
	mutex_destroy(&fFSLock);
}


uint64
NFS4Server::ServerRebooted(uint64 clientId)
{
	if (clientId != fClientId)
		return fClientId;

	fClientId = ClientId(clientId, true);

	// reclaim all opened files and held locks from all filesystems
	MutexLocker _(fFSLock);
	Filesystem* fs = fFilesystems;
	while (fs != NULL) {
		OpenFileCookie* current = fs->OpenFilesLock();
		while (current != NULL) {
			_ReclaimOpen(current);
			_ReclaimLocks(current);
			current = current->fNext;
		}
		fs->OpenFilesUnlock();
		fs = fs->fNext;
	}

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

	req.PutFH(cookie->fInfo.fHandle);
	req.Open(CLAIM_PREVIOUS, cookie->fSequence++, sModeToAccess(cookie->fMode),
		cookie->fClientId, OPEN4_NOCREATE, cookie->fOwnerId, NULL);

	status_t result = request.Send();
	if (result != B_OK)
		return result;

	ReplyInterpreter& reply = request.Reply();

	reply.PutFH();

	bool confirm;
	result = reply.Open(cookie->fStateId, &cookie->fStateSeq, &confirm);
	if (result != B_OK)
		return result;

	if (confirm) {
		request.Reset();

		req.PutFH(cookie->fInfo.fHandle);
		req.OpenConfirm(cookie->fSequence++, cookie->fStateId,
			cookie->fStateSeq);

		result = request.Send();
		if (result != B_OK)
			return result;

		reply.PutFH();
		result = reply.OpenConfirm(&cookie->fStateSeq);
		if (result != B_OK)
			return result;
	}

	return B_OK;
}


status_t
NFS4Server::_ReclaimLocks(OpenFileCookie* cookie)
{
	MutexLocker _(cookie->fLocksLock);
	LockInfo* linfo = cookie->fLocks;
	while (linfo != NULL) {
		MutexLocker locker(linfo->fOwner->fLock);
		if (linfo->fOwner->fClientId != fClientId) {
			memset(linfo->fOwner->fStateId, 0, sizeof(linfo->fOwner->fStateId));
			linfo->fOwner->fClientId = fClientId;
		}

		do {
			Request request(fServer);
			RequestBuilder& req = request.Builder();

			req.PutFH(cookie->fInfo.fHandle);
			req.Lock(cookie, linfo, true);

			status_t result = request.Send();
			if (result != B_OK)
				break;

			ReplyInterpreter &reply = request.Reply();

			reply.PutFH();
			reply.Lock(linfo);

			break;
		} while (true);
		locker.Unlock();

		linfo = linfo->fNext;
	}

	return B_OK;
}


void
NFS4Server::AddFilesystem(Filesystem* fs)
{
	MutexLocker _(fFSLock);
	fs->fPrev = NULL;
	fs->fNext = fFilesystems;
	if (fFilesystems != NULL)
		fFilesystems->fPrev = fs;
	fFilesystems = fs;
	fUseCount += fs->OpenFilesCount();
	if (fs->OpenFilesCount() > 0 && fThreadCancel)
		_StartRenewing();
}


void
NFS4Server::RemoveFilesystem(Filesystem* fs)
{
	MutexLocker _(fFSLock);
	if (fs == fFilesystems)
		fFilesystems = fs->fNext;

	if (fs->fNext)
		fs->fNext->fPrev = fs->fPrev;
	if (fs->fPrev)
		fs->fPrev->fNext = fs->fNext;
	fUseCount -= fs->OpenFilesCount();
}


uint64
NFS4Server::ClientId(uint64 prevId, bool forceNew)
{
	MutexLocker _(fClientIdLock);
	if (fUseCount == 0 && fClientIdLastUse + (time_t)LeaseTime() < time(NULL)
		|| forceNew && fClientId == prevId) {

		Request request(fServer);
		request.Builder().SetClientID(fServer);

		status_t result = request.Send();
		if (result != B_OK)
			return fClientId;

		uint64 ver;
		result = request.Reply().SetClientID(&fClientId, &ver);
		if (result != B_OK)
			return fClientId;

		request.Reset();
		request.Builder().SetClientIDConfirm(fClientId, ver);

		result = request.Send();
		if (result != B_OK)
			return fClientId;

		result = request.Reply().SetClientIDConfirm();
		if (result != B_OK)
			return fClientId;
	}

	fClientIdLastUse = time(NULL);
	return fClientId;
}


status_t
NFS4Server::FilesystemMigrated()
{
	// reclaim all opened files and held locks from all filesystems
	MutexLocker _(fFSLock);
	Filesystem* fs = fFilesystems;
	while (fs != NULL) {
		fs->Migrate(fServer);
		fs = fs->fNext;
	}

	return B_OK;
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

	reply.PutRootFH();

	AttrValue* values;
	uint32 count;
	result = reply.GetAttr(&values, &count);
	if (result != B_OK)
		return result;

	// FATTR4_LEASE_TIME is mandatory
	if (count < 1 || values[0].fAttribute != FATTR4_LEASE_TIME) {
		delete[] values;
		return B_BAD_VALUE;
	}

	fLeaseTime = values[0].fData.fValue32;

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
		status_t result = acquire_sem_etc(fWaitCancel, 1,
			B_RELATIVE_TIMEOUT, sSecToBigTime(fLeaseTime - 2));
		if (result != B_TIMED_OUT) {
			release_sem(fWaitCancel);
			return B_OK;
		}

		uint64 clientId = fClientId;

		if (fUseCount == 0) {
			MutexLocker _(fFSLock);
			if (fUseCount == 0) {
				fThreadCancel = true;
				return B_OK;
			}
		}

		Request request(fServer);
		request.Builder().Renew(fClientId);
		request.Send();

		switch (request.Reply().NFS4Error()) {
			case NFS4ERR_STALE_CLIENTID:
				ServerRebooted(clientId);
				break;
			case NFS4ERR_LEASE_MOVED:
				FilesystemMigrated();
				break;
		}
	}

	return B_OK;
}


status_t
NFS4Server::_RenewalThreadStart(void* ptr)
{
	NFS4Server* server = reinterpret_cast<NFS4Server*>(ptr);
	return server->_Renewal();
}

