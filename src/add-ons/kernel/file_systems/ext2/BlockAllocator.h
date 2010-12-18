/*
 * Copyright 2001-2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */
#ifndef BLOCKALLOCATOR_H
#define BLOCKALLOCATOR_H


#include <lock.h>

#include "ext2.h"
#include "Transaction.h"


class AllocationBlockGroup;
class Inode;
class Volume;


class BlockAllocator {
public:
						BlockAllocator(Volume* volume);
						~BlockAllocator();

			status_t	Initialize();

			status_t	AllocateBlocks(Transaction& transaction,
							uint32 minimum, uint32 maximum, uint32& blockGroup,
							fsblock_t& start, uint32& length);
			status_t	Allocate(Transaction& transaction, Inode* inode,
							off_t numBlocks, uint32 minimum, fsblock_t& start,
							uint32& length);
			status_t	Free(Transaction& transaction, fsblock_t start,
							uint32 length);

			uint32		FreeBlocks();

protected:
	static	status_t	_Initialize(BlockAllocator* allocator);


			Volume*		fVolume;
			mutex		fLock;

			AllocationBlockGroup* fGroups;
			uint32		fBlocksPerGroup;
			fsblock_t	fNumBlocks;
			uint32		fNumGroups;
			fsblock_t	fFirstBlock;
};

#endif	// BLOCKALLOCATOR_H
