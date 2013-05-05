/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "BlockAllocator.h"

#include <string.h>

#include <algorithm>

#include <util/AutoLock.h>

#include "Block.h"
#include "checksumfs.h"
#include "DebugSupport.h"
#include "SuperBlock.h"
#include "Volume.h"


// This is a simple block allocator implementation. It manages the on-disk
// block bitmap structures. Those consist of:
// * The block bitmap itself: A contiguous run of blocks with one bit for each
//   of the blocks accessible by the file system, indicating whether the block
//   is used or free. The bit is set for a used, clear for a free block. For
//   faster access the bits are grouped in 32 bit integers (host endian ATM).
// * The allocation groups blocks: An allocation group consists of the blocks
//   that 2048 block bitmap blocks refer to. Each allocation group is
//   represented by a single block which contains 2048 16 bit numbers (host
//   endian ATM), each of which referring to a block bitmap block and encoding
//   how many free blocks the block bitmap block refers to.
// The blocks for the allocation groups are directly followed by the block
// bitmap blocks. The blocks beyond the blocks accessible by the file system
// that the last block bitmap block may refer to are marked used. Non-existing
// block bitmap blocks referred to by the last allocation group block are set
// to have 0 free blocks.


static const uint32 kBlocksPerBitmapBlock	= B_PAGE_SIZE * 8;
static const uint32 kBitmapBlocksPerGroup	= B_PAGE_SIZE / 2;
static const uint32 kBlocksPerGroup
	= kBitmapBlocksPerGroup * kBlocksPerBitmapBlock;


BlockAllocator::BlockAllocator(Volume* volume)
	:
	fVolume(volume),
	fTotalBlocks(volume->TotalBlocks()),
	fBitmapBlockCount((fTotalBlocks + B_PAGE_SIZE - 1) / B_PAGE_SIZE)
{
	fAllocationGroupCount = (fBitmapBlockCount + kBitmapBlocksPerGroup - 1)
		/ kBitmapBlocksPerGroup;
	mutex_init(&fLock, "checksumfs block allocator");
}


BlockAllocator::~BlockAllocator()
{
	mutex_destroy(&fLock);
}


status_t
BlockAllocator::Init(uint64 blockBitmap, uint64 freeBlocks)
{
	if (blockBitmap <= kCheckSumFSSuperBlockOffset / B_PAGE_SIZE
		|| blockBitmap >= fTotalBlocks
		|| fTotalBlocks - blockBitmap < fAllocationGroupCount) {
		return B_BAD_DATA;
	}

	fFreeBlocks = freeBlocks;
	fAllocationGroupBlock = blockBitmap;
	fBitmapBlock = blockBitmap + fAllocationGroupCount;

	return B_OK;
}


status_t
BlockAllocator::Initialize(Transaction& transaction)
{
	status_t error = Init(kCheckSumFSSuperBlockOffset / B_PAGE_SIZE + 1,
		fTotalBlocks);
	if (error != B_OK)
		return error;

	PRINT("BlockAllocator::Initialize():\n");
	PRINT("  fTotalBlocks:          %" B_PRIu64 "\n", fTotalBlocks);
	PRINT("  fFreeBlocks:           %" B_PRIu64 "\n", fFreeBlocks);
	PRINT("  fAllocationGroupBlock: %" B_PRIu64 "\n", fAllocationGroupBlock);
	PRINT("  fAllocationGroupCount: %" B_PRIu64 "\n", fAllocationGroupCount);
	PRINT("  fBitmapBlock:          %" B_PRIu64 "\n", fBitmapBlock);
	PRINT("  fBitmapBlockCount:     %" B_PRIu64 "\n", fBitmapBlockCount);

	// clear the block bitmap
	for (uint64 i = 0; i < fBitmapBlockCount; i++) {
		Block block;
		if (!block.GetZero(fVolume, fBitmapBlock + i, transaction))
			return B_ERROR;
	}

	// the last block of the block bitmap may be partial -- mark the blocks
	// beyond the end used
	uint32 partialBitmapBlock = fTotalBlocks % kBlocksPerBitmapBlock;
	if (partialBitmapBlock != 0) {
		Block block;
		if (!block.GetZero(fVolume, fBitmapBlock + fBitmapBlockCount - 1,
				transaction)) {
			return B_ERROR;
		}

		// set full uint32s
		uint32* bits = (uint32*)block.Data();
		uint32 offset = (partialBitmapBlock + 31) / 32;
		if (partialBitmapBlock <= B_PAGE_SIZE - 4)
			memset(bits + offset, 0xff, B_PAGE_SIZE - offset * 4);

		// set the partial uint32
		uint32 bitOffset = partialBitmapBlock % 32;
		if (bitOffset != 0)
			bits[offset] = ~(((uint32)1 << bitOffset) - 1);
	}

	// init the allocation groups
	uint32 partialGroup = fTotalBlocks % kBlocksPerGroup;
	for (uint64 i = 0; i < fAllocationGroupCount; i++) {
		Block block;
		if (!block.GetZero(fVolume, fAllocationGroupBlock + i, transaction))
			return B_ERROR;

		uint16* counts = (uint16*)block.Data();

		if (i < fAllocationGroupCount - 1 || partialGroup == 0) {
			// not the last group or the last group is not partial -- all
			// blocks are free
			for (uint32 i = 0; i < kBitmapBlocksPerGroup; i++)
				counts[i] = kBlocksPerBitmapBlock;
		} else {
			// the last, partial group
			if (partialGroup != 0) {
				uint32 offset = partialGroup / kBlocksPerBitmapBlock;
				for (uint32 i = 0; i < offset; i++)
					counts[i] = kBlocksPerBitmapBlock;
				counts[offset] = partialBitmapBlock;
			}
		}
	}

	// mark all blocks we already use used
	error = AllocateExactly(0, fBitmapBlock + fBitmapBlockCount, transaction);
	if (error != B_OK)
		return error;

	PRINT("BlockAllocator::Initialize() done:\n");
	PRINT("  fFreeBlocks:           %" B_PRIu64 "\n", fFreeBlocks);

	return B_OK;
}


status_t
BlockAllocator::Allocate(uint64 baseHint, uint64 count,
	Transaction& transaction, uint64& _allocatedBase, uint64& _allocatedCount)
{
	MutexLocker locker(fLock);

	PRINT("BlockAllocator::Allocate(%" B_PRIu64 ", %" B_PRIu64 ")\n", baseHint,
		count);

	if (fFreeBlocks == 0)
		return B_DEVICE_FULL;

	if (baseHint >= fTotalBlocks)
		baseHint = 0;

	// search from base hint to end
	status_t error = _Allocate(baseHint, fTotalBlocks, count, transaction,
		&_allocatedBase, _allocatedCount);
	if (error == B_OK)
		return _UpdateSuperBlock(transaction);
	if (baseHint == 0)
		return error;

	// search from 0 to hint
	error = _Allocate(0, baseHint, count, transaction, &_allocatedBase,
		_allocatedCount);
	if (error != B_OK)
		return error;

	return _UpdateSuperBlock(transaction);
}


status_t
BlockAllocator::AllocateExactly(uint64 base, uint64 count,
	Transaction& transaction)
{
	MutexLocker locker(fLock);

	PRINT("BlockAllocator::AllocateExactly(%" B_PRIu64 ", %" B_PRIu64 ")\n",
		base, count);

	uint64 allocated;
	status_t error = _Allocate(base, fTotalBlocks, count, transaction, NULL,
		allocated);
	if (error != B_OK)
		return error;

	if (allocated < count)
		return B_BUSY;

	return _UpdateSuperBlock(transaction);
}


status_t
BlockAllocator::Free(uint64 base, uint64 count, Transaction& transaction)
{
	MutexLocker locker(fLock);

	status_t error = _Free(base, count, transaction);
	if (error != B_OK)
		return error;

	return _UpdateSuperBlock(transaction);
}


void
BlockAllocator::ResetFreeBlocks(uint64 count)
{
	MutexLocker locker(fLock);

	fFreeBlocks = count;
}


/*!	Allocates contiguous blocks.

	Might allocate fewer block than requested. If no block could be allocated
	at all, an error is returned.
	If \a _allocatedBase is not \c NULL, the method may move the base up, if it
	isn't able to allocate anything at the given base.

	\param base The first potential block to allocate.
	\param searchEnd A hint for the method where to stop searching for free
		blocks.
	\param count The maximum number of blocks to allocate.
	\param _allocatedBase If not \c NULL, the may allocate at a greater base.
		The base of the actual allocation is returned via this variable.
	\param _allocatedCount On success the variable will be set to the number of
		blocks actually allocated.
	\return \c B_OK, if one or more blocks could be allocated, another error
		code otherwise.
*/
status_t
BlockAllocator::_Allocate(uint64 base, uint64 searchEnd, uint64 count,
	Transaction& transaction, uint64* _allocatedBase, uint64& _allocatedCount)
{
	ASSERT(base <= fTotalBlocks);
	ASSERT(searchEnd <= fTotalBlocks);

	if (base >= searchEnd || fFreeBlocks == 0)
		RETURN_ERROR(B_BUSY);

	uint64 groupOffset = base % kBlocksPerGroup;
	uint64 remaining = count;

	// If we're allowed to move the base, loop until we allocate something.
	if (_allocatedBase != NULL) {
		while (count > 0 && base < searchEnd) {
			uint64 toAllocate = std::min(count, kBlocksPerGroup - groupOffset);

			uint32 allocated;
			status_t error = _AllocateInGroup(base, searchEnd, toAllocate,
				transaction, _allocatedBase, allocated);
			if (error == B_OK) {
				fFreeBlocks -= toAllocate;

				if (allocated == toAllocate) {
					_allocatedCount = allocated;
					return B_OK;
				}

				count = std::min(count,
					searchEnd - (uint32)*_allocatedBase % kBlocksPerGroup);
				_allocatedBase = NULL;
				remaining = count - allocated;

				break;
			}

			// nothing yet -- continue with the next group
			count -= toAllocate;
			base += toAllocate;
			groupOffset = 0;
		}

		if (_allocatedBase != NULL)
			return B_BUSY;
	}

	// We're not/no longer allowed to move the base. Loop as long as we can
	// allocate what ask for.
	while (remaining > 0 && base < searchEnd) {
		uint64 toAllocate = std::min(remaining, kBlocksPerGroup - groupOffset);
		uint32 allocated;
		status_t error = _AllocateInGroup(base, searchEnd, toAllocate,
			transaction, NULL, allocated);
		if (error != B_OK)
			break;

		fFreeBlocks -= toAllocate;
		remaining -= allocated;

		if (allocated < toAllocate)
			break;

		base += toAllocate;
		groupOffset = 0;
	}

	if (remaining == count)
		RETURN_ERROR(B_BUSY);

	_allocatedCount = count - remaining;
	return B_OK;
}


/*!	Allocates contiguous blocks in an allocation group.

	The range specified by \a base and \a count must lie fully within a single
	allocation group.

	Might allocate fewer block than requested. If no block could be allocated
	at all, an error is returned.
	If \a _allocatedBase is not \c NULL, the method may move the base up, if it
	isn't able to allocate anything at the given base.

	\param base The first potential block to allocate.
	\param searchEnd A hint for the method where to stop searching for free
		blocks.
	\param count The maximum number of blocks to allocate.
	\param _allocatedBase If not \c NULL, the may allocate at a greater base.
		The base of the actual allocation is returned via this variable.
	\param _allocatedCount On success the variable will be set to the number of
		blocks actually allocated.
	\return \c B_OK, if one or more blocks could be allocated, another error
		code otherwise.
*/
status_t
BlockAllocator::_AllocateInGroup(uint64 base, uint64 searchEnd, uint32 count,
	Transaction& transaction, uint64* _allocatedBase, uint32& _allocatedCount)
{
	PRINT("BlockAllocator::_AllocateInGroup(%" B_PRIu64 ", %" B_PRIu32 ")\n",
		base, count);

	ASSERT(count <= kBlocksPerGroup);
	ASSERT(base % kBlocksPerGroup + count <= kBlocksPerGroup);

	if (base >= searchEnd)
		RETURN_ERROR(B_BUSY);

	Block block;
	if (!block.GetWritable(fVolume,
			fAllocationGroupBlock + base / kBlocksPerGroup, transaction)) {
		RETURN_ERROR(B_ERROR);
	}

	uint16* counts = (uint16*)block.Data();

	uint32 blockIndex = base / kBlocksPerBitmapBlock % kBitmapBlocksPerGroup;
	uint64 inBlockOffset = base % kBlocksPerBitmapBlock;
	uint64 remaining = count;

	// If we're allowed to move the base, skip used blocks.
	if (_allocatedBase != NULL) {
		// check partial block
		if (inBlockOffset != 0) {
			if (counts[blockIndex] > 0) {
				uint32 allocated;
				if (_AllocateInBitmapBlock(base, count, transaction,
						_allocatedBase, allocated) == B_OK) {
					counts[blockIndex] -= allocated;

					if (inBlockOffset + allocated < kBlocksPerBitmapBlock
						|| allocated == remaining) {
						_allocatedCount = allocated;
						return B_OK;
					}

					count = std::min(count,
						kBlocksPerGroup
							- (uint32)*_allocatedBase % kBlocksPerGroup);
					_allocatedBase = NULL;
					remaining = count - allocated;
				}
			}

			base += kBlocksPerBitmapBlock - inBlockOffset;
			inBlockOffset = 0;
			blockIndex++;
		}

		// skip completely used blocks
		if (_allocatedBase != NULL) {
			while (blockIndex < kBitmapBlocksPerGroup && base < searchEnd) {
				if (counts[blockIndex] > 0)
					break;

				base += kBlocksPerBitmapBlock;
				blockIndex++;
			}
		}

		if (blockIndex == kBitmapBlocksPerGroup)
			return B_BUSY;

		// Clamp the count to allocate, if we have moved the base too far. Do
		// this only, if we haven't allocated anything so far.
		if (_allocatedBase != NULL) {
			count = std::min(count,
				kBlocksPerGroup - (uint32)base % kBlocksPerGroup);
			remaining = count;
		}
	}

	// Allocate as many of the requested blocks as we can.
	while (remaining > 0 && base < searchEnd) {
		if (counts[blockIndex] == 0)
			break;

		uint32 toAllocate = std::min(remaining,
			kBlocksPerBitmapBlock - inBlockOffset);

		uint32 allocated;
		status_t error = _AllocateInBitmapBlock(base, toAllocate, transaction,
			_allocatedBase, allocated);
		if (error != B_OK)
			break;

		counts[blockIndex] -= allocated;
		remaining -= allocated;

		if (allocated < toAllocate)
			break;

		base += allocated;
		blockIndex++;
		inBlockOffset = 0;
		_allocatedBase = NULL;
	}

	if (remaining == count)
		return B_BUSY;

	_allocatedCount = count - remaining;
	return B_OK;
}


/*!	Allocates contiguous blocks in a bitmap block.

	The range specified by \a base and \a count must lie fully within a single
	bitmap block.

	Might allocate fewer block than requested. If no block could be allocated
	at all, an error is returned.
	If \a _allocatedBase is not \c NULL, the method may move the base up, if it
	isn't able to allocate anything at the given base.

	\param base The first potential block to allocate.
	\param count The maximum number of blocks to allocate.
	\param _allocatedBase If not \c NULL, the may allocate at a greater base.
		The base of the actual allocation is returned via this variable.
	\param _allocatedCount On success the variable will be set to the number of
		blocks actually allocated.
	\return \c B_OK, if one or more blocks could be allocated, another error
		code otherwise.
*/
status_t
BlockAllocator::_AllocateInBitmapBlock(uint64 base, uint32 count,
	Transaction& transaction, uint64* _allocatedBase, uint32& _allocatedCount)
{
	PRINT("BlockAllocator::_AllocateInBitmapBlock(%" B_PRIu64 ", %" B_PRIu32
		")\n", base, count);

	ASSERT(count <= kBlocksPerBitmapBlock);
	ASSERT(base % kBlocksPerBitmapBlock + count <= kBlocksPerBitmapBlock);

	Block block;
	if (!block.GetWritable(fVolume,
			fBitmapBlock + base / kBlocksPerBitmapBlock, transaction)) {
		RETURN_ERROR(B_ERROR);
	}

	uint32* bits = (uint32*)block.Data()
		+ base % kBlocksPerBitmapBlock / 32;
	uint32* const bitsEnd = (uint32*)block.Data() + kBlocksPerBitmapBlock / 32;
	uint32 bitOffset = base % 32;

	// If we're allowed to move the base, skip used blocks.
	if (_allocatedBase != NULL) {
		// check partial uint32 at the beginning
		bool foundBase = false;
		if (bitOffset > 0) {
			uint32 mask = ~(((uint32)1 << bitOffset) - 1);
			if ((*bits & mask) != mask) {
				while ((*bits & ((uint32)1 << bitOffset)) != 0) {
					bitOffset++;
					base++;
				}
				foundBase = true;
			} else {
				// all used -- skip
				bits++;
				base += 32 - bitOffset;
				bitOffset = 0;
			}
		}

		// check complete uint32s
		if (!foundBase) {
			while (bits < bitsEnd) {
				if (*bits != 0xffffffff) {
					bitOffset = 0;
					while ((*bits & ((uint32)1 << bitOffset)) != 0) {
						bitOffset++;
						base++;
					}
					foundBase = true;
					break;
				}

				bits++;
				base += 32;
			}
		}

		if (!foundBase)
			return B_BUSY;

		// Clamp the count to allocate, if we have moved the base too far.
		if (base % kBlocksPerBitmapBlock + count > kBlocksPerBitmapBlock)
			count = kBlocksPerBitmapBlock - base % kBlocksPerBitmapBlock;
	}

	// Allocate as many of the requested blocks as we can, starting at the base
	// we have.
	uint32 remaining = count;

	while (remaining > 0 && bits < bitsEnd) {
		PRINT("  remaining: %" B_PRIu32 ", index: %" B_PRIu32 ".%" B_PRIu32
			", bits: %#" B_PRIx32 "\n", remaining,
			kBlocksPerBitmapBlock - (bitsEnd - bits), bitOffset, *bits);

		// TODO: Not particularly efficient for large allocations.
		uint32 endOffset = std::min(bitOffset + remaining, (uint32)32);
		for (; bitOffset < endOffset; bitOffset++) {
			if ((*bits & ((uint32)1 << bitOffset)) != 0) {
				bits = bitsEnd;
				break;
			}

			*bits |= (uint32)1 << bitOffset;
			remaining--;
		}

		bits++;
		bitOffset = 0;
	}

	if (remaining == count)
		RETURN_ERROR(B_BUSY);

	_allocatedCount = count - remaining;
	if (_allocatedBase != NULL)
		*_allocatedBase = base;

	return B_OK;
}


status_t
BlockAllocator::_Free(uint64 base, uint64 count, Transaction& transaction)
{
	if (count == 0)
		return B_OK;

	PRINT("BlockAllocator::_Free(%" B_PRIu64 ", %" B_PRIu64 ")\n", base, count);

	ASSERT(count <= fTotalBlocks - fFreeBlocks);
	ASSERT(base < fTotalBlocks && fTotalBlocks - base >= count);

	uint64 groupOffset = base % kBlocksPerGroup;
	uint64 remaining = count;

	while (remaining > 0) {
		uint64 toFree = std::min(remaining, kBlocksPerGroup - groupOffset);
		status_t error = _FreeInGroup(base, toFree, transaction);
		if (error != B_OK)
			RETURN_ERROR(error);

		fFreeBlocks += toFree;
		remaining -= toFree;
		base += toFree;
		groupOffset = 0;
	}

	return B_OK;
}


status_t
BlockAllocator::_FreeInGroup(uint64 base, uint32 count,
	Transaction& transaction)
{
	if (count == 0)
		return B_OK;

	PRINT("BlockAllocator::_FreeInGroup(%" B_PRIu64 ", %" B_PRIu32 ")\n",
		base, count);

	ASSERT(count <= kBlocksPerGroup);
	ASSERT(base % kBlocksPerGroup + count <= kBlocksPerGroup);

	Block block;
	if (!block.GetWritable(fVolume,
			fAllocationGroupBlock + base / kBlocksPerGroup, transaction)) {
		RETURN_ERROR(B_ERROR);
	}

	uint16* counts = (uint16*)block.Data();

	uint32 blockIndex = base / kBlocksPerBitmapBlock % kBitmapBlocksPerGroup;
	uint64 inBlockOffset = base % kBlocksPerBitmapBlock;
	uint64 remaining = count;

	while (remaining > 0) {
		uint32 toFree = std::min(remaining,
			kBlocksPerBitmapBlock - inBlockOffset);

		if (counts[blockIndex] + toFree > kBlocksPerBitmapBlock)
			RETURN_ERROR(B_BAD_VALUE);

		status_t error = _FreeInBitmapBlock(base, toFree, transaction);
		if (error != B_OK)
			RETURN_ERROR(error);

		counts[blockIndex] += toFree;
		remaining -= toFree;
		base += toFree;
		blockIndex++;
		inBlockOffset = 0;
	}

	return B_OK;
}


status_t
BlockAllocator::_FreeInBitmapBlock(uint64 base, uint32 count,
	Transaction& transaction)
{
	PRINT("BlockAllocator::_FreeInBitmapBlock(%" B_PRIu64 ", %" B_PRIu32 ")\n",
		base, count);

	ASSERT(count <= kBlocksPerBitmapBlock);
	ASSERT(base % kBlocksPerBitmapBlock + count <= kBlocksPerBitmapBlock);

	Block block;
	if (!block.GetWritable(fVolume,
			fBitmapBlock + base / kBlocksPerBitmapBlock, transaction)) {
		RETURN_ERROR(B_ERROR);
	}

	uint32* bits = (uint32*)block.Data() + base % kBlocksPerBitmapBlock / 32;
	uint32 bitOffset = base % 32;
	uint32 remaining = count;

	// handle partial uint32 at the beginning
	if (bitOffset > 0) {
		uint32 endOffset = std::min(bitOffset + remaining, (uint32)32);

		uint32 mask = ~(((uint32)1 << bitOffset) - 1);
		if (endOffset < 32)
			mask &= ((uint32)1 << endOffset) - 1;

		if ((*bits & mask) != mask)
			RETURN_ERROR(B_BAD_VALUE);

		*bits &= ~mask;
		remaining -= endOffset - bitOffset;
	}

	// handle complete uint32s in the middle
	while (remaining >= 32) {
		if (*bits != 0xffffffff)
			RETURN_ERROR(B_BUSY);

		*bits = 0;
		remaining -= 32;
	}

	// handle partial uint32 at the end
	if (remaining > 0) {
		uint32 mask = ((uint32)1 << remaining) - 1;

		if ((*bits & mask) != mask)
			return B_BUSY;

		*bits &= ~mask;
	}

	return B_OK;
}


status_t
BlockAllocator::_UpdateSuperBlock(Transaction& transaction)
{
	// write the superblock
	Block block;
	if (!block.GetWritable(fVolume, kCheckSumFSSuperBlockOffset / B_PAGE_SIZE,
			transaction)) {
		return B_ERROR;
	}

	SuperBlock* superBlock = (SuperBlock*)block.Data();
	superBlock->SetFreeBlocks(fFreeBlocks);

	block.Put();

	return B_OK;
}
