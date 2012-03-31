/*
 * Copyright 2001-2012, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef BLOCK_ALLOCATOR_H
#define BLOCK_ALLOCATOR_H


#include "system_dependencies.h"


class AllocationGroup;
class BPlusTree;
class Inode;
class Transaction;
class Volume;
struct disk_super_block;
struct block_run;
struct check_control;
struct check_cookie;


//#define DEBUG_ALLOCATION_GROUPS
//#define DEBUG_FRAGMENTER


class BlockAllocator {
public:
							BlockAllocator(Volume* volume);
							~BlockAllocator();

			status_t		Initialize(bool full = true);
			status_t		InitializeAndClearBitmap(Transaction& transaction);

			void			Uninitialize();

			status_t		AllocateForInode(Transaction& transaction,
								const block_run* parent, mode_t type,
								block_run& run);
			status_t		Allocate(Transaction& transaction, Inode* inode,
								off_t numBlocks, block_run& run,
								uint16 minimum = 1);
			status_t		Free(Transaction& transaction, block_run run);

			status_t		AllocateBlocks(Transaction& transaction,
								int32 group, uint16 start, uint16 numBlocks,
								uint16 minimum, block_run& run);

			status_t		StartChecking(const check_control* control);
			status_t		StopChecking(check_control* control);
			status_t		CheckNextNode(check_control* control);

			status_t		CheckBlocks(off_t start, off_t length,
								bool allocated = true);
			status_t		CheckBlockRun(block_run run,
								const char* type = NULL,
								bool allocated = true);
			status_t		CheckInode(Inode* inode, const char* name);

			size_t			BitmapSize() const;

#ifdef BFS_DEBUGGER_COMMANDS
			void			Dump(int32 index);
#endif
#ifdef DEBUG_FRAGMENTER
			void			Fragment();
#endif

private:
			status_t		_RemoveInvalidNode(Inode* parent, BPlusTree* tree,
								Inode* inode, const char* name);
#ifdef DEBUG_ALLOCATION_GROUPS
			void			_CheckGroup(int32 group) const;
#endif
			bool			_IsValidCheckControl(const check_control* control);
			bool			_CheckBitmapIsUsedAt(off_t block) const;
			void			_SetCheckBitmapAt(off_t block);
			status_t		_CheckInodeBlocks(Inode* inode, const char* name);
			status_t		_FinishBitmapPass();
			status_t		_PrepareIndices();
			void			_FreeIndices();
			status_t		_AddInodeToIndex(Inode* inode);
			status_t		_WriteBackCheckBitmap();

	static	status_t		_Initialize(BlockAllocator* self);

private:
			Volume*			fVolume;
			recursive_lock	fLock;
			AllocationGroup* fGroups;
			int32			fNumGroups;
			uint32			fBlocksPerGroup;
			uint32			fNumBlocks;

			uint32*			fCheckBitmap;
			check_cookie*	fCheckCookie;
};

#ifdef BFS_DEBUGGER_COMMANDS
#if BFS_TRACING
int dump_block_allocator_blocks(int argc, char** argv);
#endif
int dump_block_allocator(int argc, char** argv);
#endif

#endif	// BLOCK_ALLOCATOR_H
