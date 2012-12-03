/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef VNODETOINODE_H
#define VNODETOINODE_H

#include <lock.h>
#include <SupportDefs.h>
#include <util/AutoLock.h>

#include "Inode.h"
#include "InodeIdMap.h"

class VnodeToInode {
public:
	inline				VnodeToInode(ino_t id, FileSystem* fileSystem);
	inline				~VnodeToInode();

	inline	void		Lock();
	inline	void		Unlock();

			Inode*		Get();
			void		Replace(Inode* newInode);

	inline	void		Remove();
	inline	void		Clear();

	inline	ino_t		ID();
private:
			ino_t		fID;
			rw_lock		fLock;

			Inode*		fInode;
			FileSystem*	fFileSystem;
};

class VnodeToInodeLocking {
public:
	inline bool Lock(VnodeToInode* vti)
	{
		vti->Lock();
		return true;
	}

	inline void Unlock(VnodeToInode* vti)
	{
		vti->Unlock();
	}
};

typedef AutoLocker<VnodeToInode, VnodeToInodeLocking> VnodeToInodeLocker;

inline
VnodeToInode::VnodeToInode(ino_t id, FileSystem* fileSystem)
	:
	fID(id),
	fInode(NULL),
	fFileSystem(fileSystem)
{
	rw_lock_init(&fLock, NULL);
}


inline
VnodeToInode::~VnodeToInode()
{
	Remove();
	if (fFileSystem != NULL)
		fFileSystem->InoIdMap()->RemoveEntry(fID);
	rw_lock_destroy(&fLock);
}


inline void
VnodeToInode::Lock()
{
	rw_lock_read_lock(&fLock);
}


inline void
VnodeToInode::Unlock()
{
	rw_lock_read_unlock(&fLock);
}


inline void
VnodeToInode::Remove()
{
	Replace(NULL);
}


inline void
VnodeToInode::Clear()
{
	WriteLocker _(fLock);
	delete fInode;
	fInode = NULL;
}


inline ino_t
VnodeToInode::ID()
{
	return fID;
}

#endif	// VNODETOINODE_H

