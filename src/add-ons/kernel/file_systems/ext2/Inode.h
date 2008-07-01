/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef INODE_H
#define INODE_H


#include <lock.h>

#include "ext2.h"
#include "Volume.h"


class Inode {
public:
						Inode(Volume* volume, ino_t id);
						~Inode();

			status_t	InitCheck();

			ino_t		ID() const { return fID; }

			rw_lock*	Lock() { return &fLock; }

			bool		IsDirectory() const
							{ return S_ISDIR(Mode()); }
			bool		IsFile() const
							{ return S_ISREG(Mode()); }
			bool		IsSymLink() const
							{ return S_ISLNK(Mode()); }

			status_t	CheckPermissions(int accessMode) const;

			mode_t		Mode() const { return fNode->Mode(); }
			int32		Flags() const { return fNode->Flags(); }

			off_t		Size() const { return fNode->Size(); }
			time_t		ModificationTime() const
							{ return fNode->ModificationTime(); }
			time_t		CreationTime() const
							{ return fNode->CreationTime(); }
			time_t		AccessTime() const
							{ return fNode->AccessTime(); }

			//::Volume* _Volume() const { return fVolume; }
			Volume*		GetVolume() const { return fVolume; }

			status_t	FindBlock(off_t offset, uint32& block);
			status_t	ReadAt(off_t pos, uint8 *buffer, size_t *length);

			ext2_inode&	Node() { return *fNode; }

			void*		FileCache() const { return fCache; }
			void*		Map() const { return fMap; }

private:
						Inode(const Inode&);
						Inode &operator=(const Inode&);
							// no implementation

private:
	rw_lock				fLock;
	::Volume*			fVolume;
	ino_t				fID;
	void*				fCache;
	void*				fMap;
	ext2_inode*			fNode;
};

#endif	// INODE_H
