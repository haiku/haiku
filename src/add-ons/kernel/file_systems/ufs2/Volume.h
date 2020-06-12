/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef VOLUME_H
#define VOLUME_H

#include "ufs2.h"

extern fs_volume_ops gUfs2VolumeOps;
extern fs_vnode_ops gufs2VnodeOps;

enum volume_flags {
	VOLUME_READ_ONLY	= 0x0001
};

class Inode;

class Volume {
	public:
								Volume(fs_volume* volume);
								~Volume();

		status_t				Mount(const char* device, uint32 flags);
		status_t				Unmount();
		status_t				Initialize(int fd, const char* name,
									uint32 blockSize, uint32 flags);

		bool					IsValidSuperBlock();
		bool					IsValidInodeBlock(off_t block) const;
		bool					IsReadOnly() const
								{ return (fFlags & VOLUME_READ_ONLY) != 0; }
		const char*				Name() const;
		fs_volume*				FSVolume() const { return fFSVolume; }
		int						Device() const { return fDevice; }
		dev_t					ID() const
								{ return fFSVolume ? fFSVolume->id : -1; }
		ufs2_super_block&		SuperBlock() { return fSuperBlock; }
		status_t				LoadSuperBlock();
static	status_t				Identify(int fd, ufs2_super_block* superBlock);

	private:
		fs_volume*			fFSVolume;
		int 				fDevice;
		ufs2_super_block	fSuperBlock;
		mutex				fLock;
		uint32				fFlags;
		Inode*				fRootNode;
		char				fName[32];
};

#endif	// VOLUME_H
