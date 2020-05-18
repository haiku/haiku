/*
* Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
* All rights reserved. Distributed under the terms of the MIT License.
*/

#ifndef _VOLUME_H_
#define _VOLUME_H_

#include "xfs.h"


/* Converting the FS Blocks to Basic Blocks */
#define FSBSHIFT(fsBlockLog) (fsBlockLog - BBLOCKLOG);
#define FSB_TO_BB(fsBlockLog, x) x << FSBSHIFT(fsBlockLog);

enum volume_flags {
	VOLUME_READ_ONLY	= 0x0001
};


class Volume {
public:
								Volume(fs_volume *volume);
								~Volume();

			status_t			Mount(const char *device, uint32 flags);
			status_t			Unmount();
			status_t 			Initialize(int fd, const char *label,
									uint32 blockSize, uint32 sectorSize);

			bool				IsValidSuperBlock();
			bool				IsReadOnly() const
									{ return (fFlags & VOLUME_READ_ONLY) != 0; }

			dev_t 				ID() const
									{ return fFSVolume ? fFSVolume->id : -1; }
			fs_volume* 			FSVolume() const
									{ return fFSVolume; }
			char*				Name()
									{ return fSuperBlock.Name(); }

			XfsSuperBlock&		SuperBlock() { return fSuperBlock; }
			int					Device() const { return fDevice; }

			static	status_t	Identify(int fd, XfsSuperBlock *superBlock);

			uint32				BlockSize() { return fSuperBlock.BlockSize(); }

			uint8				BlockLog() { return fSuperBlock.BlockLog(); }

			uint32				DirBlockSize()
									{ return fSuperBlock.DirBlockSize(); }

			uint8				AgInodeBits()
									{ return fSuperBlock.AgInodeBits(); }

			uint8				AgBlocksLog()
									{ return fSuperBlock.AgBlocksLog(); }

			uint8				InodesPerBlkLog()
									{ return fSuperBlock.InodesPerBlkLog(); }

			off_t				Root() const { return fSuperBlock.Root(); }

			uint16				InodeSize() { return fSuperBlock.InodeSize(); }

			xfs_agnumber_t		AgCount() { return fSuperBlock.AgCount(); }

			xfs_agblock_t		AgBlocks() { return fSuperBlock.AgBlocks(); }

	#if 0
			off_t				NumBlocks() const
									{ return fSuperBlock.NumBlocks(); }
	#endif

protected:
			fs_volume*			fFSVolume;
			int					fDevice;
			XfsSuperBlock		fSuperBlock;
			char				fName[32];	/* filesystem name */

			uint32				fDeviceBlockSize;
			mutex 				fLock;

			uint32				fFlags;
};

#endif
