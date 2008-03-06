/*
 * Copyright 2001-2008, Axel Dörfler, axeld@pinc-software.de. All Rights Reserved.
 * This file may be used under the terms of the MIT License.
 */
#ifndef VOLUME_H
#define VOLUME_H


#include <KernelExport.h>
#include "fsproto.h"

#include "cache.h"

#include "bfs.h"
#include "BlockAllocator.h"
#include "BufferPool.h"
#include "Chain.h"

class Journal;
class Inode;
class Query;

enum volume_flags {
	VOLUME_READ_ONLY	= 0x0001
};

enum volume_initialize_flags {
	VOLUME_NO_INDICES	= 0x0001,
};

class Volume {
	public:
		Volume(dev_t id);
		~Volume();

		status_t			Mount(const char *device, uint32 flags);
		status_t			Unmount();
		status_t			Initialize(const char *device, const char *name,
								uint32 blockSize, uint32 flags);

		bool				IsValidSuperBlock();
		bool				IsReadOnly() const;
		void				Panic();
		RecursiveLock		&Lock();

		block_run			Root() const { return fSuperBlock.root_dir; }
		Inode				*RootNode() const { return fRootNode; }
		block_run			Indices() const { return fSuperBlock.indices; }
		Inode				*IndicesNode() const { return fIndicesNode; }
		block_run			Log() const { return fSuperBlock.log_blocks; }
		vint32				&LogStart() { return fLogStart; }
		vint32				&LogEnd() { return fLogEnd; }
		int					Device() const { return fDevice; }

		dev_t				ID() const { return fID; }
		const char			*Name() const { return fSuperBlock.name; }

		off_t				NumBlocks() const { return fSuperBlock.NumBlocks(); }
		off_t				UsedBlocks() const { return fSuperBlock.UsedBlocks(); }
		off_t				FreeBlocks() const { return NumBlocks() - UsedBlocks(); }

		uint32				BlockSize() const { return fBlockSize; }
		uint32				BlockShift() const { return fBlockShift; }
		uint32				InodeSize() const { return fSuperBlock.InodeSize(); }
		uint32				AllocationGroups() const { return fSuperBlock.AllocationGroups(); }
		uint32				AllocationGroupShift() const { return fAllocationGroupShift; }
		disk_super_block	&SuperBlock() { return fSuperBlock; }

		off_t				ToOffset(block_run run) const { return ToBlock(run) << BlockShift(); }
		off_t				ToBlock(block_run run) const { return ((((off_t)run.AllocationGroup()) << AllocationGroupShift()) | (off_t)run.Start()); }
		block_run			ToBlockRun(off_t block) const;
		status_t			ValidateBlockRun(block_run run);

		off_t				ToVnode(block_run run) const { return ToBlock(run); }
		off_t				ToVnode(off_t block) const { return block; }
		off_t				VnodeToBlock(vnode_id id) const { return (off_t)id; }

		status_t			CreateIndicesRoot(Transaction *transaction);

		// block bitmap
		BlockAllocator		&Allocator();
		status_t			AllocateForInode(Transaction *transaction, const Inode *parent,
								mode_t type, block_run &run);
		status_t			AllocateForInode(Transaction *transaction, const block_run *parent,
								mode_t type, block_run &run);
		status_t			Allocate(Transaction *transaction,const Inode *inode,
								off_t numBlocks, block_run &run, uint16 minimum = 1);
		status_t			Free(Transaction *transaction, block_run run);

		// cache access
		status_t			WriteSuperBlock();
		status_t			WriteBlocks(off_t blockNumber, const uint8 *block, uint32 numBlocks);
		void				WriteCachedBlocksIfNecessary();
		status_t			FlushDevice();

		// queries
		void				UpdateLiveQueries(Inode *inode, const char *attribute, int32 type,
								const uint8 *oldKey, size_t oldLength,
								const uint8 *newKey, size_t newLength);
		bool				CheckForLiveQuery(const char *attribute);
		void				AddQuery(Query *query);
		void				RemoveQuery(Query *query);

		status_t			Sync();
		Journal				*GetJournal(off_t refBlock) const;

		BufferPool			&Pool();

		uint32				GetUniqueID();

		static status_t		Identify(int fd, disk_super_block *superBlock);

	protected:
		dev_t				fID;
		int					fDevice;
		disk_super_block	fSuperBlock;

		uint32				fBlockSize;
		uint32				fBlockShift;
		uint32				fAllocationGroupShift;

		BlockAllocator		fBlockAllocator;
		RecursiveLock		fLock;
		Journal				*fJournal;
		vint32				fLogStart, fLogEnd;

		Inode				*fRootNode;
		Inode				*fIndicesNode;

		vint32				fDirtyCachedBlocks;

		SimpleLock			fQueryLock;
		Chain<Query>		fQueries;

		int32				fUniqueID;
		uint32				fFlags;

		BufferPool			fBufferPool;
};


// inline functions

inline bool 
Volume::IsReadOnly() const
{
	 return fFlags & VOLUME_READ_ONLY;
}


inline RecursiveLock &
Volume::Lock()
{
	 return fLock;
}


inline BlockAllocator &
Volume::Allocator()
{
	 return fBlockAllocator;
}


inline status_t 
Volume::AllocateForInode(Transaction *transaction, const block_run *parent, mode_t type, block_run &run)
{
	return fBlockAllocator.AllocateForInode(transaction, parent, type, run);
}


inline status_t 
Volume::Allocate(Transaction *transaction, const Inode *inode, off_t numBlocks, block_run &run, uint16 minimum)
{
	return fBlockAllocator.Allocate(transaction, inode, numBlocks, run, minimum);
}


inline status_t 
Volume::Free(Transaction *transaction, block_run run)
{
	return fBlockAllocator.Free(transaction, run);
}


inline status_t 
Volume::WriteBlocks(off_t blockNumber, const uint8 *block, uint32 numBlocks)
{
	atomic_add(&fDirtyCachedBlocks, numBlocks);
	return cached_write(fDevice, blockNumber, block, numBlocks, fSuperBlock.block_size);
}


inline void 
Volume::WriteCachedBlocksIfNecessary()
{
	// the specific values are only valid for the current BeOS cache
	if (fDirtyCachedBlocks > 128) {
		force_cache_flush(fDevice, false);
		atomic_add(&fDirtyCachedBlocks, -64);
	}
}


inline status_t 
Volume::FlushDevice()
{
	fDirtyCachedBlocks = 0;
	return flush_device(fDevice, 0);
}


inline Journal *
Volume::GetJournal(off_t /*refBlock*/) const
{
	 return fJournal;
}


inline BufferPool &
Volume::Pool()
{
	 return fBufferPool;
}


inline uint32 
Volume::GetUniqueID()
{
	 return atomic_add(&fUniqueID, 1);
}

#endif	/* VOLUME_H */
