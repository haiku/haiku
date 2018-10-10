/*
 * Copyright 2001-2010, Haiku Inc. All rights reserved.
 * This file may be used under the terms of the MIT License.
 *
 * Authors:
 *		Janito V. Ferreira Filho
 */


#include "DataStream.h"

#include "CachedBlock.h"
#include "Volume.h"


//#define TRACE_EXT2
#ifdef TRACE_EXT2
#	define TRACE(x...) dprintf("\33[34mext2:\33[0m " x)
#else
#	define TRACE(x...) ;
#endif
#define ERROR(x...)	dprintf("\33[34mext2:\33[0m " x)


DataStream::DataStream(Volume* volume, ext2_data_stream* stream,
	off_t size)
	:
	kBlockSize(volume->BlockSize()),
	kIndirectsPerBlock(kBlockSize / sizeof(uint32)),
	kIndirectsPerBlock2(kIndirectsPerBlock * kIndirectsPerBlock),
	kIndirectsPerBlock3(kIndirectsPerBlock2 * kIndirectsPerBlock),
	kMaxDirect(EXT2_DIRECT_BLOCKS),
	kMaxIndirect(kMaxDirect + kIndirectsPerBlock),
	kMaxDoubleIndirect(kMaxIndirect + kIndirectsPerBlock2),
	fVolume(volume),
	fStream(stream),
	fFirstBlock(volume->FirstDataBlock()),
	fAllocated(0),
	fAllocatedPos(fFirstBlock),
	fWaiting(0),
	fFreeStart(0),
	fFreeCount(0),
	fRemovedBlocks(0),
	fSize(size)
{
	fNumBlocks = size == 0 ? 0 : ((size - 1) >> fVolume->BlockShift()) + 1;
}


DataStream::~DataStream()
{
}


status_t
DataStream::FindBlock(off_t offset, fsblock_t& block, uint32 *_count)
{
	uint32 index = offset >> fVolume->BlockShift();

	if (offset >= fSize) {
		TRACE("FindBlock: offset larger than inode size\n");
		return B_ENTRY_NOT_FOUND;
	}

	// TODO: we could return the size of the sparse range, as this might be more
	// than just a block

	if (index < EXT2_DIRECT_BLOCKS) {
		// direct blocks
		block = B_LENDIAN_TO_HOST_INT32(fStream->direct[index]);
		ASSERT(block != 0);
		if (_count) {
			*_count = 1;
			uint32 nextBlock = block;
			while (++index < EXT2_DIRECT_BLOCKS
				&& fStream->direct[index] == ++nextBlock)
				(*_count)++;
		}
	} else if ((index -= EXT2_DIRECT_BLOCKS) < kIndirectsPerBlock) {
		// indirect blocks
		CachedBlock cached(fVolume);
		uint32* indirectBlocks = (uint32*)cached.SetTo(B_LENDIAN_TO_HOST_INT32(
			fStream->indirect));
		if (indirectBlocks == NULL)
			return B_IO_ERROR;

		block = B_LENDIAN_TO_HOST_INT32(indirectBlocks[index]);
		ASSERT(block != 0);
		if (_count) {
			*_count = 1;
			uint32 nextBlock = block;
			while (++index < kIndirectsPerBlock
				&& indirectBlocks[index] == ++nextBlock)
				(*_count)++;
		}
	} else if ((index -= kIndirectsPerBlock) < kIndirectsPerBlock2) {
		// double indirect blocks
		CachedBlock cached(fVolume);
		uint32* indirectBlocks = (uint32*)cached.SetTo(B_LENDIAN_TO_HOST_INT32(
			fStream->double_indirect));
		if (indirectBlocks == NULL)
			return B_IO_ERROR;

		uint32 indirectIndex = B_LENDIAN_TO_HOST_INT32(indirectBlocks[index 
			/ kIndirectsPerBlock]);
		if (indirectIndex == 0) {
			// a sparse indirect block
			block = 0;
		} else {
			indirectBlocks = (uint32*)cached.SetTo(indirectIndex);
			if (indirectBlocks == NULL)
				return B_IO_ERROR;

			block = B_LENDIAN_TO_HOST_INT32(
				indirectBlocks[index & (kIndirectsPerBlock - 1)]);
			if (_count) {
				*_count = 1;
				uint32 nextBlock = block;
				while (((++index & (kIndirectsPerBlock - 1)) != 0)
					&& indirectBlocks[index & (kIndirectsPerBlock - 1)]
						== ++nextBlock)
					(*_count)++;
			}
		}
		ASSERT(block != 0);
	} else if ((index -= kIndirectsPerBlock2) < kIndirectsPerBlock3) {
		// triple indirect blocks
		CachedBlock cached(fVolume);
		uint32* indirectBlocks = (uint32*)cached.SetTo(B_LENDIAN_TO_HOST_INT32(
			fStream->triple_indirect));
		if (indirectBlocks == NULL)
			return B_IO_ERROR;

		uint32 indirectIndex = B_LENDIAN_TO_HOST_INT32(indirectBlocks[index
			/ kIndirectsPerBlock2]);
		if (indirectIndex == 0) {
			// a sparse indirect block
			block = 0;
		} else {
			indirectBlocks = (uint32*)cached.SetTo(indirectIndex);
			if (indirectBlocks == NULL)
				return B_IO_ERROR;

			indirectIndex = B_LENDIAN_TO_HOST_INT32(
				indirectBlocks[(index / kIndirectsPerBlock) & (kIndirectsPerBlock - 1)]);
			if (indirectIndex == 0) {
				// a sparse indirect block
				block = 0;
			} else {
				indirectBlocks = (uint32*)cached.SetTo(indirectIndex);
				if (indirectBlocks == NULL)
					return B_IO_ERROR;

				block = B_LENDIAN_TO_HOST_INT32(
					indirectBlocks[index & (kIndirectsPerBlock - 1)]);
				if (_count) {
					*_count = 1;
					uint32 nextBlock = block;
					while (((++index & (kIndirectsPerBlock - 1)) != 0)
						&& indirectBlocks[index & (kIndirectsPerBlock - 1)]
							== ++nextBlock)
						(*_count)++;
				}
			}
		}
		ASSERT(block != 0);
	} else {
		// Outside of the possible data stream
		ERROR("ext2: block outside datastream!\n");
		return B_ERROR;
	}

	TRACE("FindBlock(offset %" B_PRIdOFF "): %" B_PRIu64" %" B_PRIu32 "\n", offset,
		block, _count != NULL ? *_count : 1);
	return B_OK;
}


status_t
DataStream::Enlarge(Transaction& transaction, off_t& numBlocks)
{
	TRACE("DataStream::Enlarge(): current size: %" B_PRIdOFF ", target size: %"
		B_PRIdOFF "\n", fNumBlocks, numBlocks);
	
	off_t targetBlocks = numBlocks;
	fWaiting = _BlocksNeeded(numBlocks);
	numBlocks = fWaiting;

	status_t status;

	if (fNumBlocks <= kMaxDirect) {
		status = _AddForDirectBlocks(transaction, targetBlocks);

		if (status != B_OK) {
			ERROR("DataStream::Enlarge(): _AddForDirectBlocks() failed\n");
			return status;
		}

		TRACE("DataStream::Enlarge(): current size: %" B_PRIdOFF
			", target size: %" B_PRIdOFF "\n", fNumBlocks, targetBlocks);
	
		if (fNumBlocks == targetBlocks)
			return B_OK;
	}

	TRACE("DataStream::Enlarge(): indirect current size: %" B_PRIdOFF
		", target size: %" B_PRIdOFF "\n", fNumBlocks, targetBlocks);

	if (fNumBlocks <= kMaxIndirect) {
		status = _AddForIndirectBlock(transaction, targetBlocks);

		if (status != B_OK) {
			ERROR("DataStream::Enlarge(): _AddForIndirectBlock() failed\n");
			return status;
		}

		TRACE("DataStream::Enlarge(): current size: %" B_PRIdOFF
			", target size: %" B_PRIdOFF "\n", fNumBlocks, targetBlocks);
	
		if (fNumBlocks == targetBlocks)
			return B_OK;
	}

	TRACE("DataStream::Enlarge(): indirect2 current size: %" B_PRIdOFF
		", target size: %" B_PRIdOFF "\n", fNumBlocks, targetBlocks);

	if (fNumBlocks <= kMaxDoubleIndirect) {
		status = _AddForDoubleIndirectBlock(transaction, targetBlocks);

		if (status != B_OK) {
			ERROR("DataStream::Enlarge(): _AddForDoubleIndirectBlock() failed\n");
			return status;
		}

		TRACE("DataStream::Enlarge(): current size: %" B_PRIdOFF
			", target size: %" B_PRIdOFF "\n", fNumBlocks, targetBlocks);
	
		if (fNumBlocks == targetBlocks)
			return B_OK;
	}

	TRACE("DataStream::Enlarge(): indirect3 current size: %" B_PRIdOFF
		", target size: %" B_PRIdOFF "\n", fNumBlocks, targetBlocks);

	TRACE("DataStream::Enlarge(): allocated: %" B_PRIu32 ", waiting: %"
		B_PRIu32 "\n", fAllocated, fWaiting);

	return _AddForTripleIndirectBlock(transaction, targetBlocks);
}


status_t
DataStream::Shrink(Transaction& transaction, off_t& numBlocks)
{
	TRACE("DataStream::Shrink(): current size: %" B_PRIdOFF ", target size: %"
		B_PRIdOFF "\n", fNumBlocks, numBlocks);

	fFreeStart = 0;
	fFreeCount = 0;
	fRemovedBlocks = 0;

	off_t oldNumBlocks = fNumBlocks;
	off_t blocksToRemove = fNumBlocks - numBlocks;

	status_t status;

	if (numBlocks < kMaxDirect) {
		status = _RemoveFromDirectBlocks(transaction, numBlocks);

		if (status != B_OK) {
			ERROR("DataStream::Shrink(): _RemoveFromDirectBlocks() failed\n");
			return status;
		}

		if (fRemovedBlocks == blocksToRemove) {
			fNumBlocks -= fRemovedBlocks;
			numBlocks = _BlocksNeeded(oldNumBlocks);

			return _PerformFree(transaction);
		}
	}

	if (numBlocks < kMaxIndirect) {
		status = _RemoveFromIndirectBlock(transaction, numBlocks);

		if (status != B_OK) {
			ERROR("DataStream::Shrink(): _RemoveFromIndirectBlock() failed\n");
			return status;
		}

		if (fRemovedBlocks == blocksToRemove) {
			fNumBlocks -= fRemovedBlocks;
			numBlocks = _BlocksNeeded(oldNumBlocks);

			return _PerformFree(transaction);
		}
	}

	if (numBlocks < kMaxDoubleIndirect) {
		status = _RemoveFromDoubleIndirectBlock(transaction, numBlocks);

		if (status != B_OK) {
			ERROR("DataStream::Shrink(): _RemoveFromDoubleIndirectBlock() failed\n");
			return status;
		}

		if (fRemovedBlocks == blocksToRemove) {
			fNumBlocks -= fRemovedBlocks;
			numBlocks = _BlocksNeeded(oldNumBlocks);

			return _PerformFree(transaction);
		}
	}

	status = _RemoveFromTripleIndirectBlock(transaction, numBlocks);

	if (status != B_OK) {
		ERROR("DataStream::Shrink(): _RemoveFromTripleIndirectBlock() failed\n");
		return status;
	}

	fNumBlocks -= fRemovedBlocks;
	numBlocks = _BlocksNeeded(oldNumBlocks);

	return _PerformFree(transaction);
}


uint32
DataStream::_BlocksNeeded(off_t numBlocks)
{
	TRACE("DataStream::BlocksNeeded(): num blocks %" B_PRIdOFF "\n", numBlocks);
	off_t blocksNeeded = 0;

	if (numBlocks > fNumBlocks) {
		blocksNeeded += numBlocks - fNumBlocks;

		if (numBlocks > kMaxDirect) {
			if (fNumBlocks <= kMaxDirect)
				blocksNeeded += 1;

			if (numBlocks > kMaxIndirect) {
				if (fNumBlocks <= kMaxIndirect) {
					blocksNeeded += 2 + (numBlocks - kMaxIndirect - 1)
						/ kIndirectsPerBlock;
				} else {
					blocksNeeded += (numBlocks - kMaxIndirect - 1)
						/ kIndirectsPerBlock - (fNumBlocks
							- kMaxIndirect - 1) / kIndirectsPerBlock;
				}

				if (numBlocks > kMaxDoubleIndirect) {
					if (fNumBlocks <= kMaxDoubleIndirect) {
						blocksNeeded += 2 + (numBlocks - kMaxDoubleIndirect - 1)
							/ kIndirectsPerBlock2;
					} else {
						blocksNeeded += (numBlocks - kMaxDoubleIndirect - 1)
							/ kIndirectsPerBlock - (fNumBlocks 
								- kMaxDoubleIndirect - 1) / kIndirectsPerBlock;
					}
				}
			}
		}
	}

	TRACE("DataStream::BlocksNeeded(): %" B_PRIdOFF "\n", blocksNeeded);
	return blocksNeeded;
}


status_t
DataStream::_GetBlock(Transaction& transaction, uint32& blockNum)
{
	TRACE("DataStream::_GetBlock(): allocated: %" B_PRIu32 ", pos: %" B_PRIu64
		", waiting: %" B_PRIu32 "\n", fAllocated, fAllocatedPos, fWaiting);

	if (fAllocated == 0) {
		uint32 blockGroup = (fAllocatedPos - fFirstBlock)
			/ fVolume->BlocksPerGroup();
		
		status_t status = fVolume->AllocateBlocks(transaction, 1, fWaiting,
			blockGroup, fAllocatedPos, fAllocated);
		if (status != B_OK) {
			ERROR("DataStream::_GetBlock(): AllocateBlocks() failed()\n");
			return status;
		}
		if (fAllocatedPos > UINT_MAX)
			return B_FILE_TOO_LARGE;

		fWaiting -= fAllocated;

		TRACE("DataStream::_GetBlock(): newAllocated: %" B_PRIu32 ", newpos: %"
			B_PRIu64 ", newwaiting: %" B_PRIu32 "\n", fAllocated,
			fAllocatedPos, fWaiting);
	}

	fAllocated--;
	blockNum = (uint32)fAllocatedPos++;

	return B_OK;
}


status_t
DataStream::_PrepareBlock(Transaction& transaction, uint32* pos,
	uint32& blockNum, bool& clear)
{
	blockNum = B_LENDIAN_TO_HOST_INT32(*pos);
	clear = false;

	if (blockNum == 0) {
		status_t status = _GetBlock(transaction, blockNum);
		if (status != B_OK) {
			ERROR("DataStream::_PrepareBlock() _GetBlock() failed blockNum %"
				B_PRIu32 "\n", blockNum);
			return status;
		}

		*pos = B_HOST_TO_LENDIAN_INT32(blockNum);
		clear = true;
	}

	return B_OK;
}


status_t
DataStream::_AddBlocks(Transaction& transaction, uint32* block, off_t _count)
{
	off_t count = _count;
	TRACE("DataStream::_AddBlocks(): count: %" B_PRIdOFF "\n", count);

	while (count > 0) {
		uint32 blockNum;
		status_t status = _GetBlock(transaction, blockNum);
		if (status != B_OK)
			return status;

		*(block++) = B_HOST_TO_LENDIAN_INT32(blockNum);
		--count;
	}

	fNumBlocks += _count;

	return B_OK;
}


status_t
DataStream::_AddBlocks(Transaction& transaction, uint32* block, off_t start,
	off_t end, int recursion)
{
	TRACE("DataStream::_AddBlocks(): start: %" B_PRIdOFF ", end %" B_PRIdOFF
		", recursion: %d\n", start, end, recursion);

	bool clear;
	uint32 blockNum;
	status_t status = _PrepareBlock(transaction, block, blockNum, clear);
	if (status != B_OK)
		return status;

	CachedBlock cached(fVolume);	
	uint32* childBlock = (uint32*)cached.SetToWritable(transaction, blockNum,
		clear);
	if (childBlock == NULL)
		return B_IO_ERROR;

	if (recursion == 0)
		return _AddBlocks(transaction, &childBlock[start], end - start);

	uint32 elementWidth;
	if (recursion == 1)
		elementWidth = kIndirectsPerBlock;
	else if (recursion == 2)
		elementWidth = kIndirectsPerBlock2;
	else {
		panic("Undefined recursion level\n");
		elementWidth = 0;
	}

	uint32 elementPos = start / elementWidth;
	uint32 endPos = end / elementWidth;

	TRACE("DataStream::_AddBlocks(): element pos: %" B_PRIu32 ", end pos: %"
		B_PRIu32 "\n", elementPos, endPos);

	recursion--;

	if (elementPos == endPos) {
		return _AddBlocks(transaction, &childBlock[elementPos],
			start % elementWidth, end % elementWidth, recursion);
	}
	
	if (start % elementWidth != 0) {
		status = _AddBlocks(transaction, &childBlock[elementPos],
			start % elementWidth, elementWidth, recursion);
		if (status != B_OK) {
			ERROR("DataStream::_AddBlocks() _AddBlocks() start failed\n");
			return status;
		}

		elementPos++;
	}

	while (elementPos < endPos) {
		status = _AddBlocks(transaction, &childBlock[elementPos], 0,
			elementWidth, recursion);
		if (status != B_OK) {
			ERROR("DataStream::_AddBlocks() _AddBlocks() mid failed\n");
			return status;
		}

		elementPos++;
	}

	if (end % elementWidth != 0) {
		status = _AddBlocks(transaction, &childBlock[elementPos], 0,
			end % elementWidth, recursion);
		if (status != B_OK) {
			ERROR("DataStream::_AddBlocks() _AddBlocks() end failed\n");
			return status;
		}
	}
		
	return B_OK;
}


status_t
DataStream::_AddForDirectBlocks(Transaction& transaction, uint32 numBlocks)
{
	TRACE("DataStream::_AddForDirectBlocks(): current size: %" B_PRIdOFF
		", target size: %" B_PRIu32 "\n", fNumBlocks, numBlocks);
	uint32* direct = &fStream->direct[fNumBlocks];
	uint32 end = numBlocks > kMaxDirect ? kMaxDirect : numBlocks;

	return _AddBlocks(transaction, direct, end - fNumBlocks);
}


status_t
DataStream::_AddForIndirectBlock(Transaction& transaction, uint32 numBlocks)
{
	TRACE("DataStream::_AddForIndirectBlocks(): current size: %" B_PRIdOFF
		", target size: %" B_PRIu32 "\n", fNumBlocks, numBlocks);
	uint32 *indirect = &fStream->indirect;
	uint32 start = fNumBlocks - kMaxDirect;
	uint32 end = numBlocks - kMaxDirect;

	if (end > kIndirectsPerBlock)
		end = kIndirectsPerBlock;

	return _AddBlocks(transaction, indirect, start, end, 0);
}


status_t
DataStream::_AddForDoubleIndirectBlock(Transaction& transaction,
	uint32 numBlocks)
{
	TRACE("DataStream::_AddForDoubleIndirectBlock(): current size: %" B_PRIdOFF
		", target size: %" B_PRIu32 "\n", fNumBlocks, numBlocks);
	uint32 *doubleIndirect = &fStream->double_indirect;
	uint32 start = fNumBlocks - kMaxIndirect;
	uint32 end = numBlocks - kMaxIndirect;

	if (end > kIndirectsPerBlock2)
		end = kIndirectsPerBlock2;

	return _AddBlocks(transaction, doubleIndirect, start, end, 1);
}


status_t
DataStream::_AddForTripleIndirectBlock(Transaction& transaction,
	uint32 numBlocks)
{
	TRACE("DataStream::_AddForTripleIndirectBlock(): current size: %" B_PRIdOFF
		", target size: %" B_PRIu32 "\n", fNumBlocks, numBlocks);
	uint32 *tripleIndirect = &fStream->triple_indirect;
	uint32 start = fNumBlocks - kMaxDoubleIndirect;
	uint32 end = numBlocks - kMaxDoubleIndirect;

	return _AddBlocks(transaction, tripleIndirect, start, end, 2);
}


status_t
DataStream::_PerformFree(Transaction& transaction)
{
	TRACE("DataStream::_PerformFree(): start: %" B_PRIu32 ", count: %" B_PRIu32
		"\n", fFreeStart, fFreeCount);
	status_t status;

	if (fFreeCount == 0)
		status = B_OK;
	else
		status = fVolume->FreeBlocks(transaction, fFreeStart, fFreeCount);

	fFreeStart = 0;
	fFreeCount = 0;

	return status;
}


status_t
DataStream::_MarkBlockForRemoval(Transaction& transaction, uint32* block)
{
	
	TRACE("DataStream::_MarkBlockForRemoval(*(%p) = %" B_PRIu32
		"): free start: %" B_PRIu32 ", free count: %" B_PRIu32 "\n", block,
		B_LENDIAN_TO_HOST_INT32(*block), fFreeStart, fFreeCount);
	uint32 blockNum = B_LENDIAN_TO_HOST_INT32(*block);
	*block = 0;

	if (blockNum != fFreeStart + fFreeCount) {
		if (fFreeCount != 0) {
			status_t status = fVolume->FreeBlocks(transaction, fFreeStart,
				fFreeCount);
			if (status != B_OK)
				return status;
		}

		fFreeStart = blockNum;
		fFreeCount = 0;
	}

	fFreeCount++;

	return B_OK;
}


status_t
DataStream::_FreeBlocks(Transaction& transaction, uint32* block, uint32 _count)
{
	uint32 count = _count;
	TRACE("DataStream::_FreeBlocks(%p, %" B_PRIu32 ")\n", block, count);

	while (count > 0) {
		status_t status = _MarkBlockForRemoval(transaction, block);
		if (status != B_OK)
			return status;

		block++;
		count--;
	}

	fRemovedBlocks += _count;

	return B_OK;
}


status_t
DataStream::_FreeBlocks(Transaction& transaction, uint32* block, off_t start,
	off_t end, bool freeParent, int recursion)
{
	// TODO: Designed specifically for shrinking. Perhaps make it more general?
	TRACE("DataStream::_FreeBlocks(%p, %" B_PRIdOFF ", %" B_PRIdOFF
		", %c, %d)\n", block, start, end, freeParent ? 't' : 'f', recursion);

	uint32 blockNum = B_LENDIAN_TO_HOST_INT32(*block);

	if (freeParent) {
		status_t status = _MarkBlockForRemoval(transaction, block);
		if (status != B_OK)
			return status;
	}

	CachedBlock cached(fVolume);
	uint32* childBlock = (uint32*)cached.SetToWritable(transaction, blockNum);
	if (childBlock == NULL)
		return B_IO_ERROR;

	if (recursion == 0)
		return _FreeBlocks(transaction, &childBlock[start], end - start);

	uint32 elementWidth;
	if (recursion == 1)
		elementWidth = kIndirectsPerBlock;
	else if (recursion == 2)
		elementWidth = kIndirectsPerBlock2;
	else {
		panic("Undefinied recursion level\n");
		elementWidth = 0;
	}

	uint32 elementPos = start / elementWidth;
	uint32 endPos = end / elementWidth;

	recursion--;

	if (elementPos == endPos) {
		bool free = freeParent || start % elementWidth == 0;
		return _FreeBlocks(transaction, &childBlock[elementPos],
			start % elementWidth, end % elementWidth, free, recursion);
	}

	status_t status = B_OK;

	if (start % elementWidth != 0) {
		status = _FreeBlocks(transaction, &childBlock[elementPos],
			start % elementWidth, elementWidth, false, recursion);
		if (status != B_OK)
			return status;

		elementPos++;
	}

	while (elementPos < endPos) {
		status = _FreeBlocks(transaction, &childBlock[elementPos], 0,
			elementWidth, true, recursion);
		if (status != B_OK)
			return status;

		elementPos++;
	}

	if (end % elementWidth != 0) {
		status = _FreeBlocks(transaction, &childBlock[elementPos], 0,
			end % elementWidth, true, recursion);
	}

	return status;
}


status_t
DataStream::_RemoveFromDirectBlocks(Transaction& transaction, uint32 numBlocks)
{
	TRACE("DataStream::_RemoveFromDirectBlocks(): current size: %" B_PRIdOFF
		", target size: %" B_PRIu32 "\n", fNumBlocks, numBlocks);
	uint32* direct = &fStream->direct[numBlocks];
	off_t end = fNumBlocks > kMaxDirect ? kMaxDirect : fNumBlocks;

	return _FreeBlocks(transaction, direct, end - numBlocks);
}


status_t
DataStream::_RemoveFromIndirectBlock(Transaction& transaction, uint32 numBlocks)
{
	TRACE("DataStream::_RemoveFromIndirectBlock(): current size: %" B_PRIdOFF
		", target size: %" B_PRIu32 "\n", fNumBlocks, numBlocks);
	uint32* indirect = &fStream->indirect;
	off_t start = numBlocks <= kMaxDirect ? 0 : numBlocks - kMaxDirect;
	off_t end = fNumBlocks - kMaxDirect;

	if (end > kIndirectsPerBlock)
		end = kIndirectsPerBlock;

	bool freeAll = start == 0;

	return _FreeBlocks(transaction, indirect, start, end, freeAll, 0);
}


status_t
DataStream::_RemoveFromDoubleIndirectBlock(Transaction& transaction,
	uint32 numBlocks)
{
	TRACE("DataStream::_RemoveFromDoubleIndirectBlock(): current size: %" B_PRIdOFF
		", target size: %" B_PRIu32 "\n", fNumBlocks, numBlocks);
	uint32* doubleIndirect = &fStream->double_indirect;
	off_t start = numBlocks <= kMaxIndirect ? 0 : numBlocks - kMaxIndirect;
	off_t end = fNumBlocks - kMaxIndirect;

	if (end > kIndirectsPerBlock2)
		end = kIndirectsPerBlock2;

	bool freeAll = start == 0;

	return _FreeBlocks(transaction, doubleIndirect, start, end, freeAll, 1);
}


status_t
DataStream::_RemoveFromTripleIndirectBlock(Transaction& transaction,
	uint32 numBlocks)
{
	TRACE("DataStream::_RemoveFromTripleIndirectBlock(): current size: %" B_PRIdOFF
		", target size: %" B_PRIu32 "\n", fNumBlocks, numBlocks);
	uint32* tripleIndirect = &fStream->triple_indirect;
	off_t start = numBlocks <= kMaxDoubleIndirect ? 0
		: numBlocks - kMaxDoubleIndirect;
	off_t end = fNumBlocks - kMaxDoubleIndirect;

	bool freeAll = start == 0;

	return _FreeBlocks(transaction, tripleIndirect, start, end, freeAll, 2);
}
