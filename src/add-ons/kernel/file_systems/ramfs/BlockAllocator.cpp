// BlockAllocator.cpp

// debugging
#define BA_DEFINE_INLINES	1

#include "AllocationInfo.h"
#include "BlockAllocator.h"
#include "BlockAllocatorArea.h"
#include "BlockAllocatorAreaBucket.h"
#include "Debug.h"


// BlockAllocator

// constructor
BlockAllocator::BlockAllocator(size_t areaSize)
	: fReferenceManager(),
	  fBuckets(NULL),
	  fBucketCount(0),
	  fAreaSize(areaSize),
	  fAreaCount(0),
	  fFreeBytes(0)
{
	// create and init buckets
	fBucketCount = bucket_containing_size(areaSize) + 1;
	fBuckets = new(nothrow) AreaBucket[fBucketCount];
	size_t minSize = 0;
	for (int32 i = 0; i < fBucketCount; i++) {
		size_t maxSize = (1 << i) * kMinNetBlockSize;
		fBuckets[i].SetIndex(i);
		fBuckets[i].SetSizeLimits(minSize, maxSize);
		minSize = maxSize;
	}
}

// destructor
BlockAllocator::~BlockAllocator()
{
	if (fBuckets)
		delete[] fBuckets;
}

// InitCheck
status_t
BlockAllocator::InitCheck() const
{
	RETURN_ERROR(fBuckets ? B_OK : B_NO_MEMORY);
}

// AllocateBlock
BlockReference *
BlockAllocator::AllocateBlock(size_t usableSize)
{
#if ENABLE_BA_PANIC
if (fPanic)
	return NULL;
#endif
//PRINT(("BlockAllocator::AllocateBlock(%lu)\n", usableSize));
	Block *block = NULL;
	if (usableSize > 0 && usableSize <= Area::GetMaxFreeBytesFor(fAreaSize)) {
		// get a block reference
		BlockReference *reference = fReferenceManager.AllocateReference();
		if (reference) {
			block = _AllocateBlock(usableSize);
			// set reference / cleanup on failure
			if (block)
				block->SetReference(reference);
			else
				fReferenceManager.FreeReference(reference);
		}
		D(SanityCheck(false));
	}
//PRINT(("BlockAllocator::AllocateBlock() done: %p\n", block));
	return (block ? block->GetReference() : NULL);
}

// FreeBlock
void
BlockAllocator::FreeBlock(BlockReference *blockReference)
{
#if ENABLE_BA_PANIC
if (fPanic)
	return;
#endif
D(if (!CheckBlock(blockReference)) return;);
	Block *block = (blockReference ? blockReference->GetBlock() : NULL);
//PRINT(("BlockAllocator::FreeBlock(%p)\n", block));
	Area *area = NULL;
	if (block && !block->IsFree() && (area = _AreaForBlock(block)) != NULL) {
		_FreeBlock(area, block, true);
		D(SanityCheck(false));
		if (_DefragmentingRecommended())
			_Defragment();
	}
//PRINT(("BlockAllocator::FreeBlock() done\n"));
}

// ResizeBlock
BlockReference *
BlockAllocator::ResizeBlock(BlockReference *blockReference, size_t usableSize)
{
#if ENABLE_BA_PANIC
if (fPanic)
	return NULL;
#endif
D(if (!CheckBlock(blockReference)) return NULL;);
//PRINT(("BlockAllocator::ResizeBlock(%p, %lu)\n", blockReference, usableSize));
	Block *block = (blockReference ? blockReference->GetBlock() : NULL);
	Block *resultBlock = NULL;
	Area *area = NULL;
	if (block && !block->IsFree() && (area = _AreaForBlock(block)) != NULL) {
//PRINT(("BlockAllocator::ResizeBlock(%p, %lu)\n", block, usableSize));
		if (usableSize) {
			// try to let the area resize the block
			size_t blockSize = block->GetSize();
			size_t areaFreeBytes = area->GetFreeBytes();
			bool needsDefragmenting = area->NeedsDefragmenting();
//PRINT(("  block reference: %p / %p\n", blockReference, block->GetReference()));
			resultBlock = area->ResizeBlock(block, usableSize);
			block = blockReference->GetBlock();
			if (resultBlock) {
//PRINT(("  area succeeded in resizing the block\n"));
//PRINT(("  block reference now: %p\n", resultBlock->GetReference()));
				// the area was able to resize the block
				_RethinkAreaBucket(area, area->GetBucket(),
								   needsDefragmenting);
				fFreeBytes = fFreeBytes + area->GetFreeBytes() - areaFreeBytes;
				// Defragment only, if the area was able to resize the block,
				// the new block is smaller than the old one and defragmenting
				// is recommended.
				if (blockSize > resultBlock->GetSize()
					&& _DefragmentingRecommended()) {
					_Defragment();
				}
			} else {
//PRINT(("  area failed to resize the block\n"));
				// the area failed: allocate a new block, copy the data, and
				// free the old one
				resultBlock = _AllocateBlock(usableSize);
				block = blockReference->GetBlock();
				if (resultBlock) {
					memcpy(resultBlock->GetData(), block->GetData(),
						   block->GetUsableSize());
					resultBlock->SetReference(block->GetReference());
					_FreeBlock(area, block, false);
				}
			}
		} else
			FreeBlock(blockReference);
		D(SanityCheck(false));
//PRINT(("BlockAllocator::ResizeBlock() done: %p\n", resultBlock));
	}
	return (resultBlock ? resultBlock->GetReference() : NULL);
}

// SanityCheck
bool
BlockAllocator::SanityCheck(bool deep) const
{
	// iterate through all areas of all buckets
	int32 areaCount = 0;
	size_t freeBytes = 0;
	for (int32 i = 0; i < fBucketCount; i++) {
		AreaBucket *bucket = fBuckets + i;
		if (deep) {
			if (!bucket->SanityCheck(deep))
				return false;
		}
		for (Area *area = bucket->GetFirstArea();
			 area;
			 area = bucket->GetNextArea(area)) {
			areaCount++;
			freeBytes += area->GetFreeBytes();
		}
	}
	// area count
	if (areaCount != fAreaCount) {
		FATAL(("fAreaCount is %ld, but should be %ld\n", fAreaCount,
			   areaCount));
		BA_PANIC("BlockAllocator: Bad free bytes.");
		return false;
	}
	// free bytes
	if (fFreeBytes != freeBytes) {
		FATAL(("fFreeBytes is %lu, but should be %lu\n", fFreeBytes,
			   freeBytes));
		BA_PANIC("BlockAllocator: Bad free bytes.");
		return false;
	}
	return true;
}

// CheckArea
bool
BlockAllocator::CheckArea(Area *checkArea)
{
	for (int32 i = 0; i < fBucketCount; i++) {
		AreaBucket *bucket = fBuckets + i;
		for (Area *area = bucket->GetFirstArea();
			 area;
			 area = bucket->GetNextArea(area)) {
			if (area == checkArea)
				return true;
		}
	}
	FATAL(("Area %p is not a valid Area!\n", checkArea));
	BA_PANIC("Invalid Area.");
	return false;
}

// CheckBlock
bool
BlockAllocator::CheckBlock(Block *block, size_t minSize)
{
	Area *area = _AreaForBlock(block);
	return (area/* && CheckArea(area)*/ && area->CheckBlock(block, minSize));
}

// CheckBlock
bool
BlockAllocator::CheckBlock(BlockReference *reference, size_t minSize)
{
	return (fReferenceManager.CheckReference(reference)
			&& CheckBlock(reference->GetBlock(), minSize));
}

// GetAllocationInfo
void
BlockAllocator::GetAllocationInfo(AllocationInfo &info)
{
	fReferenceManager.GetAllocationInfo(info);
	info.AddOtherAllocation(sizeof(AreaBucket), fBucketCount);
	info.AddAreaAllocation(fAreaSize, fAreaCount);
}

// _AreaForBlock
inline
BlockAllocator::Area *
BlockAllocator::_AreaForBlock(Block *block)
{
	Area *area = NULL;
	area_id id = area_for(block);
	area_info info;
	if (id >= 0 && get_area_info(id, &info) == B_OK)
		area = (Area*)info.address;
D(if (!CheckArea(area)) return NULL;);
	return area;
}

// _AllocateBlock
Block *
BlockAllocator::_AllocateBlock(size_t usableSize, bool dontCreateArea)
{
	Block *block = NULL;
	// Get the last area (the one with the most free space) and try
	// to let it allocate a block. If that fails, allocate a new area.
	// find a bucket for the allocation
// TODO: optimize
	AreaBucket *bucket = NULL;
	int32 index = bucket_containing_min_size(usableSize);
	for (; index < fBucketCount; index++) {
		if (!fBuckets[index].IsEmpty()) {
			bucket = fBuckets + index;
			break;
		}
	}
	// get an area: if we have one, from the bucket, else create a new
	// area
	Area *area = NULL;
	if (bucket)
		area = bucket->GetFirstArea();
	else if (!dontCreateArea) {
		area = Area::Create(fAreaSize);
		if (area) {
			fAreaCount++;
			fFreeBytes += area->GetFreeBytes();
			bucket = fBuckets + area->GetBucketIndex();
			bucket->AddArea(area);
PRINT(("New area allocated. area count now: %ld, free bytes: %lu\n",
fAreaCount, fFreeBytes));
		}
	}
	// allocate a block
	if (area) {
		size_t areaFreeBytes = area->GetFreeBytes();
		bool needsDefragmenting = area->NeedsDefragmenting();
		block = area->AllocateBlock(usableSize);
		// move the area into another bucket, if necessary
		if (block) {
			_RethinkAreaBucket(area, bucket, needsDefragmenting);
			fFreeBytes = fFreeBytes + area->GetFreeBytes() - areaFreeBytes;
		}
#if ENABLE_BA_PANIC
else if (!fPanic) {
FATAL(("Block allocation failed unexpectedly.\n"));
PRINT(("  usableSize: %lu, areaFreeBytes: %lu\n", usableSize, areaFreeBytes));
BA_PANIC("Block allocation failed unexpectedly.");
//block = area->AllocateBlock(usableSize);
}
#endif
	}
	return block;
}

// _FreeBlock
void
BlockAllocator::_FreeBlock(Area *area, Block *block, bool freeReference)
{
	size_t areaFreeBytes = area->GetFreeBytes();
	AreaBucket *bucket = area->GetBucket();
	bool needsDefragmenting = area->NeedsDefragmenting();
	// free the block and the block reference
	BlockReference *reference = block->GetReference();
	area->FreeBlock(block);
	if (reference && freeReference)
		fReferenceManager.FreeReference(reference);
	// move the area into another bucket, if necessary
	_RethinkAreaBucket(area, bucket, needsDefragmenting);
	fFreeBytes = fFreeBytes + area->GetFreeBytes() - areaFreeBytes;
}

// _RethinkAreaBucket
inline
void
BlockAllocator::_RethinkAreaBucket(Area *area, AreaBucket *bucket,
								   bool needsDefragmenting)
{
	AreaBucket *newBucket = fBuckets + area->GetBucketIndex();
	if (newBucket != bucket
		|| needsDefragmenting != area->NeedsDefragmenting()) {
		bucket->RemoveArea(area);
		newBucket->AddArea(area);
	}
}

// _DefragmentingRecommended
inline
bool
BlockAllocator::_DefragmentingRecommended()
{
	// Don't know, whether this makes much sense: We don't try to defragment,
	// when not at least a complete area could be deleted, and some tolerance
	// being left (a fixed value plus 1/32 of the used bytes).
	size_t usedBytes = fAreaCount * Area::GetMaxFreeBytesFor(fAreaSize)
					   - fFreeBytes;
	return (fFreeBytes > fAreaSize + kDefragmentingTolerance + usedBytes / 32);
}

// _Defragment
bool
BlockAllocator::_Defragment()
{
	bool success = false;
	// We try to empty the least populated area by moving its blocks to other
	// areas.
	if (fFreeBytes > fAreaSize) {
		// find the least populated area
		// find the bucket with the least populated areas
		AreaBucket *bucket = NULL;
		for (int32 i = fBucketCount - 1; i >= 0; i--) {
			if (!fBuckets[i].IsEmpty()) {
				bucket = fBuckets + i;
				break;
			}
		}
		// find the area in the bucket
		Area *area = NULL;
		if (bucket) {
			area = bucket->GetFirstArea();
			Area *bucketArea = area;
			while ((bucketArea = bucket->GetNextArea(bucketArea)) != NULL) {
				if (bucketArea->GetFreeBytes() > area->GetFreeBytes())
					area = bucketArea;
			}
		}
		if (area) {
			// remove the area from the bucket
			bucket->RemoveArea(area);
			fFreeBytes -= area->GetFreeBytes();
			// iterate through the blocks in the area and try to find a new
			// home for them
			success = true;
			while (Block *block = area->GetFirstUsedBlock()) {
				Block *newBlock = _AllocateBlock(block->GetUsableSize(), true);
				if (newBlock) {
					// got a new block: copy the data to it and free the old
					// one
					memcpy(newBlock->GetData(), block->GetData(),
						   block->GetUsableSize());
					newBlock->SetReference(block->GetReference());
					block->SetReference(NULL);
					area->FreeBlock(block, true);
#if ENABLE_BA_PANIC
					if (fPanic) {
						PRINT(("Panicked while trying to free block %p\n",
							   block));
						success = false;
						break;
					}
#endif
				} else {
					success = false;
					break;
				}
			}
			// delete the area
			if (success && area->IsEmpty()) {
				area->Delete();
				fAreaCount--;
PRINT(("defragmenting: area deleted\n"));
			} else {
PRINT(("defragmenting: failed to empty area\n"));
				// failed: re-add the area
				fFreeBytes += area->GetFreeBytes();
				AreaBucket *newBucket = fBuckets + area->GetBucketIndex();
				newBucket->AddArea(area);
			}
		}
		D(SanityCheck(false));
	}
	return success;
}

#if ENABLE_BA_PANIC
bool BlockAllocator::fPanic = false;
#endif
