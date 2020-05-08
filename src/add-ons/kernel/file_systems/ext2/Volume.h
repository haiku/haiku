/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2008-2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef VOLUME_H
#define VOLUME_H


#include <lock.h>

#include "ext2.h"
#include "BlockAllocator.h"
#include "InodeAllocator.h"
#include "Transaction.h"

class Inode;
class Journal;


enum volume_flags {
	VOLUME_READ_ONLY	= 0x0001
};


class Volume : public TransactionListener {
public:
								Volume(fs_volume* volume);
								~Volume();

			status_t			Mount(const char* device, uint32 flags);
			status_t			Unmount();

			bool				IsValidSuperBlock();
			bool				IsReadOnly() const
									{ return (fFlags & VOLUME_READ_ONLY) != 0; }
			mutex&				Lock();
			bool				HasExtendedAttributes() const;

			Inode*				RootNode() const { return fRootNode; }
			int					Device() const { return fDevice; }

			dev_t				ID() const
									{ return fFSVolume ? fFSVolume->id : -1; }
			fs_volume*			FSVolume() const { return fFSVolume; }
			const char*			Name() const;
			void				SetName(const char* name);

			uint32				NumInodes() const
									{ return fNumInodes; }
			uint32				NumGroups() const
									{ return fNumGroups; }
			fsblock_t			NumBlocks() const
									{ return fSuperBlock.NumBlocks(
										Has64bitFeature()); }
			off_t				NumFreeBlocks() const
									{ return fFreeBlocks; }
			uint32				FirstDataBlock() const
									{ return fFirstDataBlock; }

			uint32				BlockSize() const { return fBlockSize; }
			uint32				BlockShift() const { return fBlockShift; }
			uint32				BlocksPerGroup() const
									{ return fSuperBlock.BlocksPerGroup(); }
			uint32				InodeSize() const
									{ return fSuperBlock.InodeSize(); }
			uint32				InodesPerGroup() const
									{ return fSuperBlock.InodesPerGroup(); }
			ext2_super_block&	SuperBlock() { return fSuperBlock; }

			status_t			GetInodeBlock(ino_t id, off_t& block);
			uint32				InodeBlockIndex(ino_t id) const;
			status_t			GetBlockGroup(int32 index,
									ext2_block_group** _group);
			status_t			WriteBlockGroup(Transaction& transaction,
									int32 index);

			Journal*			GetJournal() { return fJournal; }

			bool				IndexedDirectories() const
								{ return (fSuperBlock.CompatibleFeatures()
									& EXT2_FEATURE_DIRECTORY_INDEX) != 0; }
			bool				HasJournalFeature() const
								{ return (fSuperBlock.CompatibleFeatures()
									& EXT2_FEATURE_HAS_JOURNAL) != 0; }
			bool				Has64bitFeature() const
								{ return (fSuperBlock.IncompatibleFeatures()
									& EXT2_INCOMPATIBLE_FEATURE_64BIT) != 0; }
			bool				HasExtentsFeature() const
								{ return (fSuperBlock.IncompatibleFeatures()
									& EXT2_INCOMPATIBLE_FEATURE_EXTENTS)
									!= 0; }
			bool				HasChecksumFeature() const
								{ return (fSuperBlock.ReadOnlyFeatures()
									& EXT2_READ_ONLY_FEATURE_GDT_CSUM) != 0; }
			bool				HasMetaGroupFeature() const
								{ return (fSuperBlock.IncompatibleFeatures()
									& EXT2_INCOMPATIBLE_FEATURE_META_GROUP)
									!= 0; }
			bool				HasMetaGroupChecksumFeature() const
								{ return (fSuperBlock.ReadOnlyFeatures()
									& EXT4_READ_ONLY_FEATURE_METADATA_CSUM)
									!= 0; }
			bool				HasChecksumSeedFeature() const
								{ return (fSuperBlock.IncompatibleFeatures()
									& EXT2_INCOMPATIBLE_FEATURE_CSUM_SEED)
									!= 0; }
			uint8				DefaultHashVersion() const
								{ return fSuperBlock.default_hash_version; }
			bool				HugeFiles() const
								{ return (fSuperBlock.ReadOnlyFeatures()
									& EXT2_READ_ONLY_FEATURE_HUGE_FILE) != 0; }
			status_t			ActivateLargeFiles(Transaction& transaction);
			status_t			ActivateDirNLink(Transaction& transaction);

			status_t			SaveOrphan(Transaction& transaction,
									ino_t newID, ino_t &oldID);
			status_t			RemoveOrphan(Transaction& transaction,
									ino_t id);

			status_t			AllocateInode(Transaction& transaction,
									Inode* parent, int32 mode, ino_t& id);
			status_t			FreeInode(Transaction& transaction, ino_t id,
									bool isDirectory);

			status_t			AllocateBlocks(Transaction& transaction,
									uint32 minimum, uint32 maximum,
									uint32& blockGroup, fsblock_t& start,
									uint32& length);
			status_t			FreeBlocks(Transaction& transaction,
									fsblock_t start, uint32 length);

			status_t			LoadSuperBlock();
			status_t			WriteSuperBlock(Transaction& transaction);

			// cache access
			void*				BlockCache() { return fBlockCache; }

			uint32				ChecksumSeed() const
									{ return fChecksumSeed; }
			uint16				GroupDescriptorSize() const
									{ return fGroupDescriptorSize; }

			status_t			FlushDevice();
			status_t			Sync();

	static	status_t			Identify(int fd, ext2_super_block* superBlock);

			// TransactionListener functions
			void				TransactionDone(bool success);
			void				RemovedFromTransaction();

private:
	static	uint32				_UnsupportedIncompatibleFeatures(
									ext2_super_block& superBlock);
	static	uint32				_UnsupportedReadOnlyFeatures(
									ext2_super_block& superBlock);
			uint32				_GroupDescriptorBlock(uint32 blockIndex);
			uint16				_GroupCheckSum(ext2_block_group *group,
									int32 index);
			void				_SuperBlockChecksumSeed();
			bool				_VerifySuperBlock();

private:
			mutex				fLock;
			fs_volume*			fFSVolume;
			int					fDevice;
			ext2_super_block	fSuperBlock;
			char				fName[32];

			BlockAllocator*		fBlockAllocator;
			InodeAllocator		fInodeAllocator;
			Journal*			fJournal;
			Inode*				fJournalInode;

			uint32				fFlags;
			uint32				fBlockSize;
			uint32				fBlockShift;
			uint32				fFirstDataBlock;

			uint32				fNumInodes;
			uint32				fNumGroups;
			off_t				fFreeBlocks;
			uint32				fFreeInodes;
			uint32				fGroupsPerBlock;
			uint8**				fGroupBlocks;
			uint32				fInodesPerBlock;
			uint16				fGroupDescriptorSize;

			void*				fBlockCache;
			Inode*				fRootNode;

			uint32				fChecksumSeed;
};


inline mutex&
Volume::Lock()
{
	 return fLock;
}

#endif	// VOLUME_H
