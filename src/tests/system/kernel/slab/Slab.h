/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#ifndef _SLAB_H_
#define _SLAB_H_

#include <stdint.h>
#include <stdlib.h>

#include <algorithm> // for swap()
#include <new>
#include <utility> // for pair<>

#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>

#include <OS.h>


#define smp_get_current_cpu() 0
#define smp_get_num_cpus() 1


// C interface
extern "C" {
	typedef void *object_cache_t;
	object_cache_t
	object_cache_create(const char *name, size_t object_size, size_t alignment,
		void (*_constructor)(void *, void *), void (*_destructor)(void *, void *),
		void *cookie);
	void *object_cache_alloc(object_cache_t cache);
	void *object_cache_alloc_etc(object_cache_t cache, uint32_t flags);
	void object_cache_free(object_cache_t cache, void *object);
	void object_cache_destroy(object_cache_t cache);
}


// TODO this values should be dynamically tuned per cache.
static const int kMinimumSlabItems = 32;

// base Slab implementation, opaque to the backend used.
class BaseCache {
public:
	typedef void (*Constructor)(void *cookie, void *object);
	typedef void (*Destructor)(void *cookie, void *object);

	BaseCache(const char *_name, size_t objectSize, size_t alignment,
		Constructor _constructor, Destructor _destructor, void *_cookie);

	struct ObjectLink {
		struct ObjectLink *next;
	};

	struct Slab : DoublyLinkedListLinkImpl<Slab> {
		void *pages;
		size_t count, size;
		ObjectLink *free;
	};

	typedef std::pair<Slab *, ObjectLink *> ObjectInfo;

	ObjectLink *AllocateObject();
	bool ReturnObject(const ObjectInfo &object);

	Slab *ConstructSlab(Slab *slab, void *pages, size_t byteCount,
		ObjectLink *(*getLink)(void *parent, void *object), void *parent);
	void DestructSlab(Slab *slab);

	const char *Name() const { return fName; }
	size_t ObjectSize() const { return fObjectSize; }

protected:
	typedef DoublyLinkedList<Slab> SlabList;

	char fName[32];
	size_t fObjectSize, fCacheColorCycle;
	SlabList fPartialSlabs, fFullSlabs;

	Constructor fConstructor;
	Destructor fDestructor;
	void *fCookie;
};


enum {
	CACHE_ALIGN_TO_PAGE_TOTAL	= 1 << 0,
};

struct MallocBackend {
	typedef void *AllocationID;

	static int PageSize() { return 4096; }
	static status_t AllocatePages(BaseCache *cache, AllocationID *id,
		void **pages, size_t byteCount, uint32_t flags);
	static void FreePages(BaseCache *cache, void *pages);
};


struct AreaBackend {
	typedef area_id AllocationID;

	static int PageSize() { return B_PAGE_SIZE; }
	static status_t AllocatePages(BaseCache *cache, area_id *id, void **pages,
		size_t byteCount, uint32_t flags);
	static void FreePages(BaseCache *cache, area_id id);
};


// Slab implementation, glues together the frontend, backend as
// well as the Slab strategy used.
template<typename Strategy>
class Cache : protected BaseCache {
public:
	Cache(const char *_name, size_t objectSize, size_t alignment,
		Constructor _constructor, Destructor _destructor, void *_cookie)
		: BaseCache(_name, Strategy::RequiredSpace(objectSize), alignment,
			_constructor, _destructor, _cookie), fStrategy(this) {}

	void *AllocateObject(uint32_t flags)
	{
		if (fPartialSlabs.IsEmpty()) {
			Slab *newSlab = fStrategy.NewSlab(flags);
			if (newSlab == NULL)
				return NULL;
			fPartialSlabs.Add(newSlab);
		}

		return fStrategy.Object(BaseCache::AllocateObject());
	}

	void ReturnObject(void *object)
	{
		ObjectInfo location = fStrategy.ObjectInformation(object);

		if (BaseCache::ReturnObject(location))
			fStrategy.ReturnSlab(location.first);
	}

private:
	Strategy fStrategy;
};


static inline const void *
LowerBoundary(void *object, size_t byteCount)
{
	const uint8_t *null = (uint8_t *)NULL;
	return null + ((((uint8_t *)object) - null) & ~(byteCount - 1));
}


template<typename Backend>
class BaseCacheStrategy {
protected:
	typedef BaseCache::ObjectLink ObjectLink;

	BaseCacheStrategy(BaseCache *parent)
		: fParent(parent) {}

	size_t SlabSize(size_t tailSpace) const
	{
		size_t pageCount = (kMinimumSlabItems * fParent->ObjectSize()
			+ tailSpace) / Backend::PageSize();
		if (pageCount < 1)
			pageCount = 1;
		return pageCount * Backend::PageSize();
	}

	struct Slab : BaseCache::Slab {
		typename Backend::AllocationID id;
	};

	BaseCache::Slab *_ConstructSlab(Slab *slab, void *pages, size_t tailSpace,
		ObjectLink *(*getLink)(void *parent, void *object), void *parent)
	{
		return fParent->ConstructSlab(slab, pages, SlabSize(tailSpace)
			- tailSpace, getLink, parent);
	}

	void _DestructSlab(BaseCache::Slab *slab)
	{
		fParent->DestructSlab(slab);
		Backend::FreePages(fParent, ((Slab *)slab)->id);
	}

	BaseCache *fParent;
};


// This slab strategy includes the ObjectLink at the end of each object and the
// slab at the end of the allocated pages. It uses aligned allocations to
// provide object to slab mapping with zero storage, thus there is only one
// word of overhead per object. This is optimized for small objects.
template<typename Backend>
class MergedLinkCacheStrategy : public BaseCacheStrategy<Backend> {
public:
	typedef typename BaseCacheStrategy<Backend>::Slab Slab;
	typedef BaseCache::ObjectLink Link;
	typedef BaseCache::ObjectInfo ObjectInfo;

	MergedLinkCacheStrategy(BaseCache *parent)
		: BaseCacheStrategy<Backend>(parent) {}

	static size_t RequiredSpace(size_t objectSize)
	{
		return objectSize + sizeof(Link);
	}

	void *Object(Link *link) const
	{
		return ((uint8_t *)link) - (fParent->ObjectSize() - sizeof(Link));
	}

	ObjectInfo ObjectInformation(void *object) const
	{
		Slab *slab = _SlabInPages(LowerBoundary(object, _SlabSize()));
		return ObjectInfo(slab, _Linkage(object));
	}

	BaseCache::Slab *NewSlab(uint32_t flags)
	{
		typename Backend::AllocationID id;
		void *pages;

		// in order to save a pointer per object or a hash table to
		// map objects to slabs we required this set of pages to be
		// aligned in a (pageCount * PAGE_SIZE) boundary.
		if (Backend::AllocatePages(fParent, &id, &pages, _SlabSize(),
				CACHE_ALIGN_TO_PAGE_TOTAL | flags) < B_OK)
			return NULL;

		_SlabInPages(pages)->id = id;

		return BaseCacheStrategy<Backend>::_ConstructSlab(_SlabInPages(pages),
			pages, sizeof(Slab), _Linkage, this);
	}

	void ReturnSlab(BaseCache::Slab *slab)
	{
		BaseCacheStrategy<Backend>::_DestructSlab(slab);
	}

private:
	size_t _SlabSize() const
	{
		return BaseCacheStrategy<Backend>::SlabSize(sizeof(Slab));
	}

	Link *_Linkage(void *object) const
	{
		return (Link *)(((uint8_t *)object)
			+ (fParent->ObjectSize() - sizeof(Link)));
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


template<typename Type, typename Backend>
class TypedCache : public Cache<MergedLinkCacheStrategy<Backend> > {
public:
	typedef MergedLinkCacheStrategy<Backend> Strategy;
	typedef Cache<Strategy> BaseType;

	TypedCache(const char *name, size_t alignment)
		: BaseType(name, sizeof(Type), alignment, _ConstructObject,
			_DestructObject, this) {}
	virtual ~TypedCache() {}

	Type *Alloc(uint32_t flags) { return (Type *)BaseType::AllocateObject(flags); }
	void Free(Type *object) { BaseType::ReturnObject(object); }

private:
	static void _ConstructObject(void *cookie, void *object)
	{
		((TypedCache *)cookie)->ConstructObject((Type *)object);
	}

	static void _DestructObject(void *cookie, void *object)
	{
		((TypedCache *)cookie)->DestructObject((Type *)object);
	}

	virtual void ConstructObject(Type *object) {}
	virtual void DestructObject(Type *object) {}
};


static inline int
Fls(size_t value)
{
	for (int i = 31; i >= 0; i--) {
		if ((value >> i) & 1)
			return i + 1;
	}

	return -1;
}


struct BaseHashCacheStrategy {
	typedef BaseCache::ObjectLink ObjectLink;
	typedef BaseCache::ObjectInfo ObjectInfo;

	struct Link : ObjectLink, HashTableLink<Link> {
		BaseCache::Slab *slab;
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

	BaseHashCacheStrategy(BaseCache *parent)
		: fHashTable(this), fLowerBoundary(Fls(parent->ObjectSize()) - 1) {}

	void *Object(ObjectLink *link) const
	{
		return ((Link *)link)->buffer;
	}

	ObjectInfo ObjectInformation(void *object) const
	{
		Link *link = _Linkage(object);
		return ObjectInfo(link->slab, link);
	}

protected:
	Link *_Linkage(void *object) const
	{
		return fHashTable.Lookup(object);
	}

	static ObjectLink *_Linkage(void *_this, void *object)
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

	HashCacheStrategy(BaseCache *parent)
		: BaseCacheStrategy<Backend>(parent), BaseHashCacheStrategy(parent),
			fSlabCache("slab cache", 0), fLinkCache("link cache", 0) {}

	static size_t RequiredSpace(size_t objectSize)
	{
		return objectSize;
	}

	BaseCache::Slab *NewSlab(uint32_t flags)
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

		if (_PrepareSlab(fParent, slab, pages, byteCount, flags) < B_OK) {
			Backend::FreePages(fParent, slab->id);
			fSlabCache.Free(slab);
			return NULL;
		}

		// it's very important that we cast this to BaseHashCacheStrategy
		// so we get the proper instance offset through void *
		return BaseCacheStrategy<Backend>::_ConstructSlab(slab, pages, 0,
			_Linkage, (BaseHashCacheStrategy *)this);
	}

	void ReturnSlab(BaseCache::Slab *slab)
	{
		_ClearSlab(fParent, slab->pages, _SlabSize());
		BaseCacheStrategy<Backend>::_DestructSlab(slab);
		fSlabCache.Free((Slab *)slab);
	}

private:
	size_t _SlabSize() const
	{
		return BaseCacheStrategy<Backend>::SlabSize(0);
	}

	status_t _PrepareSlab(BaseCache *parent, Slab *slab, void *pages,
		size_t byteCount, uint32_t flags)
	{
		uint8_t *data = (uint8_t *)pages;
		for (uint8_t *it = data;
				it < (data + byteCount); it += parent->ObjectSize()) {
			Link *link = fLinkCache.Alloc(flags);

			if (link == NULL) {
				_ClearSlabRange(parent, data, it);
				return B_NO_MEMORY;
			}

			link->slab = slab;
			link->buffer = it;
			fHashTable.Insert(link);
		}

		return B_OK;
	}

	void _ClearSlab(BaseCache *parent, void *pages, size_t byteCount)
	{
		_ClearSlabRange(parent, (uint8_t *)pages,
			((uint8_t *)pages) + byteCount);
	}

	void _ClearSlabRange(BaseCache *parent, uint8_t *data, uint8_t *end)
	{
		for (uint8_t *it = data; it < end; it += parent->ObjectSize()) {
			Link *link = fHashTable.Lookup(it);
			fHashTable.Remove(link);
			fLinkCache.Free(link);
		}
	}

	TypedCache<Slab, Backend> fSlabCache;
	TypedCache<Link, Backend> fLinkCache;
};


class BaseDepot {
public:
	struct Magazine {
		Magazine *next;
		uint16_t current_round, round_count;
		void *rounds[0];
	};

	struct CPUStore {
		benaphore lock;
		Magazine *loaded, *previous;
	};

protected:
	BaseDepot();
	virtual ~BaseDepot();

	status_t InitCheck() const;

	CPUStore *CPU() const { return &fStores[smp_get_current_cpu()]; }

	void *ObtainFromStore(CPUStore *store);
	bool ReturnToStore(CPUStore *store, void *object);
	void MakeEmpty();

	virtual void ReturnObject(void *object) = 0;

	bool _ExchangeWithFull(Magazine* &magazine);
	bool _ExchangeWithEmpty(Magazine* &magazine);
	void _EmptyMagazine(Magazine *magazine);

	Magazine *_AllocMagazine();
	void _FreeMagazine(Magazine *magazine);

	benaphore fLock;
	Magazine *fFull, *fEmpty;
	size_t fFullCount, fEmptyCount;
	CPUStore *fStores;
};


template<typename CacheType>
class LocalCache : public CacheType, protected BaseDepot {
public:
	typedef typename CacheType::Constructor Constructor;
	typedef typename CacheType::Destructor Destructor;

	LocalCache(const char *name, size_t objectSize, size_t alignment,
		Constructor _constructor, Destructor _destructor, void *_cookie)
		: CacheType(name, objectSize, alignment, _constructor, _destructor,
			_cookie) {}

	~LocalCache() { Destroy(); }

	void *Alloc(uint32_t flags)
	{
		void *object = BaseDepot::ObtainFromStore(CPU());
		if (object == NULL)
			object = CacheType::AllocateObject(flags);
		return object;
	}

	void Free(void *object)
	{
		if (!BaseDepot::ReturnToStore(CPU(), object))
			CacheType::ReturnObject(object);
	}

	void Destroy() { BaseDepot::MakeEmpty(); }

private:
	void ReturnObject(void *object)
	{
		CacheType::ReturnObject(object);
	}
};

#endif
