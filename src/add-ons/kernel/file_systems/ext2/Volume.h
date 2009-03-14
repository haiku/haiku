/*
 * Copyright 2008-2009, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef VOLUME_H
#define VOLUME_H


#include <lock.h>

#include "ext2.h"

class Inode;

enum volume_flags {
	VOLUME_READ_ONLY	= 0x0001
};

class Volume {
public:
						Volume(fs_volume* volume);
						~Volume();

			status_t	Mount(const char* device, uint32 flags);
			status_t	Unmount();

			bool		IsValidSuperBlock();
			bool		IsReadOnly() const { return fFlags & VOLUME_READ_ONLY; }

			Inode*		RootNode() const { return fRootNode; }
			int			Device() const { return fDevice; }

			dev_t		ID() const { return fFSVolume ? fFSVolume->id : -1; }
			fs_volume*	FSVolume() const { return fFSVolume; }
			const char*	Name() const;

			off_t		NumBlocks() const { return fSuperBlock.NumBlocks(); }
			off_t		FreeBlocks() const { return fSuperBlock.FreeBlocks(); }

			uint32		BlockSize() const { return fBlockSize; }
			uint32		BlockShift() const { return fBlockShift; }
			uint32		InodeSize() const { return fSuperBlock.InodeSize(); }
			ext2_super_block& SuperBlock() { return fSuperBlock; }

			status_t	GetInodeBlock(ino_t id, uint32& block);
			uint32		InodeBlockIndex(ino_t id) const;
			status_t	GetBlockGroup(int32 index, ext2_block_group** _group);

			// cache access
			void*		BlockCache() { return fBlockCache; }

	static	status_t	Identify(int fd, ext2_super_block* superBlock);

private:
	static uint32		_UnsupportedIncompatibleFeatures(
							ext2_super_block& superBlock);
			off_t		_GroupBlockOffset(uint32 blockIndex);

private:
	mutex				fLock;
	fs_volume*			fFSVolume;
	int					fDevice;
	ext2_super_block	fSuperBlock;
	char				fName[32];
	uint32				fFlags;
	uint32				fBlockSize;
	uint32				fBlockShift;
	uint32				fFirstDataBlock;
	uint32				fNumGroups;
	uint32				fGroupsPerBlock;
	ext2_block_group**	fGroupBlocks;
	uint32				fInodesPerBlock;

	void*				fBlockCache;
	Inode*				fRootNode;
};

#endif	// VOLUME_H
