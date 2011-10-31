/*
 * Copyright 2001-2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef VOLUME_H
#define VOLUME_H


#include "system_dependencies.h"

#include "bfs.h"
#include "BlockAllocator.h"


class Journal;
class Inode;
class Query;


enum volume_flags {
	VOLUME_READ_ONLY	= 0x0001
};

enum volume_initialize_flags {
	VOLUME_NO_INDICES	= 0x0001,
};

typedef DoublyLinkedList<Inode> InodeList;


class Volume {
public:
							Volume(fs_volume* volume);
							~Volume();

			status_t		Mount(const char* device, uint32 flags);
			status_t		Unmount();
			status_t		Initialize(int fd, const char* name,
								uint32 blockSize, uint32 flags);

			bool			IsInitializing() const { return fVolume == NULL; }

			bool			IsValidSuperBlock();
			bool			IsReadOnly() const;
			void			Panic();
			mutex&			Lock();

			block_run		Root() const { return fSuperBlock.root_dir; }
			Inode*			RootNode() const { return fRootNode; }
			block_run		Indices() const { return fSuperBlock.indices; }
			Inode*			IndicesNode() const { return fIndicesNode; }
			block_run		Log() const { return fSuperBlock.log_blocks; }
			vint32&			LogStart() { return fLogStart; }
			vint32&			LogEnd() { return fLogEnd; }
			int				Device() const { return fDevice; }

			dev_t			ID() const { return fVolume ? fVolume->id : -1; }
			fs_volume*		FSVolume() const { return fVolume; }
			const char*		Name() const { return fSuperBlock.name; }

			off_t			NumBlocks() const
								{ return fSuperBlock.NumBlocks(); }
			off_t			UsedBlocks() const
								{ return fSuperBlock.UsedBlocks(); }
			off_t			FreeBlocks() const
								{ return NumBlocks() - UsedBlocks(); }

			uint32			DeviceBlockSize() const { return fDeviceBlockSize; }
			uint32			BlockSize() const { return fBlockSize; }
			uint32			BlockShift() const { return fBlockShift; }
			uint32			InodeSize() const
								{ return fSuperBlock.InodeSize(); }
			uint32			AllocationGroups() const
								{ return fSuperBlock.AllocationGroups(); }
			uint32			AllocationGroupShift() const
								{ return fAllocationGroupShift; }
			disk_super_block& SuperBlock() { return fSuperBlock; }

			off_t			ToOffset(block_run run) const
								{ return ToBlock(run) << BlockShift(); }
			off_t			ToBlock(block_run run) const
								{ return ((((off_t)run.AllocationGroup())
										<< AllocationGroupShift())
									| (off_t)run.Start()); }
			block_run		ToBlockRun(off_t block) const;
			status_t		ValidateBlockRun(block_run run);

			off_t			ToVnode(block_run run) const
								{ return ToBlock(run); }
			off_t			ToVnode(off_t block) const { return block; }
			off_t			VnodeToBlock(ino_t id) const { return (off_t)id; }

			status_t		CreateIndicesRoot(Transaction& transaction);

			status_t		CreateVolumeID();

			InodeList&		RemovedInodes() { return fRemovedInodes; }
				// This list is guarded by the transaction lock

			// block bitmap
			BlockAllocator&	Allocator();
			status_t		AllocateForInode(Transaction& transaction,
								const Inode* parent, mode_t type,
								block_run& run);
			status_t		AllocateForInode(Transaction& transaction,
								const block_run* parent, mode_t type,
								block_run& run);
			status_t		Allocate(Transaction& transaction, Inode* inode,
								off_t numBlocks, block_run& run,
								uint16 minimum = 1);
			status_t		Free(Transaction& transaction, block_run run);
			void			SetCheckingThread(thread_id thread)
								{ fCheckingThread = thread; }
			bool			IsCheckingThread() const
								{ return find_thread(NULL) == fCheckingThread; }

			// cache access
			status_t		WriteSuperBlock();
			status_t		FlushDevice();

			// queries
			void			UpdateLiveQueries(Inode* inode,
								const char* attribute, int32 type,
								const uint8* oldKey, size_t oldLength,
								const uint8* newKey, size_t newLength);
			void			UpdateLiveQueriesRenameMove(Inode* inode,
								ino_t oldDirectoryID, const char* oldName,
								ino_t newDirectoryID, const char* newName);

			bool			CheckForLiveQuery(const char* attribute);
			void			AddQuery(Query* query);
			void			RemoveQuery(Query* query);

			status_t		Sync();
			Journal*		GetJournal(off_t refBlock) const;

			void*			BlockCache() { return fBlockCache; }

	static	status_t		CheckSuperBlock(const uint8* data,
								uint32* _offset = NULL);
	static	status_t		Identify(int fd, disk_super_block* superBlock);

protected:
			fs_volume*		fVolume;
			int				fDevice;
			disk_super_block fSuperBlock;

			uint32			fDeviceBlockSize;
			uint32			fBlockSize;
			uint32			fBlockShift;
			uint32			fAllocationGroupShift;

			BlockAllocator	fBlockAllocator;
			mutex			fLock;
			Journal*		fJournal;
			vint32			fLogStart;
			vint32			fLogEnd;

			Inode*			fRootNode;
			Inode*			fIndicesNode;

			vint32			fDirtyCachedBlocks;

			mutex			fQueryLock;
			SinglyLinkedList<Query> fQueries;

			uint32			fFlags;

			void*			fBlockCache;
			thread_id		fCheckingThread;

			InodeList		fRemovedInodes;
};


// inline functions

inline bool
Volume::IsReadOnly() const
{
	 return fFlags & VOLUME_READ_ONLY;
}


inline mutex&
Volume::Lock()
{
	 return fLock;
}


inline BlockAllocator&
Volume::Allocator()
{
	 return fBlockAllocator;
}


inline status_t
Volume::AllocateForInode(Transaction& transaction, const block_run* parent,
	mode_t type, block_run& run)
{
	return fBlockAllocator.AllocateForInode(transaction, parent, type, run);
}


inline status_t
Volume::Allocate(Transaction& transaction, Inode* inode, off_t numBlocks,
	block_run& run, uint16 minimum)
{
	return fBlockAllocator.Allocate(transaction, inode, numBlocks, run,
		minimum);
}


inline status_t
Volume::Free(Transaction& transaction, block_run run)
{
	return fBlockAllocator.Free(transaction, run);
}


inline status_t
Volume::FlushDevice()
{
	return block_cache_sync(fBlockCache);
}


inline Journal*
Volume::GetJournal(off_t /*refBlock*/) const
{
	 return fJournal;
}


#endif	// VOLUME_H
