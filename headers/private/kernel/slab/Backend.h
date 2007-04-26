/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#ifndef _SLAB_BACKEND_H_
#define _SLAB_BACKEND_H_

#include <slab/Base.h>

extern "C" {
	status_t slab_area_backend_allocate(base_cache *cache, area_id *id,
		void **pages, size_t byte_count, uint32_t flags);
	void slab_area_backend_free(base_cache *cache, area_id id);
}

struct AreaBackend {
	typedef area_id AllocationID;

	static const size_t kPageSize = B_PAGE_SIZE;
	static const size_t kMaximumAlignedLength = B_PAGE_SIZE;

	static status_t AllocatePages(base_cache *cache, area_id *id, void **pages,
		size_t byteCount, uint32_t flags)
	{
		return slab_area_backend_allocate(cache, id, pages, byteCount, flags);
	}

	static void FreePages(base_cache *cache, area_id id)
	{
		return slab_area_backend_free(cache, id);
	}
};

#endif
