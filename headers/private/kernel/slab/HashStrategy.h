/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#ifndef _SLAB_HASH_STRATEGY_H_
#define _SLAB_HASH_STRATEGY_H_

#include <slab/Strategy.h>
#include <slab/Utilities.h> // for TypedCache

#include <KernelExport.h>
#include <util/OpenHashTable.h>


struct BaseHashCacheStrategy {
	struct Link : cache_object_link, HashTableLink<Link> {
		cache_slab *slab;
		void *buffer;
	};

	struct HashTableDefinition {
		typedef BaseHashCacheStrategy	ParentType;
		typedef void *					KeyType;
		typedef Link					ValueType;

		HashTableDefinition(BaseHashCacheStrategy *_parent) : parent(_parent) {}

		size_t HashKey(void *key) const
		{
			return (((uint8_t *)key) - ((uint8_t *)0)) >> parent->fLowerBoundary;
		}

		size_t Hash(Link *value) const { return HashKey(value->buffer); }

		bool Compare(void *key, Link *value) const
		{
			return value->buffer == key;
		}

		HashTableLink<Link> *GetLink(Link *value) const { return value; }

		BaseHashCacheStrategy *parent;
	};

	// for g++ 2.95
	friend class HashTableDefinition;

	typedef OpenHashTable<HashTableDefinition> HashTable;

	static inline int
	__Fls0(size_t value)
	{
		if (value == 0)
			return -1;

		int bit;
		for (bit = 0; value != 1; bit++)
			value >>= 1;
		return bit;
	}

	BaseHashCacheStrategy(base_cache *parent)
		: fHashTable(this), fLowerBoundary(__Fls0(parent->object_size)) {}

	void *Object(cache_object_link *link) const
	{
		return ((Link *)link)->buffer;
	}

	CacheObjectInfo ObjectInformation(void *object) const
	{
		Link *link = _Linkage(object);
		return CacheObjectInfo(link->slab, link);
	}

protected:
	Link *_Linkage(void *object) const
	{
		Link *link = fHashTable.Lookup(object);
		if (link == NULL)
			panic("slab: missing buffer link from hash table.");
		return link;
	}

	static cache_object_link *_Linkage(void *_this, void *object)
	{
		return ((BaseHashCacheStrategy *)_this)->_Linkage(object);
	}

	HashTable fHashTable;
	const size_t fLowerBoundary;
};


template<typename Backend>
struct HashCacheStrategy : BaseCacheStrategy<Backend>, BaseHashCacheStrategy {
	typedef typename BaseCacheStrategy<Backend>::Slab Slab;
	typedef HashCacheStrategy<Backend> Strategy;

	HashCacheStrategy(base_cache *parent)
		: BaseCacheStrategy<Backend>(parent), BaseHashCacheStrategy(parent),
			fSlabCache("slab cache", 0), fLinkCache("link cache", 0) {}

	static size_t RequiredSpace(size_t objectSize)
	{
		return objectSize;
	}

	BaseSlab *NewSlab(uint32_t flags)
	{
		size_t byteCount = _SlabSize();

		Slab *slab = fSlabCache.Alloc(flags);
		if (slab == NULL)
			return NULL;

		void *pages;
		if (Backend::AllocatePages(fParent, &slab->id, &pages, byteCount,
				flags) < B_OK) {
			fSlabCache.Free(slab);
			return NULL;
		}

		if (_PrepareSlab(slab, pages, byteCount, flags) < B_OK) {
			Backend::FreePages(fParent, slab->id);
			fSlabCache.Free(slab);
			return NULL;
		}

		// it's very important that we cast this to BaseHashCacheStrategy
		// so we get the proper instance offset through void *
		return BaseCacheStrategy<Backend>::_ConstructSlab(slab, pages,
			_SlabSize(), _Linkage, (BaseHashCacheStrategy *)this);
	}

	void ReturnSlab(BaseSlab *slab)
	{
		_ClearSlab(slab->pages, _SlabSize());
		BaseCacheStrategy<Backend>::_DestructSlab(slab);
		fSlabCache.Free((Slab *)slab);
	}

private:
	size_t _SlabSize() const
	{
		return BaseCacheStrategy<Backend>::SlabSize(0);
	}

	status_t _PrepareSlab(Slab *slab, void *pages, size_t byteCount,
		uint32_t flags)
	{
		uint8_t *data = (uint8_t *)pages;
		for (uint8_t *it = data;
				it < (data + byteCount); it += fParent->object_size) {
			Link *link = fLinkCache.Alloc(flags);

			if (link == NULL) {
				_ClearSlabRange(data, it);
				return B_NO_MEMORY;
			}

			link->slab = slab;
			link->buffer = it;
			fHashTable.Insert(link);
		}

		return B_OK;
	}

	void _ClearSlab(void *pages, size_t size)
	{
		_ClearSlabRange((uint8_t *)pages, ((uint8_t *)pages) + size);
	}

	void _ClearSlabRange(uint8_t *data, uint8_t *end)
	{
		for (uint8_t *it = data; it < end; it += fParent->object_size) {
			Link *link = _Linkage(it);
			fHashTable.Remove(link);
			fLinkCache.Free(link);
		}
	}

	TypedCache<Slab, Backend> fSlabCache;
	TypedCache<Link, Backend> fLinkCache;
};

#endif
