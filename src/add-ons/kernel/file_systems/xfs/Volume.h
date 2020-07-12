/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _VOLUME_H_
#define _VOLUME_H_


#include <DeviceOpener.h>

#include "xfs.h"


#define FSBLOCK_SHIFT(fsBlockLog) (fsBlockLog - BASICBLOCKLOG);
#define FSBLOCKS_TO_BASICBLOCKS(fsBlockLog, x) x << FSBLOCK_SHIFT(fsBlockLog);
	// Converting the FS Blocks to Basic Blocks

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

			bool				IsValidSuperBlock() const;
			bool				IsReadOnly() const
									{ return
										(fFlags & VOLUME_READ_ONLY) != 0; }

			dev_t 				ID() const
									{ return fFSVolume ? fFSVolume->id : -1; }
			fs_volume* 			FSVolume() const
									{ return fFSVolume; }
			const char*			Name() const
									{ return fSuperBlock.Name(); }

			XfsSuperBlock&		SuperBlock() { return fSuperBlock; }
			int					Device() const { return fDevice; }

			static	status_t	Identify(int fd, XfsSuperBlock *superBlock);

			uint32				BlockSize() const
									{ return fSuperBlock.BlockSize(); }

			uint8				BlockLog() const
									{ return fSuperBlock.BlockLog(); }

			uint32				DirBlockSize() const
									{ return fSuperBlock.DirBlockSize(); }

			uint32				DirBlockLog() const
									{ return fSuperBlock.DirBlockLog(); }

			uint8				AgInodeBits() const
									{ return fSuperBlock.AgInodeBits(); }

			uint8				AgBlocksLog() const
									{ return fSuperBlock.AgBlocksLog(); }

			uint8				InodesPerBlkLog() const
									{ return fSuperBlock.InodesPerBlkLog(); }

			off_t				Root() const { return fSuperBlock.Root(); }

			uint16				InodeSize() const
									{ return fSuperBlock.InodeSize(); }

			xfs_agnumber_t		AgCount() const
									{ return fSuperBlock.AgCount(); }

			xfs_agblock_t		AgBlocks() const
									{ return fSuperBlock.AgBlocks(); }

			uint8				SuperBlockFlags() const
									{ return fSuperBlock.Flags(); }

			uint32				SuperBlockFeatures2() const
									{ return fSuperBlock.Features2(); }

	#if 0
			off_t				NumBlocks() const
									{ return fSuperBlock.NumBlocks(); }
	#endif

protected:
			fs_volume*			fFSVolume;
			int					fDevice;
			XfsSuperBlock		fSuperBlock;
			char				fName[32];
				// Filesystem name

			uint32				fDeviceBlockSize;
			mutex 				fLock;

			uint32				fFlags;
};

#endif
