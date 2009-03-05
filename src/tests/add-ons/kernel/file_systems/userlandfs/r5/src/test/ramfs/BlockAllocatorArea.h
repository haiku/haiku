// BlockAllocatorArea.h

#ifndef BLOCK_ALLOCATOR_AREA_H
#define BLOCK_ALLOCATOR_AREA_H

#include <util/DoublyLinkedList.h>

#include "BlockAllocator.h"
#include "BlockAllocatorMisc.h"


class BlockAllocator::Area : public DoublyLinkedListLinkImpl<Area> {
public:
	static Area *Create(size_t size);
	void Delete();

	inline void SetBucket(AreaBucket *bucket)		{ fBucket = bucket; }
	inline AreaBucket *GetBucket() const			{ return fBucket; }

	inline Block *GetFirstBlock() const				{ return fFirstBlock; }
	inline Block *GetLastBlock() const				{ return fLastBlock; }

	inline TFreeBlock *GetFirstFreeBlock() const	{ return fFirstFree; }
	inline TFreeBlock *GetLastFreeBlock() const		{ return fLastFree; }

	inline bool IsEmpty() const				{ return (fUsedBlockCount == 0); }
	inline Block *GetFirstUsedBlock() const;

	static inline size_t GetMaxFreeBytesFor(size_t areaSize);

	inline size_t GetFreeBytes() const		{ return fFreeBytes; }
	inline bool NeedsDefragmenting() const	{ return (fFreeBlockCount > 1); }

	inline int32 GetBucketIndex();

	Block *AllocateBlock(size_t usableSize, bool dontDefragment = false);
	void FreeBlock(Block *block, bool dontDefragment = false);
	Block *ResizeBlock(Block *block, size_t newSize,
					   bool dontDefragment = false);

	// debugging only
	bool SanityCheck() const;
	bool CheckBlock(Block *block, size_t minSize = 0);

private:
	inline size_t _GetBlockFreeBytes()
		{ return fFreeBytes + sizeof(BlockHeader); }

	Area(area_id id, size_t size);
	~Area();

	inline void _FixBlockList(Block *block, Block *prevBlock,
							  Block *nextBlock);
	inline void _FixFreeList(TFreeBlock *block, TFreeBlock *prevFree,
							 TFreeBlock *nextFree);

	TFreeBlock *_FindFreeBlock(size_t minSize);
	void _InsertFreeBlock(TFreeBlock *block);
	void _RemoveFreeBlock(TFreeBlock *block);
	TFreeBlock * _MoveResizeFreeBlock(TFreeBlock *freeBlock, ssize_t offset,
									  size_t newSize);
	inline TFreeBlock *_MakeFreeBlock(void *address, ssize_t offset,
		Block *previous, size_t size, bool hasNext, TFreeBlock *previousFree,
		TFreeBlock *nextFree);
	bool _CoalesceWithNext(TFreeBlock *block);

	inline Block *_MakeUsedBlock(void *address, ssize_t offset,
								 Block *previous, size_t size, bool hasNext);

	inline bool _DefragmentingRecommended();
	void _Defragment();

private:
	AreaBucket			*fBucket;
	area_id				fID;
	size_t				fSize;
	size_t				fFreeBytes;
	size_t				fFreeBlockCount;
	size_t				fUsedBlockCount;
	Block				*fFirstBlock;
	Block				*fLastBlock;
	TFreeBlock			*fFirstFree;
	TFreeBlock			*fLastFree;
};

typedef BlockAllocator::Area Area;


// inline methods

// debugging
#if BA_DEFINE_INLINES

// GetFirstUsedBlock
inline
Block *
BlockAllocator::Area::GetFirstUsedBlock() const
{
	// Two assumptions:
	// 1) There is always a first block. If that isn't so, our structure are
	//    corrupt.
	// 2) If the first block is free, the second (if any) is not. Otherwise
	//    there were adjoining free blocks, which our coalescing strategy
	//    prevents.
	return (fFirstBlock->IsFree() ? fFirstBlock->GetNextBlock() : fFirstBlock);
}

// GetMaxFreeBytesFor
inline
size_t
BlockAllocator::Area::GetMaxFreeBytesFor(size_t areaSize)
{
	size_t headerSize = block_align_ceil(sizeof(Area));
	return Block::GetUsableSizeFor(block_align_floor(areaSize - headerSize));
}

// GetBucketIndex
inline
int32
BlockAllocator::Area::GetBucketIndex()
{
	return bucket_containing_size(GetFreeBytes());
}

// _FixBlockList
inline
void
BlockAllocator::Area::_FixBlockList(Block *block, Block *prevBlock,
									Block *nextBlock)
{
	if (block) {
		if (prevBlock)
			prevBlock->SetNextBlock(block);
		else
			fFirstBlock = block;
		if (nextBlock)
			nextBlock->SetPreviousBlock(block);
		else
			fLastBlock = block;
	}
}

// _FixFreeList
inline
void
BlockAllocator::Area::_FixFreeList(TFreeBlock *block, TFreeBlock *prevFree,
								   TFreeBlock *nextFree)
{
	if (block) {
		if (prevFree)
			prevFree->SetNextFreeBlock(block);
		else
			fFirstFree = block;
		if (nextFree)
			nextFree->SetPreviousFreeBlock(block);
		else
			fLastFree = block;
		block->SetPreviousFreeBlock(prevFree);
		block->SetNextFreeBlock(nextFree);
	}
}

// _DefragmentingRecommended
inline
bool
BlockAllocator::Area::_DefragmentingRecommended()
{
	// Defragmenting Condition: At least more than 5 free blocks and
	// free / block ratio greater 1 / 10. Don't know, if that makes any
	// sense. ;-)
	return (fFreeBlockCount > 5 && fUsedBlockCount / fFreeBlockCount < 10);
}

#endif	// BA_DEFINE_INLINES


#endif	// BLOCK_ALLOCATOR_AREA_H
