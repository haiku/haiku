/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef OPENSTATE_H
#define OPENSTATE_H


#include <lock.h>
#include <SupportDefs.h>
#include <util/KernelReferenceable.h>

#include "Cookie.h"
#include "NFS4Object.h"


struct OpenState : public NFS4Object, public KernelReferenceable,
	public DoublyLinkedListLinkImpl<OpenState> {
							OpenState();
							~OpenState();

			uint64			fClientID;

			int				fMode;
			mutex			fLock;

			uint32			fStateID[3];
			uint32			fStateSeq;

			bool			fOpened;
			Delegation*		fDelegation;

			LockInfo*		fLocks;
			mutex			fLocksLock;

			LockOwner*		fLockOwners;
			mutex			fOwnerLock;

			uid_t			fUid;
			gid_t			fGid;

			LockOwner*		GetLockOwner(uint32 owner);

			void			AddLock(LockInfo* lock);
			void			RemoveLock(LockInfo* lock, LockInfo* prev);
			void			DeleteLock(LockInfo* lock);

			status_t		Reclaim(uint64 newClientID);

			status_t		Close();

			void			Dump(void (*xprintf)(const char*, ...) = dprintf);

private:
			status_t		_ReclaimOpen(uint64 newClientID);
			status_t		_ReclaimLocks(uint64 newClientID);
			status_t		_ReleaseLockOwner(LockOwner* owner);

			void			_DumpLocked(void (*xprintf)(const char*, ...)) const;
};


#endif	// OPENSTATE_H

