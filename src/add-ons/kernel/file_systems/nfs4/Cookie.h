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

class Inode;

struct Cookie {
			struct RequestEntry {
				RPC::Request*	fRequest;
				RequestEntry*	fNext;
			};

			Inode*			fInode;
			RequestEntry*	fRequests;
			mutex			fRequestLock;

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

			OpenFileCookie*	fNext;
			OpenFileCookie*	fPrev;
};

struct OpenDirCookie : public Cookie {
			uint64			fCookie;
			uint64			fCookieVerf;
};


#endif	// COOKIE_H

