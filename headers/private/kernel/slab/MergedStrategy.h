/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#ifndef _SLAB_MERGED_STRATEGY_H_
#define _SLAB_MERGED_STRATEGY_H_

#include <slab/Strategy.h>


namespace Private {
	static inline const void *
	LowerBoundary(void *object, size_t byteCount)
	{
		const uint8_t *null = (uint8_t *)NULL;
		return null + ((((uint8_t *)object) - null) & ~(byteCount - 1));
	}
}


// This slab strategy includes the ObjectLink at the end of each object and the
// slab at the end of the allocated pages. It uses aligned allocations to
// provide object to slab mapping with zero storage, thus there is only one
// word of overhead per object. This is optimized for small objects.
template<typename Backend>
class MergedLinkCacheStrategy : public BaseCacheStrategy<Backend> {
public:
	typedef typename BaseCacheStrategy<Backend>::Slab Slab;
	typedef cache_object_link Link;

	MergedLinkCacheStrategy(base_cache *parent)
		: BaseCacheStrategy<Backend>(parent) {}

	static size_t RequiredSpace(size_t objectSize)
	{
		return objectSize + sizeof(Link);
	}

	void *Object(Link *link) const
	{
		return ((uint8_t *)link) - (fParent->object_size - sizeof(Link));
	}

	CacheObjectInfo ObjectInformation(void *object) const
	{
		Slab *slab = _SlabInPages(Private::LowerBoundary(object, _SlabSize()));
		return CacheObjectInfo(slab, _Linkage(object));
	}

	BaseSlab *NewSlab(uint32_t flags)
	{
		typename Backend::AllocationID id;
		void *pages;

		size_t byteCount = _SlabSize();
		// in order to save a pointer per object or a hash table to
		// map objects to slabs we required this set of pages to be
		// aligned in a (pageCount * PAGE_SIZE) boundary.
		if (Backend::AllocatePages(fParent, &id, &pages, byteCount,
				CACHE_ALIGN_TO_TOTAL | flags) < B_OK)
			return NULL;

		_SlabInPages(pages)->id = id;

		return BaseCacheStrategy<Backend>::_ConstructSlab(_SlabInPages(pages),
			pages, _SlabSize() - sizeof(Slab), _Linkage, this);
	}

	void ReturnSlab(BaseSlab *slab)
	{
		BaseCacheStrategy<Backend>::_DestructSlab(slab);
	}

private:
	size_t _SlabSize() const
	{
		size_t byteCount = BaseCacheStrategy<Backend>::SlabSize(sizeof(Slab));
		if (byteCount > Backend::kMaximumAlignedLength)
			byteCount = Backend::kMaximumAlignedLength;
		return byteCount;
	}

	Link *_Linkage(void *object) const
	{
		return (Link *)(((uint8_t *)object)
			+ (fParent->object_size - sizeof(Link)));
	}

	Slab *_SlabInPages(const void *pages) const
	{
		return (Slab *)(((uint8_t *)pages) + _SlabSize() - sizeof(Slab));
	}

	static Link *_Linkage(void *_this, void *object)
	{
		return ((MergedLinkCacheStrategy *)_this)->_Linkage(object);
	}
};

#endif
