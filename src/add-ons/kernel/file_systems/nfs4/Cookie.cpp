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
		fFilesystem->Server()->WakeCall(ent->fRequest);
		ent = ent->fNext;
	}

	return B_OK;
}


OpenFileCookie::OpenFileCookie()
{
	mutex_init(&fLocksLock, NULL);
}


OpenFileCookie::~OpenFileCookie()
{
	mutex_destroy(&fLocksLock);
}

