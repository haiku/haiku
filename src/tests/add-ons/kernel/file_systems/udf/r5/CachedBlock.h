//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//
//  Copyright (c) 2003 Tyler Dauwalder, tyler@dauwalder.net
//  Based on the CachedBlock class from OpenBFS,
//  Copyright (c) 2002 Axel Dörfler, axeld@pinc-software.de
//---------------------------------------------------------------------
#ifndef _UDF_CACHED_BLOCK_H
#define _UDF_CACHED_BLOCK_H

/*! \file CachedBlock.h

	Based on the CachedBlock class from OpenBFS, written by
	Axel Dörfler, axeld@pinc-software.de
*/

#ifdef COMPILE_FOR_R5
extern "C" {
#endif
	#include "fsproto.h"
#ifdef COMPILE_FOR_R5
}
#endif

extern "C" {
	#ifndef _IMPEXP_KERNEL
	#	define _IMPEXP_KERNEL
	#endif
	
	#include "lock.h"
	#include "cache.h"
}

#include "kernel_cpp.h"
#include "UdfDebug.h"

#include "UdfStructures.h"
#include "Volume.h"

namespace Udf {

class CachedBlock {
	public:
		CachedBlock(Volume *volume);
		CachedBlock(Volume *volume, off_t block, bool empty = false);
		CachedBlock(CachedBlock *cached);
		~CachedBlock();

		inline void Keep();
		inline void Unset();
		inline uint8 *SetTo(off_t block, bool empty = false);
		inline uint8 *SetTo(long_address address, bool empty = false);
		template <class Accessor, class Descriptor>
			inline uint8* SetTo(Accessor &accessor, Descriptor &descriptor,
			bool empty = false);

		uint8 *Block() const { return fBlock; }
		off_t BlockNumber() const { return fBlockNumber; }
		uint32 BlockSize() const { return fVolume->BlockSize(); }
		uint32 BlockShift() const { return fVolume->BlockShift(); }

	private:
		CachedBlock(const CachedBlock &);				// unimplemented
		CachedBlock &operator=(const CachedBlock &);	// unimplemented

	protected:
		Volume	*fVolume;
		off_t	fBlockNumber;
		uint8	*fBlock;
};

inline
CachedBlock::CachedBlock(Volume *volume)
	:
	fVolume(volume),
	fBlock(NULL)
{
}

inline
CachedBlock::CachedBlock(Volume *volume, off_t block, bool empty = false)
	:
	fVolume(volume),
	fBlock(NULL)
{
	SetTo(block, empty);
}

inline 
CachedBlock::CachedBlock(CachedBlock *cached)
	: fVolume(cached->fVolume)
	, fBlockNumber(cached->BlockNumber())
	, fBlock(cached->fBlock)
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
	DEBUG_INIT("CachedBlock");
	if (fBlock) {
		PRINT(("releasing block #%Ld\n", BlockNumber()));	
		release_block(fVolume->Device(), fBlockNumber);
	} else {
		PRINT(("no block to release\n"));
	}
}

inline uint8 *
CachedBlock::SetTo(off_t block, bool empty = false)
{
	DEBUG_INIT_ETC("CachedBlock", ("block: %Ld, empty: %s",
	               block, (empty ? "true" : "false")));
	Unset();
	fBlockNumber = block;
	PRINT(("getting block #%Ld\n", block));
	return fBlock = empty ? (uint8 *)get_empty_block(fVolume->Device(), block, BlockSize())
						  : (uint8 *)get_block(fVolume->Device(), block, BlockSize());
}

inline uint8 *
CachedBlock::SetTo(long_address address, bool empty = false)
{
	off_t block;
	if (fVolume->MapBlock(address, &block) == B_OK)
		return SetTo(block, empty);
	else
		return NULL;
}

template <class Accessor, class Descriptor>
inline uint8*
CachedBlock::SetTo(Accessor &accessor, Descriptor &descriptor, bool empty = false)
{
	// Make a long_address out of the descriptor and call it a day
	long_address address;
	address.set_to(accessor.GetBlock(descriptor),
                             accessor.GetPartition(descriptor));
    return SetTo(address, empty);
}

};	// namespace Udf

#endif	// _UDF_CACHED_BLOCK_H

