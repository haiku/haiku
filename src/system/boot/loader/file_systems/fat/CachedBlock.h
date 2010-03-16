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
		enum {
			READ	= 0x01,
			CLEAR	= 0x02,
			FORCE	= 0x04,
		};

	public:
		CachedBlock(Volume &volume);
		CachedBlock(Volume &volume, off_t block);
		~CachedBlock();

		uint8 *SetTo(off_t block);
		status_t SetTo(off_t blockNumber, uint32 flags);
		status_t Flush();

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


}	// namespace FATFS


#endif	/* CACHED_BLOCK_H */
