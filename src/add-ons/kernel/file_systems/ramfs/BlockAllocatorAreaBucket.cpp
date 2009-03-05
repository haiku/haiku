// BlockAllocatorAreaBucket.cpp

#include "BlockAllocatorAreaBucket.h"

// constructor
BlockAllocator::AreaBucket::AreaBucket()
	: fAreas(),
	  fIndex(-1),
	  fMinSize(0),
	  fMaxSize(0)
{
}

// destructor
BlockAllocator::AreaBucket::~AreaBucket()
{
	while (Area *area = fAreas.First()) {
		RemoveArea(area);
		area->Delete();
	}
}

// SanityCheck
bool
BlockAllocator::AreaBucket::SanityCheck(bool deep) const
{
	// check area list
	for (Area *area = GetFirstArea(); area; area = GetNextArea(area)) {
		if (deep) {
			if (!area->SanityCheck())
				return false;
		}
		// bucket
		if (area->GetBucket() != this) {
			FATAL(("Area %p is in bucket %p, but thinks it is in bucket %p\n",
				   area, this, area->GetBucket()));
			BA_PANIC("Wrong area bucket.");
			return false;
		}
		// size
		size_t areaSize = area->GetFreeBytes();
		if (areaSize < fMinSize || areaSize >= fMaxSize) {
			FATAL(("Area is in wrong bucket: free: %lu, min: %lu, max: %lu\n",
				   areaSize, fMinSize, fMaxSize));
			BA_PANIC("Area in wrong bucket.");
			return false;
		}
	}
	return true;
}

