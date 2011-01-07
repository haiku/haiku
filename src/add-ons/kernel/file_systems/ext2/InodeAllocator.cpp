/*
 * Copyright 2001-2011, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Janito V. Ferreira Filho
 */


#include "InodeAllocator.h"

#include <util/AutoLock.h>

#include "BitmapBlock.h"
#include "Inode.h"
#include "Volume.h"


#undef ASSERT
//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m " x)
#	define ASSERT(x) { if (!(x)) kernel_debugger("ext2: assert failed: " #x "\n"); }
#else
#	define TRACE(x...) ;
#	define ASSERT(x) ;
#endif
#define ERROR(x...) dprintf("\33[34mext2:\33[0m " x)


InodeAllocator::InodeAllocator(Volume* volume)
	:
	fVolume(volume)
{
	mutex_init(&fLock, "ext2 inode allocator");
}


InodeAllocator::~InodeAllocator()
{
	mutex_destroy(&fLock);
}


/*virtual*/ status_t
InodeAllocator::New(Transaction& transaction, Inode* parent, int32 mode,
	ino_t& id)
{
	// Apply allocation policy
	uint32 preferredBlockGroup = parent != NULL ? (parent->ID() - 1)
		/ parent->GetVolume()->InodesPerGroup() : 0;
	
	return _Allocate(transaction, preferredBlockGroup, S_ISDIR(mode), id);
}


/*virtual*/ status_t
InodeAllocator::Free(Transaction& transaction, ino_t id, bool isDirectory)
{
	TRACE("InodeAllocator::Free(%d, %c)\n", (int)id, isDirectory ? 't' : 'f');
	MutexLocker lock(fLock);

	uint32 numInodes = fVolume->InodesPerGroup();
	uint32 blockGroup = (id - 1) / numInodes;
	ext2_block_group* group;

	status_t status = fVolume->GetBlockGroup(blockGroup, &group);
	if (status != B_OK)
		return status;

	if (group->Flags() & EXT2_BLOCK_GROUP_INODE_UNINIT)
		panic("InodeAllocator::Free() can't free inodes if uninit\n");

	if (blockGroup == fVolume->NumGroups() - 1)
		numInodes = fVolume->NumInodes() - blockGroup * numInodes;

	TRACE("InodeAllocator::Free(): Updating block group data\n");
	group->SetFreeInodes(group->FreeInodes(fVolume->Has64bitFeature()) + 1,
		fVolume->Has64bitFeature());
	if (isDirectory) {
		group->SetUsedDirectories(
			group->UsedDirectories(fVolume->Has64bitFeature()) - 1,
			fVolume->Has64bitFeature());
	}

	status = fVolume->WriteBlockGroup(transaction, blockGroup);
	if (status != B_OK)
		return status;

	return _UnmarkInBitmap(transaction,
		group->InodeBitmap(fVolume->Has64bitFeature()), numInodes, id);
}


status_t
InodeAllocator::_Allocate(Transaction& transaction, uint32 preferredBlockGroup,
	bool isDirectory, ino_t& id)
{
	MutexLocker lock(fLock);

	uint32 blockGroup = preferredBlockGroup;
	uint32 lastBlockGroup = fVolume->NumGroups() - 1;

	for (int i = 0; i < 2; ++i) {
		for (; blockGroup < lastBlockGroup; ++blockGroup) {
			if (_AllocateInGroup(transaction, blockGroup,
				isDirectory, id, fVolume->InodesPerGroup()) == B_OK)
				return B_OK;
		}

		if (i == 0 && _AllocateInGroup(transaction, blockGroup,
			isDirectory, id, fVolume->NumInodes() - blockGroup
				* fVolume->InodesPerGroup()) == B_OK)
			return B_OK;

		blockGroup = 0;
		lastBlockGroup = preferredBlockGroup;
	}

	ERROR("InodeAllocator::_Allocate() device is full\n");
	return B_DEVICE_FULL;
}


status_t
InodeAllocator::_AllocateInGroup(Transaction& transaction, uint32 blockGroup,
	bool isDirectory, ino_t& id, uint32 numInodes)
{
	ext2_block_group* group;
	status_t status = fVolume->GetBlockGroup(blockGroup, &group);
	if (status != B_OK) {
		ERROR("InodeAllocator::_Allocate() GetBlockGroup() failed\n");
		return status;
	}

	fsblock_t block = group->InodeBitmap(fVolume->Has64bitFeature());
	_InitGroup(transaction, group, block, fVolume->InodesPerGroup());
	uint32 freeInodes = group->FreeInodes(fVolume->Has64bitFeature());
	if (freeInodes == 0)
		return B_DEVICE_FULL;
	TRACE("InodeAllocator::_Allocate() freeInodes %ld\n",
		freeInodes);
	group->SetFreeInodes(freeInodes - 1, fVolume->Has64bitFeature());
	if (isDirectory) {
		group->SetUsedDirectories(group->UsedDirectories(
			fVolume->Has64bitFeature()) + 1,
			fVolume->Has64bitFeature());
	}
	
	uint32 pos = 0;
	status = _MarkInBitmap(transaction, block, blockGroup, 
		fVolume->InodesPerGroup(), pos);
	if (status != B_OK)
		return status;

	if (fVolume->HasChecksumFeature() && pos > (fVolume->InodesPerGroup() - 1
		- group->UnusedInodes(fVolume->Has64bitFeature()))) {
		group->SetUnusedInodes(fVolume->InodesPerGroup() - 1 - pos,
			fVolume->Has64bitFeature());
	}

	status = fVolume->WriteBlockGroup(transaction, blockGroup);
	if (status != B_OK)
		return status;

	id = pos + blockGroup * fVolume->InodesPerGroup() + 1;

	return status;
}


status_t
InodeAllocator::_MarkInBitmap(Transaction& transaction, fsblock_t bitmapBlock,
	uint32 blockGroup, uint32 numInodes, uint32& pos)
{
	BitmapBlock inodeBitmap(fVolume, numInodes);

	if (!inodeBitmap.SetToWritable(transaction, bitmapBlock)) {
		ERROR("Unable to open inode bitmap (block number: %llu) for block group "
			"%lu\n", bitmapBlock, blockGroup);
		return B_IO_ERROR;
	}

	pos = 0;
	inodeBitmap.FindNextUnmarked(pos);

	if (pos == inodeBitmap.NumBits()) {
		ERROR("Even though the block group %lu indicates there are free "
			"inodes, no unmarked bit was found in the inode bitmap at block "
			"%llu (numInodes %lu).\n", blockGroup, bitmapBlock, numInodes);
		return B_ERROR;
	}

	if (!inodeBitmap.Mark(pos, 1)) {
		ERROR("Failed to mark bit %lu at bitmap block %llu\n", pos,
			bitmapBlock);
		return B_BAD_DATA;
	}

	return B_OK;
}


status_t
InodeAllocator::_UnmarkInBitmap(Transaction& transaction, fsblock_t bitmapBlock,
	uint32 numInodes, ino_t id)
{
	BitmapBlock inodeBitmap(fVolume, numInodes);

	if (!inodeBitmap.SetToWritable(transaction, bitmapBlock)) {
		ERROR("Unable to open inode bitmap at block %llu\n", bitmapBlock);
		return B_IO_ERROR;
	}

	uint32 pos = (id - 1) % fVolume->InodesPerGroup();
	if (!inodeBitmap.Unmark(pos, 1)) {
		ERROR("Unable to unmark bit %lu in inode bitmap block %llu\n", pos,
			bitmapBlock);
		return B_BAD_DATA;
	}

	return B_OK;
}


status_t
InodeAllocator::_InitGroup(Transaction& transaction, ext2_block_group* group,
	fsblock_t bitmapBlock, uint32 numInodes)
{
	uint16 flags = group->Flags();
	if ((flags & EXT2_BLOCK_GROUP_INODE_UNINIT) == 0)
		return B_OK;

	TRACE("InodeAllocator::_InitGroup() initing group\n");
	BitmapBlock inodeBitmap(fVolume, numInodes);
	if (!inodeBitmap.SetToWritable(transaction, bitmapBlock))
		return B_ERROR;
	inodeBitmap.Unmark(0, numInodes, true);
	group->SetFlags(flags & ~EXT2_BLOCK_GROUP_INODE_UNINIT);

	return B_OK;
}

