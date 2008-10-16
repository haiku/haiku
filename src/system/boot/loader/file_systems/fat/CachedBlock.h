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
#include <stdlib.h>
#include <unistd.h>

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


inline void
CachedBlock::Unset()
{
	fBlockNumber = -1;
}


inline uint8 *
CachedBlock::SetTo(off_t block)
{
	if (block == fBlockNumber)
		return fBlock;
	if (fBlock == NULL) {
		fBlock = (uint8 *)malloc(BlockSize());
		if (fBlock == NULL)
			return NULL;
	}

	fBlockNumber = block;
	if (read_pos(fVolume.Device(), block << BlockShift(), fBlock, BlockSize()) < (ssize_t)BlockSize())
		return NULL;

	return fBlock;
}


}	// namespace FATFS


#endif	/* CACHED_BLOCK_H */
