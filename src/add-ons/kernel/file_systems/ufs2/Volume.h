/*
 * Copyright 2020 Suhel Mehta, mehtasuhel@gmail.com
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef VOLUME_H
#define VOLUME_H

#include "ufs2.h"

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
				bool					IsReadOnly() const;
				fs_volume*				FSVolume() const { return fFSVolume; }
				ufs2_super_block&		SuperBlock() { return fSuperBlock; }
				status_t				LoadSuperBlock();
		static	status_t				Identify(int fd, ufs2_super_block* superBlock);

	private:
				fs_volume*				fFSVolume;
				ufs2_super_block		fSuperBlock;
};

#endif	// VOLUME_H
