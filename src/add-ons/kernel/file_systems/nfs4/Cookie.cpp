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
	ASSERT(owner != NULL);
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
	uint64 length = static_cast<uint64>(lock.l_len);

	return fStart == start && (fLength == length
		|| (eof && fLength == UINT64_MAX));
}


bool
LockInfo::operator==(const LockInfo& lock) const
{
	return fOwner == lock.fOwner && fStart == lock.fStart
		&& fLength == lock.fLength && fType == lock.fType;
}


Cookie::Cookie(FileSystem* fileSystem)
	:
	fFileSystem(fileSystem),
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
Cookie::RegisterRequest(RPC::Request* request)
{
	ASSERT(request != NULL);

	RequestEntry* entry = new RequestEntry;
	if (entry == NULL)
		return B_NO_MEMORY;

	MutexLocker _(fRequestLock);
	entry->fRequest = request;
	entry->fNext = fRequests;
	fRequests = entry;

	return B_OK;
}


status_t
Cookie::UnregisterRequest(RPC::Request* request)
{
	ASSERT(request != NULL);

	MutexLocker _(fRequestLock);
	RequestEntry* entry = fRequests;
	RequestEntry* previous = NULL;
	while (entry != NULL) {
		if (entry->fRequest == request) {
			if (previous == NULL)
				fRequests = entry->fNext;
			else
				previous->fNext = entry->fNext;
			delete entry;
			break;
		}

		previous = entry;
		entry = entry->fNext;
	}

	return B_OK;
}


status_t
Cookie::CancelAll()
{
	release_sem(fSnoozeCancel);

	MutexLocker _(fRequestLock);
	RequestEntry* entry = fRequests;
	while (entry != NULL) {
		fFileSystem->Server()->WakeCall(entry->fRequest);
		entry = entry->fNext;
	}

	return B_OK;
}


OpenStateCookie::OpenStateCookie(FileSystem* fileSystem)
	:
	Cookie(fileSystem)
{
}


OpenFileCookie::OpenFileCookie(FileSystem* fileSystem)
	:
	OpenStateCookie(fileSystem),
	fLocks(NULL)
{
}


void
OpenFileCookie::AddLock(LockInfo* lock)
{
	ASSERT(lock != NULL);

	lock->fCookieNext = fLocks;
	fLocks = lock;
}


void
OpenFileCookie::RemoveLock(LockInfo* lock, LockInfo* previous)
{
	if (previous != NULL)
		previous->fCookieNext = lock->fCookieNext;
	else {
		ASSERT(previous == NULL && fLocks == lock);
		fLocks = lock->fCookieNext;
	}
}


OpenDirCookie::OpenDirCookie(FileSystem* fileSystem)
	:
	Cookie(fileSystem),
	fSnapshot(NULL)
{
}


OpenDirCookie::~OpenDirCookie()
{
	if (fSnapshot != NULL)
		fSnapshot->ReleaseReference();
}


OpenAttrCookie::OpenAttrCookie(FileSystem* fileSystem)
	:
	OpenStateCookie(fileSystem)
{
}

