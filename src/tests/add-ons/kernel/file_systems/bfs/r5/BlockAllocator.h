#ifndef BLOCK_ALLOCATOR_H
#define BLOCK_ALLOCATOR_H
/* BlockAllocator - block bitmap handling and allocation policies
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the MIT License.
*/


#include "Lock.h"


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

		status_t AllocateForInode(Transaction *transaction, const block_run *parent,
					mode_t type, block_run &run);
		status_t Allocate(Transaction *transaction, const Inode *inode, off_t numBlocks,
					block_run &run, uint16 minimum = 1);
		status_t Free(Transaction *transaction, block_run run);

		status_t AllocateBlocks(Transaction *transaction, int32 group, uint16 start,
					uint16 numBlocks, uint16 minimum, block_run &run);

		status_t StartChecking(check_control *control);
		status_t StopChecking(check_control *control);
		status_t CheckNextNode(check_control *control);

		status_t CheckBlockRun(block_run run, const char *type = NULL, check_control *control = NULL, bool allocated = true);
		status_t CheckInode(Inode *inode, check_control *control = NULL);

		size_t BitmapSize() const;

	private:
		bool IsValidCheckControl(check_control *control);
		bool CheckBitmapIsUsedAt(off_t block) const;
		void SetCheckBitmapAt(off_t block);

		static status_t initialize(BlockAllocator *);

		Volume			*fVolume;
		Semaphore		fLock;
		AllocationGroup	*fGroups;
		int32			fNumGroups;
		uint32			fBlocksPerGroup;

		uint32			*fCheckBitmap;
		check_cookie	*fCheckCookie;
};

#endif	/* BLOCK_ALLOCATOR_H */
