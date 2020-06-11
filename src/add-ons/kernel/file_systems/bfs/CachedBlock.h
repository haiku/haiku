/*
 * Copyright 2001-2020, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */
#ifndef CACHED_BLOCK_H
#define CACHED_BLOCK_H


//!	Interface for the block cache


#include "system_dependencies.h"

#include "Volume.h"
#include "Journal.h"
#include "Debug.h"


// The CachedBlock class is completely implemented as inlines.
// It should be used when cache single blocks to make sure they
// will be properly released after use (and it's also very
// convenient to use them).

class CachedBlock {
public:
								CachedBlock(Volume* volume);
								CachedBlock(CachedBlock* cached);
								~CachedBlock();

	inline	void				Keep();
	inline	void				Unset();

	inline status_t				SetTo(off_t block, off_t base, size_t length);
	inline status_t				SetTo(off_t block);
	inline status_t				SetTo(block_run run);
	inline status_t				SetToWritable(Transaction& transaction,
									off_t block, off_t base, size_t length,
									bool empty = false);
	inline status_t				SetToWritable(Transaction& transaction,
									off_t block, bool empty = false);
	inline status_t				SetToWritable(Transaction& transaction,
									block_run run, bool empty = false);
	inline status_t				MakeWritable(Transaction& transaction);

			const uint8*		Block() const { return fBlock; }
			uint8*				WritableBlock() const { return fBlock; }
			off_t				BlockNumber() const { return fBlockNumber; }
			uint32				BlockSize() const
									{ return fVolume->BlockSize(); }
			uint32				BlockShift() const
									{ return fVolume->BlockShift(); }

private:
	CachedBlock(const CachedBlock& other);
	CachedBlock& operator=(const CachedBlock& other);
		// no implementation

protected:
			Volume*				fVolume;
			off_t				fBlockNumber;
			uint8*				fBlock;
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
CachedBlock::CachedBlock(CachedBlock* cached)
	:
	fVolume(cached->fVolume),
	fBlockNumber(cached->BlockNumber()),
	fBlock(cached->fBlock)
{
	cached->Keep();
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


inline status_t
CachedBlock::SetTo(off_t block, off_t base, size_t length)
{
	Unset();
	fBlockNumber = block;
	return block_cache_get_etc(fVolume->BlockCache(), block, base, length,
		(const void**)&fBlock);
}


inline status_t
CachedBlock::SetTo(off_t block)
{
	return SetTo(block, block, 1);
}


inline status_t
CachedBlock::SetTo(block_run run)
{
	return SetTo(fVolume->ToBlock(run));
}


inline status_t
CachedBlock::SetToWritable(Transaction& transaction, off_t block, off_t base,
	size_t length, bool empty)
{
	Unset();
	fBlockNumber = block;

	if (empty) {
		fBlock = (uint8*)block_cache_get_empty(fVolume->BlockCache(),
			block, transaction.ID());
		return fBlock != NULL ? B_OK : B_NO_MEMORY;
	}

	return block_cache_get_writable_etc(fVolume->BlockCache(),
		block, base, length, transaction.ID(), (void**)&fBlock);
}


inline status_t
CachedBlock::SetToWritable(Transaction& transaction, off_t block, bool empty)
{
	return SetToWritable(transaction, block, block, 1, empty);
}


inline status_t
CachedBlock::SetToWritable(Transaction& transaction, block_run run, bool empty)
{
	return SetToWritable(transaction, fVolume->ToBlock(run), empty);
}


inline status_t
CachedBlock::MakeWritable(Transaction& transaction)
{
	if (fBlock == NULL)
		return B_NO_INIT;

	return block_cache_make_writable(fVolume->BlockCache(), fBlockNumber,
		transaction.ID());
}


#endif	// CACHED_BLOCK_H
