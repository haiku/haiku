#ifndef VOLUME_H
#define VOLUME_H
/* Volume - BFS super block, mounting, etc.
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include <KernelExport.h>

extern "C" {
	#ifndef _IMPEXP_KERNEL
	#	define _IMPEXP_KERNEL
	#endif
	#include "fsproto.h"
	#include "lock.h"
	#include "cache.h"
}

#include "bfs.h"
#include "BlockAllocator.h"
#include "Chain.h"

class Journal;
class Inode;
class Query;

enum volume_flags {
	VOLUME_READ_ONLY	= 0x0001
};


class Volume {
	public:
		Volume(nspace_id id);
		~Volume();

		status_t			Mount(const char *device,uint32 flags);
		status_t			Unmount();

		bool				IsValidSuperBlock();
		bool				IsReadOnly() const { return fFlags & VOLUME_READ_ONLY; }
		void				Panic();
		Benaphore			&Lock() { return fLock; }

		block_run			Root() const { return fSuperBlock.root_dir; }
		Inode				*RootNode() const { return fRootNode; }
		block_run			Indices() const { return fSuperBlock.indices; }
		Inode				*IndicesNode() const { return fIndicesNode; }
		block_run			Log() const { return fSuperBlock.log_blocks; }
		vint32				&LogStart() { return fLogStart; }
		vint32				&LogEnd() { return fLogEnd; }
		int					Device() const { return fDevice; }

		nspace_id			ID() const { return fID; }
		const char			*Name() const { return fSuperBlock.name; }

		off_t				NumBlocks() const { return fSuperBlock.num_blocks; }
		off_t				UsedBlocks() const { return fSuperBlock.used_blocks; }
		off_t				FreeBlocks() const { return fSuperBlock.num_blocks - fSuperBlock.used_blocks; }

		uint32				BlockSize() const { return fSuperBlock.block_size; }
		uint32				BlockShift() const { return fSuperBlock.block_shift; }
		uint32				InodeSize() const { return fSuperBlock.inode_size; }
		uint32				AllocationGroups() const { return fSuperBlock.num_ags; }
		uint32				AllocationGroupShift() const { return fSuperBlock.ag_shift; }
		disk_super_block	&SuperBlock() { return fSuperBlock; }

		off_t				ToOffset(block_run run) const { return ToBlock(run) << fSuperBlock.block_shift; }
		off_t				ToBlock(block_run run) const { return ((((off_t)run.allocation_group) << fSuperBlock.ag_shift) | (off_t)run.start); }
		block_run			ToBlockRun(off_t block) const;
		status_t			IsValidBlockRun(block_run run);
		
		off_t				ToVnode(block_run run) const { return ToBlock(run); }
		off_t				ToVnode(off_t block) const { return block; }
		off_t				VnodeToBlock(vnode_id id) const { return (off_t)id; }

		status_t			CreateIndicesRoot(Transaction *transaction);

		status_t			AllocateForInode(Transaction *transaction,const Inode *parent,mode_t type,block_run &run);
		status_t			AllocateForInode(Transaction *transaction,const block_run *parent,mode_t type,block_run &run);
		status_t			Allocate(Transaction *transaction,const Inode *inode,off_t numBlocks,block_run &run,uint16 minimum = 1);
		status_t			Free(Transaction *transaction,block_run &run);

#ifdef DEBUG
		BlockAllocator		&Allocator() { return fBlockAllocator; }
#endif

		status_t			Sync();
		Journal				*GetJournal(off_t /*refBlock*/) const { return fJournal; }

		status_t			WriteSuperBlock();
		status_t			WriteBlocks(off_t blockNumber,const uint8 *block,uint32 numBlocks);
		void				WriteCachedBlocksIfNecessary();
		status_t			FlushDevice();

		void				UpdateLiveQueries(Inode *inode,const char *attribute,int32 type,const uint8 *oldKey,size_t oldLength,const uint8 *newKey,size_t newLength);
		void				AddQuery(Query *query);
		void				RemoveQuery(Query *query);

		uint32				GetUniqueID() { return atomic_add(&fUniqueID,1); }


	protected:
		nspace_id			fID;
		int					fDevice;
		disk_super_block	fSuperBlock;
		BlockAllocator		fBlockAllocator;
		Benaphore			fLock;
		Journal				*fJournal;
		vint32				fLogStart,fLogEnd;

		Inode				*fRootNode;
		Inode				*fIndicesNode;

		vint32				fDirtyCachedBlocks;

		SimpleLock			fQueryLock;
		Chain<Query>		fQueries;

		int32				fUniqueID;
		uint32				fFlags;
};

// inline functions

inline status_t 
Volume::AllocateForInode(Transaction *transaction, const block_run *parent, mode_t type, block_run &run)
{
	return fBlockAllocator.AllocateForInode(transaction,parent,type,run);
}


inline status_t 
Volume::Allocate(Transaction *transaction, const Inode *inode, off_t numBlocks, block_run &run, uint16 minimum)
{
	return fBlockAllocator.Allocate(transaction,inode,numBlocks,run,minimum);
}


inline status_t 
Volume::Free(Transaction *transaction, block_run &run)
{
	return fBlockAllocator.Free(transaction,run);
}


inline status_t 
Volume::WriteBlocks(off_t blockNumber, const uint8 *block, uint32 numBlocks)
{
	atomic_add(&fDirtyCachedBlocks,numBlocks);
	return cached_write(fDevice,blockNumber,block,numBlocks,fSuperBlock.block_size);
}


inline void 
Volume::WriteCachedBlocksIfNecessary()
{
	// the specific values are only valid for the current BeOS cache
	if (fDirtyCachedBlocks > 128) {
		force_cache_flush(fDevice,false);
		atomic_add(&fDirtyCachedBlocks,-64);
	}
}


inline status_t 
Volume::FlushDevice()
{
	fDirtyCachedBlocks = 0;
	return flush_device(fDevice,0);
}


#endif	/* VOLUME_H */
