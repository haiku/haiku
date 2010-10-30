/*
 * Copyright 2001-2008, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef CACHED_BLOCK_H
#define CACHED_BLOCK_H

//!	interface for the block cache

#include <fs_cache.h>

#include "Transaction.h"
#include "Volume.h"


class CachedBlock {
public:
							CachedBlock(Volume* volume);
							CachedBlock(Volume* volume, off_t block);
							~CachedBlock();

			void			Keep();
			void			Unset();

			const uint8*	SetTo(off_t block);
			uint8*			SetToWritable(Transaction& transaction, 
								off_t block, bool empty = false);
			uint8*			SetToWritableWithoutTransaction(off_t block,
								bool empty = false);

			const uint8*	Block() const { return fBlock; }
			off_t			BlockNumber() const { return fBlockNumber; }

private:
							CachedBlock(const CachedBlock &);
							CachedBlock &operator=(const CachedBlock &);
								// no implementation
						
			uint8*			_SetToWritableEtc(int32 transaction, off_t block,
								bool empty);

protected:
			Volume*			fVolume;
			off_t			fBlockNumber;
			uint8*			fBlock;
};


// inlines


inline
CachedBlock::CachedBlock(Volume* volume)
	:
	fVolume(volume),
	fBlockNumber(0),
	fBlock(NULL)
{
}


inline
CachedBlock::CachedBlock(Volume* volume, off_t block)
	:
	fVolume(volume),
	fBlockNumber(0),
	fBlock(NULL)
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


inline const uint8 *
CachedBlock::SetTo(off_t block)
{
	Unset();
	fBlockNumber = block;
	return fBlock = (uint8 *)block_cache_get(fVolume->BlockCache(), block);
}


inline uint8*
CachedBlock::SetToWritable(Transaction& transaction, off_t block, bool empty)
{
	return _SetToWritableEtc(transaction.ID(), block, empty);
}


inline uint8*
CachedBlock::SetToWritableWithoutTransaction(off_t block, bool empty)
{
	return _SetToWritableEtc((int32)-1, block, empty);
}

inline uint8*
CachedBlock::_SetToWritableEtc(int32 transaction, off_t block, bool empty)
{
	Unset();
	fBlockNumber = block;

	if (empty) {
		fBlock = (uint8*)block_cache_get_empty(fVolume->BlockCache(),
			block, transaction);
	} else {
		fBlock = (uint8*)block_cache_get_writable(fVolume->BlockCache(),
			block, transaction);
	}

	return fBlock;
}

#endif	// CACHED_BLOCK_H
