// BlockAllocatorArea.cpp

#include "BlockAllocatorArea.h"
#include "Debug.h"

// constructor
BlockAllocator::Area::Area(area_id id, size_t size)
	: fBucket(NULL),
	  fID(id),
	  fSize(size),
	  fFreeBytes(0),
	  fFreeBlockCount(1),
	  fUsedBlockCount(0),
	  fFirstBlock(NULL),
	  fLastBlock(NULL),
	  fFirstFree(NULL),
	  fLastFree(NULL)
{
	size_t headerSize = block_align_ceil(sizeof(Area));
	fFirstFree = (TFreeBlock*)((char*)this + headerSize);
	fFirstFree->SetTo(NULL, block_align_floor(fSize - headerSize), false, NULL,
					  NULL);
	fFirstBlock = fLastBlock = fLastFree = fFirstFree;
	fFreeBytes = fFirstFree->GetUsableSize();
}

// Create
BlockAllocator::Area *
BlockAllocator::Area::Create(size_t size)
{
	Area *area = NULL;
	void *base = NULL;
#if USER
	area_id id = create_area("block alloc", &base, B_ANY_ADDRESS,
							 size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
#else
	area_id id = create_area("block alloc", &base, B_ANY_KERNEL_ADDRESS,
							 size, B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);
#endif
	if (id >= 0) {
		area = new(base) Area(id, size);
	} else {
		ERROR(("BlockAllocator::Area::Create(%lu): Failed to create area: %s\n",
			size, strerror(id)));
	}
	return area;
}

// Delete
void
BlockAllocator::Area::Delete()
{
	delete_area(fID);
}

// AllocateBlock
Block *
BlockAllocator::Area::AllocateBlock(size_t usableSize, bool dontDefragment)
{
if (kMinBlockSize != block_align_ceil(sizeof(TFreeBlock))) {
FATAL(("kMinBlockSize is not correctly initialized! Is %lu, but should be: "
"%lu\n", kMinBlockSize, block_align_ceil(sizeof(TFreeBlock))));
BA_PANIC("kMinBlockSize not correctly initialized.");
return NULL;
}
	if (usableSize == 0)
		return NULL;
	Block *newBlock = NULL;
	size_t size = max(usableSize + sizeof(BlockHeader), kMinBlockSize);
	size = block_align_ceil(size);
	if (size <= _GetBlockFreeBytes()) {
		// find first fit
		TFreeBlock *block = _FindFreeBlock(size);
		if (!block && !dontDefragment) {
			// defragmenting is necessary
			_Defragment();
			block = _FindFreeBlock(size);
			if (!block) {
				// no free block
				// Our data structures seem to be corrupted, since
				// _GetBlockFreeBytes() promised that we would have enough
				// free space.
				FATAL(("Couldn't find free block of min size %lu after "
					   "defragmenting, although we should have %lu usable free "
					   "bytes!\n", size, _GetBlockFreeBytes()));
				BA_PANIC("Bad area free bytes.");
			}
		}
		if (block) {
			// found a free block
			size_t remainder = block->GetSize() - size;
			if (remainder >= kMinBlockSize) {
				// enough space left for a free block
				Block *freePrev = block->GetPreviousBlock();
//				TFreeBlock *prevFree = block->GetPreviousFreeBlock();
//				TFreeBlock *nextFree = block->GetNextFreeBlock();
//				newBlock = block;
				_MoveResizeFreeBlock(block, size, remainder);
				// setup the new block
//				newBlock->SetSize(size, true);
//				newBlock->SetFree(false);
				newBlock = _MakeUsedBlock(block, 0, freePrev, size, true);
			} else {
				// not enough space left: take the free block over completely
				// remove the block from the free list
				_RemoveFreeBlock(block);
				newBlock = block;
				newBlock->SetFree(false);
			}
			if (fFreeBlockCount)
				fFreeBytes -= newBlock->GetSize();
			else
				fFreeBytes = 0;
			fUsedBlockCount++;
		}
	}
	D(SanityCheck());
	return newBlock;
}

// FreeBlock
void
BlockAllocator::Area::FreeBlock(Block *block, bool dontDefragment)
{
	if (block) {
		// mark the block free and insert it into the free list
		block->SetFree(true);
		TFreeBlock *freeBlock = (TFreeBlock*)block;
		_InsertFreeBlock(freeBlock);
		fUsedBlockCount--;
		if (fFreeBlockCount == 1)
			fFreeBytes += freeBlock->GetUsableSize();
		else
			fFreeBytes += freeBlock->GetSize();
		// try coalescing with the next and the previous free block
D(SanityCheck());
		_CoalesceWithNext(freeBlock);
D(SanityCheck());
		_CoalesceWithNext(freeBlock->GetPreviousFreeBlock());
		// defragment, if sensible
		if (!dontDefragment && _DefragmentingRecommended())
			_Defragment();
		D(SanityCheck());
	}
}

// ResizeBlock
Block *
BlockAllocator::Area::ResizeBlock(Block *block, size_t newUsableSize,
								  bool dontDefragment)
{
//PRINT(("Area::ResizeBlock(%p, %lu)\n", block, newUsableSize));
// newUsableSize must be >0 !
	if (newUsableSize == 0)
		return NULL;
	Block *resultBlock = NULL;
	if (block) {
		size_t size = block->GetSize();
		size_t newSize = max(newUsableSize + sizeof(BlockHeader),
							 kMinBlockSize);
		newSize = block_align_ceil(newSize);
		if (newSize == size) {
			// size doesn't change: nothing to do
			resultBlock = block;
		} else if (newSize < size) {
			// shrink the block
			size_t sizeDiff = size - newSize;
			Block *nextBlock = block->GetNextBlock();
			if (nextBlock && nextBlock->IsFree()) {
				// join the space with the adjoining free block
				TFreeBlock *freeBlock = nextBlock->ToFreeBlock();
				_MoveResizeFreeBlock(freeBlock, -sizeDiff,
									 freeBlock->GetSize() + sizeDiff);
				// resize the block and we're done
				block->SetSize(newSize, true);
				fFreeBytes += sizeDiff;
			} else if (sizeDiff >= sizeof(TFreeBlock)) {
				// the freed space is large enough for a free block
				TFreeBlock *newFree = _MakeFreeBlock(block, newSize, block,
					sizeDiff, nextBlock, NULL, NULL);
				_InsertFreeBlock(newFree);
				block->SetSize(newSize, true);
				if (fFreeBlockCount == 1)
					fFreeBytes += newFree->GetUsableSize();
				else
					fFreeBytes += newFree->GetSize();
				if (!dontDefragment && _DefragmentingRecommended())
					_Defragment();
			}	// else: insufficient space for a free block: no changes
			resultBlock = block;
		} else {
//PRINT(("  grow...\n"));
			// grow the block
			size_t sizeDiff = newSize - size;
			Block *nextBlock = block->GetNextBlock();
			if (nextBlock && nextBlock->IsFree()
				&& nextBlock->GetSize() >= sizeDiff) {
//PRINT(("  adjoining free block\n"));
				// there is a adjoining free block and it is large enough
				TFreeBlock *freeBlock = nextBlock->ToFreeBlock();
				size_t freeSize = freeBlock->GetSize();
				if (freeSize - sizeDiff >= sizeof(TFreeBlock)) {
					// the remaining space is still large enough for a free
					// block
					_MoveResizeFreeBlock(freeBlock, sizeDiff,
										 freeSize - sizeDiff);
					block->SetSize(newSize, true);
					fFreeBytes -= sizeDiff;
				} else {
					// the remaining free space wouldn't be large enough for
					// a free block: consume the free block completely
					Block *freeNext = freeBlock->GetNextBlock();
					_RemoveFreeBlock(freeBlock);
					block->SetSize(size + freeSize, freeNext);
					_FixBlockList(block, block->GetPreviousBlock(), freeNext);
					if (fFreeBlockCount == 0)
						fFreeBytes = 0;
					else
						fFreeBytes -= freeSize;
				}
				resultBlock = block;
			} else {
//PRINT(("  no adjoining free block\n"));
				// no (large enough) adjoining free block: allocate
				// a new block and copy the data to it
				BlockReference *reference = block->GetReference();
				resultBlock = AllocateBlock(newUsableSize, dontDefragment);
				block = reference->GetBlock();
				if (resultBlock) {
					resultBlock->SetReference(reference);
					memcpy(resultBlock->GetData(), block->GetData(),
						   block->GetUsableSize());
					FreeBlock(block, dontDefragment);
					resultBlock = reference->GetBlock();
				}
			}
		}
	}
	D(SanityCheck());
//PRINT(("Area::ResizeBlock() done: %p\n", resultBlock));
	return resultBlock;
}

// SanityCheck
bool
BlockAllocator::Area::SanityCheck() const
{
	// area ID
	if (fID < 0) {
		FATAL(("Area ID < 0: %lx\n", fID));
		BA_PANIC("Bad area ID.");
		return false;
	}
	// size
	size_t areaHeaderSize = block_align_ceil(sizeof(Area));
	if (fSize < areaHeaderSize + sizeof(TFreeBlock)) {
		FATAL(("Area too small to contain area header and at least one free "
			   "block: %lu bytes\n", fSize));
		BA_PANIC("Bad area size.");
		return false;
	}
	// free bytes
	if (fFreeBytes > fSize) {
		FATAL(("Free size greater than area size: %lu vs %lu\n", fFreeBytes,
			   fSize));
		BA_PANIC("Bad area free bytes.");
		return false;
	}
	// block count
	if (fFreeBlockCount + fUsedBlockCount == 0) {
		FATAL(("Area contains no blocks at all.\n"));
		BA_PANIC("Bad area block count.");
		return false;
	}
	// block list
	uint32 usedBlockCount = 0;
	uint32 freeBlockCount = 0;
	size_t freeBytes = 0;
	if (!fFirstBlock || !fLastBlock) {
		FATAL(("Invalid block list: first or last block NULL: first: %p, "
			   "last: %p\n", fFirstBlock, fLastBlock));
		BA_PANIC("Bad area block list.");
		return false;
	} else {
		// iterate through block list and also check free list
		int32 blockCount = fFreeBlockCount + fUsedBlockCount;
		Block *block = fFirstBlock;
		Block *prevBlock = NULL;
		Block *prevFree = NULL;
		Block *nextFree = fFirstFree;
		bool blockListOK = true;
		for (int32 i = 0; i < blockCount; i++) {
			blockListOK = false;
			if (!block) {
				FATAL(("Encountered NULL in block list at index %ld, although "
					   "list should have %ld blocks\n", i, blockCount));
				BA_PANIC("Bad area block list.");
				return false;
			}
			uint64 address = (uint32)block;
			// block within area?
			if (address < (uint32)this + areaHeaderSize
				|| address + sizeof(TFreeBlock) > (uint32)this + fSize) {
				FATAL(("Utterly mislocated block: %p, area: %p, "
					   "size: %lu\n", block, this, fSize));
				BA_PANIC("Bad area block.");
				return false;
			}
			// block too large for area?
			size_t blockSize = block->GetSize();
			if (blockSize < sizeof(TFreeBlock)
				|| address + blockSize > (uint32)this + fSize) {
				FATAL(("Mislocated block: %p, size: %lu, area: %p, "
					   "size: %lu\n", block, blockSize, this, fSize));
				BA_PANIC("Bad area block.");
				return false;
			}
			// alignment
			if (block_align_floor(address) != address
				|| block_align_floor(blockSize) != blockSize) {
				FATAL(("Block %ld not properly aligned: %p, size: %lu\n",
					   i, block, blockSize));
				BA_PANIC("Bad area block.");
				return false;
			}
			// previous block
			if (block->GetPreviousBlock() != prevBlock) {
				FATAL(("Previous block of block %ld was not the previous "
					   "block in list: %p vs %p\n", i,
					   block->GetPreviousBlock(), prevBlock));
				BA_PANIC("Bad area block list.");
				return false;
			}
			// additional checks for free block list
			if (block->IsFree()) {
				freeBlockCount++;
				TFreeBlock *freeBlock = block->ToFreeBlock();
				if (prevFree)
					freeBytes += freeBlock->GetSize();
				else
					freeBytes += freeBlock->GetUsableSize();
				// block == next free block of previous free block
				if (freeBlock != nextFree) {
					FATAL(("Free block %ld is not the next block in free "
						   "list: %p vs %p\n", i, freeBlock, nextFree));
					BA_PANIC("Bad area free list.");
					return false;
				}
				// previous free block
				if (freeBlock->GetPreviousFreeBlock() != prevFree) {
					FATAL(("Previous free block of block %ld was not the "
						   " previous block in free list: %p vs %p\n", i,
						   freeBlock->GetPreviousFreeBlock(), prevFree));
					BA_PANIC("Bad area free list.");
					return false;
				}
				prevFree = freeBlock;
				nextFree = freeBlock->GetNextFreeBlock();
			} else
				usedBlockCount++;
			prevBlock = block;
			block = block->GetNextBlock();
			blockListOK = true;
		}
		// final checks on block list
		if (blockListOK) {
			if (block) {
				FATAL(("More blocks in block list than expected\n"));
				BA_PANIC("Bad area block count.");
				return false;
			} else if (fLastBlock != prevBlock) {
				FATAL(("last block in block list was %p, but should be "
					   "%p\n", fLastBlock, prevBlock));
				BA_PANIC("Bad area last block.");
				return false;
			} else if (prevFree != fLastFree) {
				FATAL(("last block in free list was %p, but should be %p\n",
					   fLastFree, prevFree));
				BA_PANIC("Bad area last free block.");
				return false;
			}
			// block counts (a bit reduntant)
			if (freeBlockCount != fFreeBlockCount) {
				FATAL(("Free block count is %ld, but should be %ld\n",
					   fFreeBlockCount, freeBlockCount));
				BA_PANIC("Bad area free block count.");
				return false;
			}
			if (usedBlockCount != fUsedBlockCount) {
				FATAL(("Used block count is %ld, but should be %ld\n",
					   fUsedBlockCount, usedBlockCount));
				BA_PANIC("Bad area used block count.");
				return false;
			}
			// free bytes
			if (fFreeBytes != freeBytes) {
				FATAL(("Free bytes is %lu, but should be %lu\n",
					   fFreeBytes, freeBytes));
				BA_PANIC("Bad area free bytes.");
				return false;
			}
		}
	}
	return true;
}

// CheckBlock
bool
BlockAllocator::Area::CheckBlock(Block *checkBlock, size_t minSize)
{
	for (Block *block = fFirstBlock; block; block = block->GetNextBlock()) {
		if (block == checkBlock)
			return (block->GetUsableSize() >= minSize);
	}
	FATAL(("Block %p is not in area %p!\n", checkBlock, this));
	BA_PANIC("Invalid Block.");
	return false;
}

// _FindFreeBlock
TFreeBlock *
BlockAllocator::Area::_FindFreeBlock(size_t minSize)
{
	// first fit
	for (TFreeBlock *block = GetFirstFreeBlock();
		 block;
		 block = block->GetNextFreeBlock()) {
		if (block->GetSize() >= minSize)
			return block;
	}
	return NULL;
}

// _InsertFreeBlock
void
BlockAllocator::Area::_InsertFreeBlock(TFreeBlock *block)
{
	if (block) {
		// find the free block before which this one has to be inserted
		TFreeBlock *nextFree = NULL;
		for (nextFree = GetFirstFreeBlock();
			 nextFree;
			 nextFree = nextFree->GetNextFreeBlock()) {
			if ((uint32)nextFree > (uint32)block)
				break;
		}
		// get the previous block and insert the block between the two
		TFreeBlock *prevFree
			= (nextFree ? nextFree->GetPreviousFreeBlock() : fLastFree);
		_FixFreeList(block, prevFree, nextFree);
		fFreeBlockCount++;
	}
}

// _RemoveFreeBlock
void
BlockAllocator::Area::_RemoveFreeBlock(TFreeBlock *block)
{
	if (block) {
		TFreeBlock *prevFree = block->GetPreviousFreeBlock();
		TFreeBlock *nextFree = block->GetNextFreeBlock();
		if (prevFree)
			prevFree->SetNextFreeBlock(nextFree);
		else
			fFirstFree = nextFree;
		if (nextFree)
			nextFree->SetPreviousFreeBlock(prevFree);
		else
			fLastFree = prevFree;
	}
	fFreeBlockCount--;
}

// _MoveResizeFreeBlock
TFreeBlock *
BlockAllocator::Area::_MoveResizeFreeBlock(TFreeBlock *freeBlock,
										   ssize_t offset, size_t newSize)
{
	TFreeBlock *movedFree = NULL;
	if (freeBlock && offset) {
		// move the header of the free block
		Block *freePrev = freeBlock->GetPreviousBlock();
		TFreeBlock *prevFree = freeBlock->GetPreviousFreeBlock();
		TFreeBlock *nextFree = freeBlock->GetNextFreeBlock();
		movedFree = _MakeFreeBlock(freeBlock, offset, freePrev, newSize,
			freeBlock->HasNextBlock(), prevFree, nextFree);
		// update the free list
		_FixFreeList(movedFree, prevFree, nextFree);
	}
	return movedFree;
}

// _MakeFreeBlock
inline
TFreeBlock *
BlockAllocator::Area::_MakeFreeBlock(void *address, ssize_t offset,
									 Block *previous, size_t size,
									 bool hasNext, TFreeBlock *previousFree,
									 TFreeBlock *nextFree)
{
	TFreeBlock *block = (TFreeBlock*)((char*)address + offset);
	block->SetTo(previous, size, hasNext, previousFree, nextFree);
	if (hasNext)
		block->GetNextBlock()->SetPreviousBlock(block);
	else
		fLastBlock = block;
	return block;
}

// _CoalesceWithNext
bool
BlockAllocator::Area::_CoalesceWithNext(TFreeBlock *block)
{
	bool result = false;
	TFreeBlock *nextFree = NULL;
	if (block && (nextFree = block->GetNextFreeBlock()) != NULL
		&& block->GetNextBlock() == nextFree) {
		_RemoveFreeBlock(nextFree);
		Block *nextBlock = nextFree->GetNextBlock();
		block->SetSize(block->GetSize() + nextFree->GetSize(), nextBlock);
		if (nextBlock)
			nextBlock->SetPreviousBlock(block);
		else
			fLastBlock = block;
		result = true;
	}
	return result;
}

// _MakeUsedBlock
inline
Block *
BlockAllocator::Area::_MakeUsedBlock(void *address, ssize_t offset,
									 Block *previous, size_t size,
									 bool hasNext)
{
	Block *block = (Block*)((char*)address + offset);
	block->SetTo(previous, size, false, hasNext, NULL);
	if (hasNext)
		block->GetNextBlock()->SetPreviousBlock(block);
	else
		fLastBlock = block;
	return block;
}

// _Defragment
void
BlockAllocator::Area::_Defragment()
{
D(SanityCheck());
//PRINT(("BlockAllocator::Area::_Defragment()\n"));
	// A trivial strategy for now: Keep the last free block and move the
	// others so that they can be joined with it. This is done iteratively
	// by moving the first free block to adjoin to the second one and
	// coalescing them. A free block is moved by moving the data blocks in
	// between.
	TFreeBlock *nextFree = NULL;
	while (fFirstFree && (nextFree = fFirstFree->GetNextFreeBlock()) != NULL) {
		Block *prevBlock = fFirstFree->GetPreviousBlock();
		Block *nextBlock = fFirstFree->GetNextBlock();
		size_t size = fFirstFree->GetSize();
		// Used blocks are relatively position independed. We can move them
		// en bloc and only need to adjust the previous pointer of the first
		// one.
		if (!nextBlock->IsFree()) {
			// move the used blocks
			size_t chunkSize = (char*)nextFree - (char*)nextBlock;
			Block *nextFreePrev = nextFree->GetPreviousBlock();
			Block *movedBlock = fFirstFree;
			memmove(movedBlock, nextBlock, chunkSize);
			movedBlock->SetPreviousBlock(prevBlock);
			// init the first free block
			Block *movedNextFreePrev = (Block*)((char*)nextFreePrev - size);
			fFirstFree = _MakeFreeBlock(movedBlock, chunkSize,
				movedNextFreePrev, size, true, NULL, nextFree);
			nextFree->SetPreviousFreeBlock(fFirstFree);
			// fix the references of the moved blocks
			for (Block *block = movedBlock;
				 block != fFirstFree;
				 block = block->GetNextBlock()) {
				block->FixReference();
			}
		} else {
			// uncoalesced adjoining free block: That should never happen,
			// since we always coalesce as early as possible.
			INFORM(("Warning: Found uncoalesced adjoining free blocks!\n"));
		}
		// coalesce the first two blocks
D(SanityCheck());
		_CoalesceWithNext(fFirstFree);
D(SanityCheck());
	}
//D(SanityCheck());
//PRINT(("BlockAllocator::Area::_Defragment() done\n"));
}

