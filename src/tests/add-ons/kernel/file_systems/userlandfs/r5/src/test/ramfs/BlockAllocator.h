// BlockAllocator.h

#ifndef BLOCK_ALLOCATOR_H
#define BLOCK_ALLOCATOR_H

#include <OS.h>

#include "Block.h"
#include "BlockReferenceManager.h"
#include "Debug.h"
#include "List.h"

#define ENABLE_BA_PANIC	1
#if ENABLE_BA_PANIC
#define BA_PANIC(x)	{ PANIC(x); BlockAllocator::fPanic = true; }
#endif

class AllocationInfo;

// BlockAllocator
class BlockAllocator {
public:
	BlockAllocator(size_t areaSize);
	~BlockAllocator();

	status_t InitCheck() const;

	BlockReference *AllocateBlock(size_t usableSize);
	void FreeBlock(BlockReference *block);
	BlockReference *ResizeBlock(BlockReference *block, size_t usableSize);

	size_t GetAvailableBytes() const	{ return fAreaCount * fAreaSize; }
	size_t GetFreeBytes() const			{ return fFreeBytes; }
	size_t GetUsedBytes() const			{ return fAreaCount * fAreaSize
												 - fFreeBytes; }

public:
	class Area;
	class AreaBucket;

	// debugging only
	bool SanityCheck(bool deep = false) const;
	bool CheckArea(Area *area);
	bool CheckBlock(Block *block, size_t minSize = 0);
	bool CheckBlock(BlockReference *reference, size_t minSize = 0);
	void GetAllocationInfo(AllocationInfo &info);

private:
	inline Area *_AreaForBlock(Block *block);
	Block *_AllocateBlock(size_t usableSize, bool dontCreateArea = false);
	void _FreeBlock(Area *area, Block *block, bool freeReference);
	inline void _RethinkAreaBucket(Area *area, AreaBucket *bucket,
								   bool needsDefragmenting);
	inline bool _DefragmentingRecommended();
	bool _Defragment();

private:
	BlockReferenceManager		fReferenceManager;
	AreaBucket					*fBuckets;
	int32						fBucketCount;
	size_t						fAreaSize;
	int32						fAreaCount;
	size_t						fFreeBytes;
#if ENABLE_BA_PANIC
public:
	static bool					fPanic;
#endif
};

#endif	// BLOCK_ALLOCATOR_H
