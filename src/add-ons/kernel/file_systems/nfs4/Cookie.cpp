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


bool
LockInfo::operator==(const LockInfo& lock) const
{
	return fOwner == lock.fOwner && fStart == lock.fStart
		&& fLength == lock.fLength && fType == lock.fType;
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
	fLocks(NULL)
{
}


void
OpenFileCookie::AddLock(LockInfo* lock)
{
	lock->fCookieNext = fLocks;
	fLocks = lock;
}


void
OpenFileCookie::RemoveLock(LockInfo* lock, LockInfo* prev)
{
	if (prev != NULL)
		prev->fCookieNext = lock->fCookieNext;
	else
		fLocks = lock->fCookieNext;
}


OpenDirCookie::~OpenDirCookie()
{
	if (fSnapshot != NULL)
		fSnapshot->ReleaseReference();
}

