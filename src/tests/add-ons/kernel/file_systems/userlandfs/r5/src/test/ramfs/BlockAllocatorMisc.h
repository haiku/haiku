// BlockAllocatorMisc.h

#ifndef BLOCK_ALLOCATOR_MISC_H
#define BLOCK_ALLOCATOR_MISC_H

#include "Block.h"
#include "Misc.h"

// block alignment -- start offsets and size
static const size_t kBlockAlignment = 4;	// must be a power of 2

// block_align_{floor,ceil}
static inline size_t block_align_floor(size_t value)
	{ return value & ~(kBlockAlignment - 1); }
static inline size_t block_align_ceil(size_t value)
	{ return (value + kBlockAlignment - 1) & ~(kBlockAlignment - 1); }

// minimal size of a gross/net block
// BAD DOG: No initializers in the kernel!
//static const size_t kMinBlockSize = block_align_ceil(sizeof(TFreeBlock));
#define kMinBlockSize	(block_align_ceil(sizeof(TFreeBlock)))
static const size_t kMinNetBlockSize = 8;

static const size_t kDefragmentingTolerance = 10240;

// bucket_containing_size -- bucket for to contain an area with size
static inline int bucket_containing_size(size_t size)
	{ return fls(size / kMinNetBlockSize) + 1; }

// bucket_containing_min_size -- bucket containing areas >= size
static inline int
bucket_containing_min_size(size_t size)
	{ return (size ? bucket_containing_size(size - 1) + 1 : 0); }


#endif // BLOCK_ALLOCATOR_MISC_H
