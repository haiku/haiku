/*
 * 	Copyright 2008, Salvatore Benedetto, salvatore.benedetto@gmail.com
 * 	Copyright 2003, Tyler Dauwalder, tyler@dauwalder.net
 * 	Copyright 2002, Axel Dörfler, axeld@pinc-software.de
 * 	Distributed under the terms of the MIT License.
 */
#ifndef _UDF_CACHED_BLOCK_H
#define _UDF_CACHED_BLOCK_H

/*! \file CachedBlock.h

	Based on the CachedBlock class from OpenBFS, written by
	Axel Dörfler, axeld@pinc-software.de
*/

#include <fs_cache.h>
#include <util/kernel_cpp.h>

#include "UdfDebug.h"
#include "UdfStructures.h"
#include "Volume.h"

class CachedBlock {
public:
								CachedBlock(Volume *volume);
								CachedBlock(Volume *volume, off_t block);
								CachedBlock(CachedBlock *cached);
								~CachedBlock();

	uint8						*Block() const { return fBlock; }
	off_t						BlockNumber() const { return fBlockNumber; }
	uint32						BlockSize() const { return fVolume->BlockSize(); }
	uint32						BlockShift() const { return fVolume->BlockShift(); }

	inline void					Keep();
	inline void					Unset();

	inline uint8				*SetTo(off_t block);
	inline uint8				*SetTo(off_t block, off_t base, size_t length);
	inline uint8				*SetTo(long_address address);
	template <class Accessor, class Descriptor>
	inline uint8*				SetTo(Accessor &accessor, Descriptor &descriptor);

protected:
	uint8						*fBlock;
	off_t						fBlockNumber;
	Volume						*fVolume;
};


inline
CachedBlock::CachedBlock(Volume *volume)
	:
	fBlock(NULL),
	fBlockNumber(0),
	fVolume(volume)
{
}


inline
CachedBlock::CachedBlock(Volume *volume, off_t block)
	:
	fBlock(NULL),
	fBlockNumber(0),
	fVolume(volume)
{
	SetTo(block);
}


inline 
CachedBlock::CachedBlock(CachedBlock *cached)
	:
	fBlock(cached->fBlock),
	fBlockNumber(cached->BlockNumber()),
	fVolume(cached->fVolume)
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
	if (fBlock) {
		block_cache_put(fVolume->BlockCache(), fBlockNumber);
		fBlock = NULL;
	}
}


inline uint8*
CachedBlock::SetTo(off_t block)
{
	return SetTo(block, block, 1);
}


inline uint8*
CachedBlock::SetTo(off_t block, off_t base, size_t length)
{
	Unset();
	fBlockNumber = block;
	return fBlock = (uint8 *)block_cache_get_etc(fVolume->BlockCache(),
		block, base, length);
}


inline uint8 *
CachedBlock::SetTo(long_address address)
{
	off_t block;
	if (fVolume->MapBlock(address, &block) == B_OK)
		return SetTo(block, block, 1);
	else
		return NULL;
}


template <class Accessor, class Descriptor>
inline uint8*
CachedBlock::SetTo(Accessor &accessor, Descriptor &descriptor)
{
	// Make a long_address out of the descriptor and call it a day
	long_address address;
	address.set_to(accessor.GetBlock(descriptor),
		accessor.GetPartition(descriptor));
    return SetTo(address);
}

#endif	// _UDF_CACHED_BLOCK_H
