/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "Cookie.h"

#include "Inode.h"
#include "Request.h"


vint64 OpenFileCookie::fLastOwnerId = 0;


LockOwner::LockOwner(uint32 owner)
	:
	fSequence(0),
	fOwner(owner),
	fUseCount(0),
	fNext(NULL),
	fPrev(NULL)
{
	memset(fStateId, 0, sizeof(fStateId));
	mutex_init(&fLock, NULL);
}


LockOwner::~LockOwner()
{
	mutex_destroy(&fLock);
}


LockInfo::LockInfo(LockOwner* owner)
	:
	fOwner(owner)
{
	fOwner->fUseCount++;
}


LockInfo::~LockInfo()
{
	fOwner->fUseCount--;
}


bool
LockInfo::operator==(const struct flock& lock) const
{
	bool eof = lock.l_len + lock.l_start == OFF_MAX;
	uint64 start = static_cast<uint64>(lock.l_start);
	uint64 len = static_cast<uint64>(lock.l_len);

	return fStart == start && fLength == len || eof && fLength == UINT64_MAX;
}


Cookie::Cookie()
	:
	fRequests(NULL),
	fSnoozeCancel(create_sem(1, NULL))
{
	acquire_sem(fSnoozeCancel);
	mutex_init(&fRequestLock, NULL);
}


Cookie::~Cookie()
{
	delete_sem(fSnoozeCancel);
	mutex_destroy(&fRequestLock);
}


status_t
Cookie::RegisterRequest(RPC::Request* req)
{
	RequestEntry* ent = new RequestEntry;
	if (ent == NULL)
		return B_NO_MEMORY;

	MutexLocker _(fRequestLock);
	ent->fRequest = req;
	ent->fNext = fRequests;
	fRequests = ent;

	return B_OK;
}


status_t
Cookie::UnregisterRequest(RPC::Request* req)
{
	MutexLocker _(fRequestLock);
	RequestEntry* ent = fRequests;
	RequestEntry* prev = NULL;
	while (ent != NULL) {
		if (ent->fRequest == req) {
			if (prev == NULL)
				fRequests = ent->fNext;
			else
				prev->fNext = ent->fNext;
			delete ent;
		}

		prev = ent;
		ent = ent->fNext;
	}

	return B_OK;
}


status_t
Cookie::CancelAll()
{
	release_sem(fSnoozeCancel);

	MutexLocker _(fRequestLock);
	RequestEntry* ent = fRequests;
	while (ent != NULL) {
		fFileSystem->Server()->WakeCall(ent->fRequest);
		ent = ent->fNext;
	}

	return B_OK;
}


OpenFileCookie::OpenFileCookie()
	:
	fLocks(NULL),
	fLockOwners(NULL)
{
	mutex_init(&fLocksLock, NULL);
	mutex_init(&fOwnerLock, NULL);
}


OpenFileCookie::~OpenFileCookie()
{
	mutex_destroy(&fLocksLock);
	mutex_destroy(&fOwnerLock);
}


LockOwner*
OpenFileCookie::GetLockOwner(uint32 owner)
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

	current->fClientId = fClientId;
	current->fNext = fLockOwners;
	if (fLockOwners != NULL)
		fLockOwners->fPrev = current;
	fLockOwners = current;

	return current;
}


// Caller must hold fLocksLock
void
OpenFileCookie::AddLock(LockInfo* lock)
{
	lock->fNext = fLocks;
	fLocks = lock;
}


// Caller must hold fLocksLock
void
OpenFileCookie::RemoveLock(LockInfo* lock, LockInfo* prev)
{
	if (prev != NULL)
		prev->fNext = lock->fNext;
	else
		fLocks = lock->fNext;
}


void
OpenFileCookie::DeleteLock(LockInfo* lock)
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
OpenFileCookie::_ReleaseLockOwner(LockOwner* owner)
{
	Request request(fFileSystem->Server());
	RequestBuilder& req = request.Builder();

	req.ReleaseLockOwner(this, owner);

	status_t result = request.Send();
	if (result != B_OK)
		return result;

	ReplyInterpreter &reply = request.Reply();

	return reply.ReleaseLockOwner();
}

