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

#include "FileSystem.h"


struct LockOwner {
			uint64			fClientId;

			uint32			fStateId[3];
			uint32			fStateSeq;
			uint32			fSequence;

			uint32			fOwner;

			uint32			fUseCount;

			mutex			fLock;

			LockOwner*		fNext;
			LockOwner*		fPrev;

							LockOwner(uint32 owner);
							~LockOwner();
};

struct LockInfo {
			LockOwner*		fOwner;

			uint64			fStart;
			uint64			fLength;
			LockType		fType;

			LockInfo*		fNext;

							LockInfo(LockOwner* owner);
							~LockInfo();

			bool			operator==(const struct flock& lock) const;
};

struct Cookie {
			struct RequestEntry {
				RPC::Request*	fRequest;
				RequestEntry*	fNext;
			};

			FileSystem*		fFileSystem;
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

			FileInfo		fInfo;
			uint32			fStateId[3];
			uint32			fStateSeq;

			uint32			fSequence;

			uint64			fOwnerId;
	static	vint64			fLastOwnerId;

			LockInfo*		fLocks;
			mutex			fLocksLock;

			LockOwner*		fLockOwners;
			mutex			fOwnerLock;

			OpenFileCookie*	fNext;
			OpenFileCookie*	fPrev;

							OpenFileCookie();
							~OpenFileCookie();

			LockOwner*		GetLockOwner(uint32 owner);

			void			AddLock(LockInfo* lock);
			void			RemoveLock(LockInfo* lock, LockInfo* prev);
			void			DeleteLock(LockInfo* lock);

private:
			status_t		_ReleaseLockOwner(LockOwner* owner);
};

struct OpenDirCookie : public Cookie {
			int							fSpecial;
			DirectoryCacheSnapshot*		fSnapshot;
			NameCacheEntry*				fCurrent;
			bool						fEOF;

										~OpenDirCookie();
};


#endif	// COOKIE_H

