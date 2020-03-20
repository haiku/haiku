/*
 * Copyright 2020, Shubham Bhagat, shubhambhagat111@yahoo.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef _VOLUME_H_
#define _VOLUME_H_

#include "xfs.h"

extern fs_volume_ops gxfsVolumeOps;
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

	static	status_t			Identify(int fd, XfsSuperBlock *superBlock);

	#if 0
			off_t				NumBlocks() const
									{ return fSuperBlock.NumBlocks(); }

			off_t				Root() const { return fSuperBlock.rootino; }

	static	status_t			Identify(int fd, SuperBlock *superBlock);
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
