/*
 * Copyright 2011, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2008-2010, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef VOLUME_H
#define VOLUME_H


#include <lock.h>

#include "btrfs.h"


enum volume_flags {
	VOLUME_READ_ONLY	= 0x0001
};

class BPlusTree;
class Chunk;
class Inode;


class Volume {
public:
								Volume(fs_volume* volume);
								~Volume();

			status_t			Mount(const char* device, uint32 flags);
			status_t			Unmount();

			bool				IsValidSuperBlock();
			bool				IsReadOnly() const
									{ return (fFlags & VOLUME_READ_ONLY) != 0; }

			Inode*				RootNode() const { return fRootNode; }
			int					Device() const { return fDevice; }

			dev_t				ID() const
									{ return fFSVolume ? fFSVolume->id : -1; }
			fs_volume*			FSVolume() const { return fFSVolume; }
			const char*			Name() const;
			BPlusTree*			FSTree() const { return fFSTree; }
			BPlusTree*			RootTree() const { return fRootTree; }

			uint32				BlockSize() const { return fBlockSize; }
			btrfs_super_block&	SuperBlock() { return fSuperBlock; }

			status_t			LoadSuperBlock();

			// cache access
			void*				BlockCache() { return fBlockCache; }

	static	status_t			Identify(int fd, btrfs_super_block* superBlock);

			status_t			FindBlock(off_t logical, fsblock_t &physical);
			status_t			FindBlock(off_t logical, off_t &physical);

private:
			mutex				fLock;
			fs_volume*			fFSVolume;
			int					fDevice;
			btrfs_super_block	fSuperBlock;
			char				fName[32];

			uint32				fFlags;
			uint32				fBlockSize;

			void*				fBlockCache;
			Inode*				fRootNode;

			Chunk*				fChunk;
			BPlusTree*			fChunkTree;
			BPlusTree*			fRootTree;
			BPlusTree*			fDevTree;
			BPlusTree*			fExtentTree;
			BPlusTree*			fFSTree;
			BPlusTree*			fChecksumTree;
};

#endif	// VOLUME_H

