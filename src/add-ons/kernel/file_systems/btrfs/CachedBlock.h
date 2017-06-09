/*
 * Copyright 2001-2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef CACHED_BLOCK_H
#define CACHED_BLOCK_H

//!	interface for the block cache


#include "Volume.h"


//#define TRACE_BTRFS
#ifdef TRACE_BTRFS
#	define TRACE(x...) dprintf("\33[34mbtrfs:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif


class CachedBlock {
public:
							CachedBlock(Volume* volume);
							CachedBlock(Volume* volume, off_t block);
							~CachedBlock();

			void			Keep();
			void			Unset();

			const uint8*	SetTo(off_t block);
			uint8*			SetToWritable(off_t block, int32 transactionId,
								bool empty);

			const uint8*	Block() const { return fBlock; }
			off_t			BlockNumber() const { return fBlockNumber; }
			bool			IsWritable() const { return fWritable; }

private:
							CachedBlock(const CachedBlock&);
							CachedBlock& operator=(const CachedBlock&);
								// no implementation

protected:
			Volume*			fVolume;
			off_t			fBlockNumber;
			uint8*			fBlock;
			bool			fWritable;
};


// inlines


inline
CachedBlock::CachedBlock(Volume* volume)
	:
	fVolume(volume),
	fBlockNumber(0),
	fBlock(NULL),
	fWritable(false)
{
}


inline
CachedBlock::CachedBlock(Volume* volume, off_t block)
	:
	fVolume(volume),
	fBlockNumber(0),
	fBlock(NULL),
	fWritable(false)
{
	SetTo(block);
}


inline
CachedBlock::~CachedBlock()
{
	Unset();
}


inline void
CachedBlock::Keep()
{
	fBlock = NULL;
}


inline void
CachedBlock::Unset()
{
	if (fBlock != NULL) {
		block_cache_put(fVolume->BlockCache(), fBlockNumber);
		fBlock = NULL;
	}
}


inline const uint8*
CachedBlock::SetTo(off_t block)
{
	Unset();
	fBlockNumber = block;
	return fBlock = (uint8*)block_cache_get(fVolume->BlockCache(), block);
}


inline uint8*
CachedBlock::SetToWritable(off_t block, int32 transactionId, bool empty)
{
	Unset();
	fBlockNumber = block;
	fWritable = true;
	if (empty) {
		fBlock = (uint8*)block_cache_get_empty(fVolume->BlockCache(),
			block, transactionId);
	} else {
		fBlock = (uint8*)block_cache_get_writable(fVolume->BlockCache(),
			block, transactionId);
	}

	return fBlock;
}


#endif	// CACHED_BLOCK_H
