/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef COOKIE_H
#define COOKIE_H


#include <SupportDefs.h>

#include "Filesystem.h"


struct LockInfo {
			uint32			fStateId[3];
			uint32			fStateSeq;

			uint32			fSequence;

			uint32			fOwner;
			uint64			fStart;
			uint64			fLength;
			LockType		fType;

			LockInfo*		fNext;
};

struct Cookie {
			struct RequestEntry {
				RPC::Request*	fRequest;
				RequestEntry*	fNext;
			};

			Filesystem*		fFilesystem;
			RequestEntry*	fRequests;
			mutex			fRequestLock;

			sem_id			fSnoozeCancel;

							Cookie();
	virtual					~Cookie();

			status_t		RegisterRequest(RPC::Request* req);
			status_t		UnregisterRequest(RPC::Request* req);
			status_t		CancelAll();
};

struct OpenFileCookie : public Cookie {
			uint64			fClientId;

			uint32			fMode;

			Filehandle		fHandle;
			uint32			fStateId[3];
			uint32			fStateSeq;

			uint32			fSequence;

			uint64			fOwnerId;
	static	vint64			fLastOwnerId;

			LockInfo*		fLocks;
			mutex			fLocksLock;

			OpenFileCookie*	fNext;
			OpenFileCookie*	fPrev;

							OpenFileCookie();
							~OpenFileCookie();
};

struct OpenDirCookie : public Cookie {
			uint64			fCookie;
			uint64			fCookieVerf;
};


#endif	// COOKIE_H

