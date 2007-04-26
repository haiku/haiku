/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#ifndef _SLAB_STRATEGY_H_
#define _SLAB_STRATEGY_H_

#include <slab/Base.h>


template<typename Backend>
class BaseCacheStrategy {
protected:
	typedef cache_object_link	ObjectLink;
	typedef cache_slab			BaseSlab;

	BaseCacheStrategy(base_cache *parent)
		: fParent(parent) {}

	size_t SlabSize(size_t tailSpace) const
	{
		size_t pageCount = (kMinimumSlabItems * fParent->object_size
			+ tailSpace) / Backend::kPageSize;
		if (pageCount < 1)
			pageCount = 1;
		return pageCount * Backend::kPageSize;
	}

	struct Slab : BaseSlab {
		typename Backend::AllocationID id;
	};

	BaseSlab *_ConstructSlab(Slab *slab, void *pages, size_t byteCount,
		ObjectLink *(*getLink)(void *parent, void *object), void *parent)
	{
		return base_cache_construct_slab(fParent, slab, pages, byteCount,
			getLink, parent);
	}

	void _DestructSlab(BaseSlab *slab)
	{
		base_cache_destruct_slab(fParent, slab);
		Backend::FreePages(fParent, ((Slab *)slab)->id);
	}

	base_cache *Parent() const { return fParent; }

	base_cache *fParent;
};

#endif
