#ifndef BLOCK_ALLOCATOR_H
#define BLOCK_ALLOCATOR_H
/* BlockAllocator - block bitmap handling and allocation policies
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include <Lock.h>


class AllocationGroup;
class Transaction;
class Volume;
class Inode;
struct disk_super_block;
struct block_run;


class BlockAllocator {
	public:
		BlockAllocator(Volume *volume);
		~BlockAllocator();

		status_t Initialize();

		status_t AllocateForInode(Transaction *transaction,const block_run *parent,mode_t type,block_run &run);
		status_t Allocate(Transaction *transaction,const Inode *inode,off_t numBlocks,block_run &run,uint16 minimum = 1);
		status_t Free(Transaction *transaction,block_run &run);

		status_t AllocateBlocks(Transaction *transaction,int32 group, uint16 start, uint16 numBlocks, uint16 minimum, block_run &run);

#ifdef DEBUG
		status_t CheckBlockRun(block_run run);
		status_t CheckInode(Inode *inode);
		status_t Check(Inode *inode);
#endif

	private:
		static status_t initialize(BlockAllocator *);

		Volume			*fVolume;
		Benaphore		fLock;
		AllocationGroup	*fGroups;
		int32			fNumGroups,fBlocksPerGroup;
};

#endif	/* BLOCK_ALLOCATOR_H */
