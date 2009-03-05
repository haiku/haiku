// BlockAllocatorAreaBucket.h

#ifndef BLOCK_ALLOCATOR_AREA_BUCKET_H
#define BLOCK_ALLOCATOR_AREA_BUCKET_H

#include <util/DoublyLinkedList.h>

#include "BlockAllocator.h"
#include "BlockAllocatorArea.h"
#include "Debug.h"


class BlockAllocator::AreaBucket {
public:
	AreaBucket();
	~AreaBucket();

	inline void SetIndex(int32 index)	{ fIndex = index; }
	inline int32 GetIndex() const		{ return fIndex; }

	inline void SetSizeLimits(size_t minSize, size_t maxSize);
	inline size_t GetMinSize() const	{ return fMinSize; }	// incl.
	inline size_t GetMaxSize() const	{ return fMaxSize; }	// excl.

	inline void AddArea(Area *area);
	inline void RemoveArea(Area *area);

	inline Area *GetFirstArea() const	{ return fAreas.First(); }
	inline Area *GetLastArea() const	{ return fAreas.Last(); }
	inline Area *GetNextArea(Area* area) const;

	inline bool IsEmpty() const			{ return fAreas.IsEmpty(); }

	// debugging only
	bool SanityCheck(bool deep = false) const;

private:
	DoublyLinkedList<Area>	fAreas;
	int32			fIndex;
	size_t			fMinSize;
	size_t			fMaxSize;
};

typedef BlockAllocator::AreaBucket AreaBucket;


// inline methods

// debugging
#if BA_DEFINE_INLINES

// SetSizeLimits
/*!	\brief Sets the size limits for areas this bucket may contain.
	\param minSize Minimal area size. Inclusively.
	\param maxSize Maximal area size. Exlusively.
*/
inline
void
BlockAllocator::AreaBucket::SetSizeLimits(size_t minSize, size_t maxSize)
{
	fMinSize = minSize;
	fMaxSize = maxSize;
}

// AddArea
inline
void
BlockAllocator::AreaBucket::AddArea(Area *area)
{
	if (area) {
		fAreas.Insert(area, area->NeedsDefragmenting());
		area->SetBucket(this);
		D(SanityCheck(false));
	}
}

// RemoveArea
inline
void
BlockAllocator::AreaBucket::RemoveArea(Area *area)
{
	if (area) {
		fAreas.Remove(area);
		area->SetBucket(NULL);
		D(SanityCheck(false));
	}
}

// GetNextArea
inline
Area *
BlockAllocator::AreaBucket::GetNextArea(Area* area) const
{
	return fAreas.GetNext(area);
}

#endif	// BA_DEFINE_INLINES

#endif	// BLOCK_ALLOCATOR_AREA_BUCKET_H
