/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "OpenState.h"

#include <util/AutoLock.h>

#include "FileSystem.h"
#include "Request.h"


OpenState::OpenState()
	:
	fOpened(false),
	fDelegation(NULL),
	fLocks(NULL),
	fLockOwners(NULL)
{
	mutex_init(&fLock, NULL);

	mutex_init(&fLocksLock, NULL);
	mutex_init(&fOwnerLock, NULL);
}


OpenState::~OpenState()
{
	Close();
	mutex_destroy(&fLock);

	mutex_destroy(&fLocksLock);
	mutex_destroy(&fOwnerLock);
}


LockOwner*
OpenState::GetLockOwner(uint32 owner)
{
	LockOwner* current = fLockOwners;
	while (current != NULL) {
		if (current->fOwner == owner)
			return current;

		current = current->fNext;
	}

	current = new LockOwner(owner);
	if (current == NULL)
		return NULL;

	current->fNext = fLockOwners;
	if (fLockOwners != NULL)
		fLockOwners->fPrev = current;
	fLockOwners = current;

	return current;
}


// Caller must hold fLocksLock
void
OpenState::AddLock(LockInfo* lock)
{
	lock->fNext = fLocks;
	fLocks = lock;
}


// Caller must hold fLocksLock
void
OpenState::RemoveLock(LockInfo* lock, LockInfo* prev)
{
	if (prev != NULL)
		prev->fNext = lock->fNext;
	else
		fLocks = lock->fNext;
}


void
OpenState::DeleteLock(LockInfo* lock)
{
	MutexLocker _(fOwnerLock);

	LockOwner* owner = lock->fOwner;
	delete lock;

	if (owner->fUseCount == 0) {
		if (owner->fPrev)
			owner->fPrev->fNext = owner->fNext;
		else
			fLockOwners = owner->fNext;
		if (owner->fNext)
			owner->fNext->fPrev = owner->fPrev;

		_ReleaseLockOwner(owner);
		delete owner;
	}
}


status_t
OpenState::_ReleaseLockOwner(LockOwner* owner)
{
	do {
		RPC::Server* server = fFileSystem->Server();
		Request request(server);
		RequestBuilder& req = request.Builder();

		req.ReleaseLockOwner(this, owner);

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter &reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), server))
			continue;

		return reply.ReleaseLockOwner();
	} while (true);
}


status_t
OpenState::Reclaim(uint64 newClientID)
{
	if (!fOpened)
		return B_OK;

	MutexLocker _(fLock);

	if (fClientID == newClientID)
		return B_OK;
	fClientID = newClientID;

	_ReclaimOpen(newClientID);
	_ReclaimLocks(newClientID);
	return B_OK;
}


status_t
OpenState::_ReclaimOpen(uint64 newClientID)
{
	bool confirm;
	OpenDelegationData delegation;

	uint32 sequence = fFileSystem->OpenOwnerSequenceLock();
	do {
		RPC::Server* server = fFileSystem->Server();
		Request request(server);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.Open(CLAIM_PREVIOUS, sequence, sModeToAccess(fMode), newClientID,
			OPEN4_NOCREATE, fFileSystem->OpenOwner(), NULL, NULL, 0, false,
			fDelegation->Type());

		status_t result = request.Send();
		if (result != B_OK) {
			fFileSystem->OpenOwnerSequenceUnlock(false);
			return result;
		}

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), server))
			continue;

 		fFileSystem->OpenOwnerSequenceUnlock();

		reply.PutFH();

		result = reply.Open(fStateID, &fStateSeq, &confirm, &delegation);
		if (result != B_OK)
			return result;
	} while (true);

	if (delegation.fRecall)
		fDelegation->GiveUp();

	if (confirm)
		return ConfirmOpen(fInfo.fHandle, this);

	return B_OK;
}


status_t
OpenState::_ReclaimLocks(uint64 newClientID)
{
	MutexLocker _(fLocksLock);
	LockInfo* linfo = fLocks;
	while (linfo != NULL) {
		MutexLocker locker(linfo->fOwner->fLock);

		if (linfo->fOwner->fClientId != newClientID) {
			memset(linfo->fOwner->fStateId, 0, sizeof(linfo->fOwner->fStateId));
			linfo->fOwner->fClientId = newClientID;
		}

		do {
			RPC::Server* server = fFileSystem->Server();
			Request request(server);
			RequestBuilder& req = request.Builder();

			req.PutFH(fInfo.fHandle);
			req.Lock(this, linfo, true);

			status_t result = request.Send();
			if (result != B_OK)
				break;

			ReplyInterpreter &reply = request.Reply();

			if (HandleErrors(reply.NFS4Error(), server))
				continue;

			reply.PutFH();
			reply.Lock(linfo);

			break;
		} while (true);
		locker.Unlock();

		linfo = linfo->fNext;
	}

	return B_OK;
}


status_t
OpenState::Close()
{
	if (!fOpened)
		return B_OK;

	MutexLocker _(fLock);
	fOpened = false;

	uint32 sequence = fFileSystem->OpenOwnerSequenceLock();
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.Close(sequence, fStateID, fStateSeq);

		status_t result = request.Send();
		if (result != B_OK) {
			fFileSystem->OpenOwnerSequenceUnlock(false);
			return result;
		}

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(reply.NFS4Error(), serv, NULL, this))
			continue;
 		fFileSystem->OpenOwnerSequenceUnlock();

		reply.PutFH();

		return reply.Close();
	} while (true);
}

