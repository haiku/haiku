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
	struct Link : HashTableLink<Link> {
		const void *buffer;
		cache_slab *slab;
	};

	struct HashTableDefinition {
		typedef BaseHashCacheStrategy	ParentType;
		typedef const void *			KeyType;
		typedef Link					ValueType;

		HashTableDefinition(BaseHashCacheStrategy *_parent) : parent(_parent) {}

		size_t HashKey(const void *key) const
		{
			return (((const uint8_t *)key)
				- ((const uint8_t *)0)) >> parent->fLowerBoundary;
		}

		size_t Hash(Link *value) const { return HashKey(value->buffer); }

		bool Compare(const void *key, Link *value) const
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

	cache_slab *ObjectSlab(void *object) const
	{
		return _Linkage(object)->slab;
	}

protected:
	Link *_Linkage(void *object) const
	{
		Link *link = fHashTable.Lookup(object);
		if (link == NULL)
			panic("slab: missing buffer link from hash table.");
		return link;
	}

	HashTable fHashTable;
	const size_t fLowerBoundary;
};


template<typename Backend>
struct HashCacheStrategy : BaseCacheStrategy<Backend>, BaseHashCacheStrategy {
	typedef typename BaseCacheStrategy<Backend>::BaseSlab BaseSlab;
	typedef typename BaseCacheStrategy<Backend>::Slab Slab;
	typedef HashCacheStrategy<Backend> Strategy;

	HashCacheStrategy(base_cache *parent)
		: BaseCacheStrategy<Backend>(parent), BaseHashCacheStrategy(parent),
			fSlabCache("slab cache", 0), fLinkCache("link cache", 0) {}

	BaseSlab *NewSlab(uint32_t flags)
	{
		size_t byteCount = _SlabSize();

		Slab *slab = fSlabCache.Alloc(flags);
		if (slab == NULL)
			return NULL;

		void *pages;
		if (Backend::AllocatePages(Parent(), &slab->id, &pages, byteCount,
				flags) < B_OK) {
			fSlabCache.Free(slab);
			return NULL;
		}

		// it's very important that we cast this to BaseHashCacheStrategy
		// so we get the proper instance offset through void *
		cache_slab *result = BaseCacheStrategy<Backend>::_ConstructSlab(slab,
			pages, _SlabSize(), this, _PrepareObject, _UnprepareObject);
		if (result == NULL) {
			Backend::FreePages(Parent(), slab->id);
			fSlabCache.Free(slab);
		}

		return result;
	}

	void ReturnSlab(BaseSlab *slab)
	{
		BaseCacheStrategy<Backend>::_DestructSlab(slab, this, _UnprepareObject);
		fSlabCache.Free((Slab *)slab);
	}

private:
	size_t _SlabSize() const
	{
		return BaseCacheStrategy<Backend>::SlabSize(0);
	}

	base_cache *Parent() const { return BaseCacheStrategy<Backend>::Parent(); }

	static status_t _PrepareObject(void *_self, cache_slab *slab, void *object)
	{
		Strategy *self = (Strategy *)_self;

		Link *link = self->fLinkCache.Alloc(CACHE_DONT_SLEEP);
		if (link == NULL)
			return B_NO_MEMORY;

		link->slab = slab;
		link->buffer = object;

		self->fHashTable.Insert(link);

		return B_OK;
	}

	static void _UnprepareObject(void *_self, cache_slab *slab, void *object)
	{
		((Strategy *)_self)->_UnprepareObject(object);
	}

	void _UnprepareObject(void *object)
	{
		Link *link = _Linkage(object);
		fHashTable.Remove(link);
		fLinkCache.Free(link);
	}

	TypedCache<Slab, Backend> fSlabCache;
	TypedCache<Link, Backend> fLinkCache;
};

#endif
