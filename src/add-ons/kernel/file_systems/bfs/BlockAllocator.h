/*
 * Copyright 2001-2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef BLOCK_ALLOCATOR_H
#define BLOCK_ALLOCATOR_H


#include "system_dependencies.h"


class AllocationGroup;
class Transaction;
class Volume;
class Inode;
struct disk_super_block;
struct block_run;
struct check_control;
struct check_cookie;


class BlockAllocator {
	public:
		BlockAllocator(Volume *volume);
		~BlockAllocator();

		status_t Initialize(bool full = true);
		status_t InitializeAndClearBitmap(Transaction &transaction);

		void Uninitialize();

		status_t AllocateForInode(Transaction &transaction, const block_run *parent,
					mode_t type, block_run &run);
		status_t Allocate(Transaction &transaction, Inode *inode, off_t numBlocks,
					block_run &run, uint16 minimum = 1);
		status_t Free(Transaction &transaction, block_run run);

		status_t AllocateBlocks(Transaction &transaction, int32 group, uint16 start,
					uint16 numBlocks, uint16 minimum, block_run &run);

		status_t StartChecking(check_control *control);
		status_t StopChecking(check_control *control);
		status_t CheckNextNode(check_control *control);

		status_t CheckBlockRun(block_run run, const char *type = NULL, check_control *control = NULL, bool allocated = true);
		status_t CheckInode(Inode *inode, check_control *control = NULL);

		size_t BitmapSize() const;

#ifdef BFS_DEBUGGER_COMMANDS
		void Dump(int32 index);
#endif

	private:
		bool _IsValidCheckControl(check_control *control);
		bool _CheckBitmapIsUsedAt(off_t block) const;
		void _SetCheckBitmapAt(off_t block);

		static status_t _Initialize(BlockAllocator *);

		Volume			*fVolume;
		mutex			fLock;
		AllocationGroup	*fGroups;
		int32			fNumGroups;
		uint32			fBlocksPerGroup;

		uint32			*fCheckBitmap;
		check_cookie	*fCheckCookie;
};

#ifdef BFS_DEBUGGER_COMMANDS
int dump_block_allocator(int argc, char **argv);
#endif

#endif	/* BLOCK_ALLOCATOR_H */
