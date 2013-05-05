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
#include "WorkQueue.h"


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
	if (fOpened)
		fFileSystem->RemoveOpenFile(this);
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
	ASSERT(owner != NULL);

	uint32 attempt = 0;
	do {
		RPC::Server* server = fFileSystem->Server();
		Request request(server, fFileSystem);
		RequestBuilder& req = request.Builder();

		req.ReleaseLockOwner(this, owner);

		status_t result = request.Send();
		if (result != B_OK)
			return result;

		ReplyInterpreter& reply = request.Reply();

		if (HandleErrors(attempt, reply.NFS4Error(), server))
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
	delegation.fType = OPEN_DELEGATE_NONE;
	delegation.fRecall = false;

	status_t result;
	uint32 sequence = fFileSystem->OpenOwnerSequenceLock();
	OpenDelegation delegType = fDelegation != NULL ? fDelegation->Type()
		: OPEN_DELEGATE_NONE;
	uint32 attempt = 0;
	do {
		RPC::Server* server = fFileSystem->Server();
		Request request(server, fFileSystem);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.Open(CLAIM_PREVIOUS, sequence, sModeToAccess(fMode), newClientID,
			OPEN4_NOCREATE, fFileSystem->OpenOwner(), NULL, NULL, 0, false,
			delegType);

		result = request.Send();
		if (result != B_OK) {
			fFileSystem->OpenOwnerSequenceUnlock(sequence);
			return result;
		}

		ReplyInterpreter& reply = request.Reply();

		result = reply.PutFH();
		if (result == B_OK)
			sequence += IncrementSequence(reply.NFS4Error());

		if (reply.NFS4Error() != NFS4ERR_STALE_CLIENTID
			&& HandleErrors(attempt, reply.NFS4Error(), server, NULL, NULL,
				&sequence)) {
			continue;
		}

		result = reply.Open(fStateID, &fStateSeq, &confirm, &delegation);
		if (result != B_OK) {
			fFileSystem->OpenOwnerSequenceUnlock(sequence);
			return result;
		}

		break;
	} while (true);

	if (fDelegation != NULL)
		fDelegation->SetData(delegation);

	if (delegation.fRecall) {
		DelegationRecallArgs* args = new(std::nothrow) DelegationRecallArgs;
		args->fDelegation = fDelegation;
		args->fTruncate = false;
		gWorkQueue->EnqueueJob(DelegationRecall, args);
	}

	if (confirm)
		result = ConfirmOpen(fInfo.fHandle, this, &sequence);

 	fFileSystem->OpenOwnerSequenceUnlock(sequence);
	return result;
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

		uint32 attempt = 0;
		uint32 sequence = fFileSystem->OpenOwnerSequenceLock();
		do {
			RPC::Server* server = fFileSystem->Server();
			Request request(server, fFileSystem);
			RequestBuilder& req = request.Builder();

			req.PutFH(fInfo.fHandle);
			req.Lock(this, linfo, &sequence, true);

			status_t result = request.Send();
			if (result != B_OK) {
				fFileSystem->OpenOwnerSequenceUnlock(sequence);
				break;
			}

			ReplyInterpreter& reply = request.Reply();

			result = reply.PutFH();
			if (result == B_OK)
				sequence += IncrementSequence(reply.NFS4Error());

			if (reply.NFS4Error() != NFS4ERR_STALE_CLIENTID
				&& reply.NFS4Error() !=  NFS4ERR_STALE_STATEID
				&& HandleErrors(attempt, reply.NFS4Error(), server, NULL, NULL,
					&sequence)) {
				continue;
			}

			reply.Lock(linfo);

			fFileSystem->OpenOwnerSequenceUnlock(sequence);
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

	uint32 attempt = 0;
	uint32 sequence = fFileSystem->OpenOwnerSequenceLock();
	do {
		RPC::Server* serv = fFileSystem->Server();
		Request request(serv, fFileSystem);
		RequestBuilder& req = request.Builder();

		req.PutFH(fInfo.fHandle);
		req.Close(sequence, fStateID, fStateSeq);

		status_t result = request.Send();
		if (result != B_OK) {
			fFileSystem->OpenOwnerSequenceUnlock(sequence);
			return result;
		}

		ReplyInterpreter& reply = request.Reply();

		result = reply.PutFH();
		if (result == B_OK)
			sequence += IncrementSequence(reply.NFS4Error());

		// RFC 3530 8.10.1. Some servers does not do anything to help client
		// recognize retried CLOSE requests so we just assume that BAD_STATEID
		// on CLOSE request is just a result of retransmission.
		if (reply.NFS4Error() == NFS4ERR_BAD_STATEID) {
			fFileSystem->OpenOwnerSequenceUnlock(sequence);
			return B_OK;
		}

		if (HandleErrors(attempt, reply.NFS4Error(), serv, NULL, this,
				&sequence)) {
			continue;
		}
 		fFileSystem->OpenOwnerSequenceUnlock(sequence);

		return reply.Close();
	} while (true);
}

