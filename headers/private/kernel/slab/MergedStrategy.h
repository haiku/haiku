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


// This slab strategy includes the ObjectLink at the end of each object and the
// slab at the end of the allocated pages. It uses aligned allocations to
// provide object to slab mapping with zero storage, thus there is only one
// word of overhead per object. This is optimized for small objects.
template<typename Backend>
class MergedLinkCacheStrategy : public BaseCacheStrategy<Backend> {
public:
	typedef typename BaseCacheStrategy<Backend>::BaseSlab BaseSlab;
	typedef typename BaseCacheStrategy<Backend>::Slab Slab;

	MergedLinkCacheStrategy(base_cache *parent)
		: BaseCacheStrategy<Backend>(parent) {}

	static inline const void *
	LowerBoundary(void *object, size_t byteCount)
	{
		const uint8_t *null = (uint8_t *)NULL;
		return null + ((((uint8_t *)object) - null) & ~(byteCount - 1));
	}

	BaseSlab *ObjectSlab(void *object) const
	{
		return _SlabInPages(LowerBoundary(object, _SlabSize()));
	}

	BaseSlab *NewSlab(uint32_t flags)
	{
		typename Backend::AllocationID id;
		void *pages;

		size_t byteCount = _SlabSize();
		// in order to save a pointer per object or a hash table to
		// map objects to slabs we required this set of pages to be
		// aligned in a (pageCount * PAGE_SIZE) boundary.
		if (Backend::AllocatePages(Parent(), &id, &pages, byteCount,
				CACHE_ALIGN_TO_TOTAL | flags) < B_OK)
			return NULL;

		_SlabInPages(pages)->id = id;

		return BaseCacheStrategy<Backend>::_ConstructSlab(_SlabInPages(pages),
			pages, byteCount - sizeof(Slab), this, NULL, NULL);
	}

	void ReturnSlab(BaseSlab *slab)
	{
		BaseCacheStrategy<Backend>::_DestructSlab(slab, NULL, NULL);
	}

private:
	size_t _SlabSize() const
	{
		size_t byteCount = BaseCacheStrategy<Backend>::SlabSize(sizeof(Slab));
		if (byteCount > Backend::kMaximumAlignedLength)
			byteCount = Backend::kMaximumAlignedLength;
		return byteCount;
	}

	base_cache *Parent() const { return BaseCacheStrategy<Backend>::Parent(); }

	Slab *_SlabInPages(const void *pages) const
	{
		return (Slab *)(((uint8_t *)pages) + _SlabSize() - sizeof(Slab));
	}
};

#endif
