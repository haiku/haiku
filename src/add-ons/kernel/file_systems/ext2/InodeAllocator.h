/*
 * Copyright 2001-2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */
#ifndef INODEALLOCATOR_H
#define INODEALLOCATOR_H

#include <lock.h>

#include "ext2.h"
#include "Transaction.h"


class Inode;
class Volume;


class InodeAllocator {
public:
						InodeAllocator(Volume* volume);
	virtual				~InodeAllocator();

	virtual	status_t	New(Transaction& transaction, Inode* parent,
							int32 mode, ino_t& id);
	virtual	status_t	Free(Transaction& transaction, ino_t id,
							bool isDirectory);

private:
			status_t	_Allocate(Transaction& transaction,
							uint32 preferredBlockGroup, bool isDirectory,
							ino_t& id);
			status_t	_AllocateInGroup(Transaction& transaction,
							uint32 blockGroup, bool isDirectory,
							ino_t& id, uint32 numInodes);
			status_t	_MarkInBitmap(Transaction& transaction,
							fsblock_t bitmapBlock, uint32 blockGroup,
							uint32 numInodes, uint32& pos, uint32& checksum);
			status_t	_UnmarkInBitmap(Transaction& transaction,
							fsblock_t bitmapBlock, uint32 numInodes, ino_t id,
							uint32& checksum);
			status_t	_InitGroup(Transaction& transaction,
							ext2_block_group* group, fsblock_t bitmapBlock,
							uint32 numInodes);
			void		_SetInodeBitmapChecksum(ext2_block_group* group,
							uint32 checksum);

			Volume*		fVolume;
			mutex		fLock;
};

#endif	// INODEALLOCATOR_H
