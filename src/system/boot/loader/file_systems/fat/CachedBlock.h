/*
 * Copyright 2008, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		François Revol <revol@free.fr>
 */
#ifndef CACHED_BLOCK_H
#define CACHED_BLOCK_H

#include <SupportDefs.h>
#include <sys/stat.h>

#include "Volume.h"

namespace FATFS {

class CachedBlock {
	public:
		CachedBlock(Volume &volume);
		CachedBlock(Volume &volume, off_t block);
		~CachedBlock();

		uint8 *SetTo(off_t block);

		void Unset();

		uint8 *Block() const { return fBlock; }
		off_t BlockNumber() const { return fBlockNumber; }
		uint32 BlockSize() const { return fVolume.BlockSize(); }
		uint32 BlockShift() const { return fVolume.BlockShift(); }

	private:
		Volume	&fVolume;
		off_t	fBlockNumber;
		uint8	*fBlock;
};

}	// namespace FATFS

#endif	/* CACHED_BLOCK_H */
