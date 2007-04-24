/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#include "Slab.h"

#include <stdio.h>
#include <malloc.h>


// TODO this value should be dynamically tuned per cache.
static const int kMagazineCapacity = 32;

static const size_t kCacheColorPeriod = 8;


template<typename Type> static Type *
SListPop(Type* &head)
{
	Type *oldHead = head;
	head = head->next;
	return oldHead;
}


template<typename Type> static inline void
SListPush(Type* &head, Type *object)
{
	object->next = head;
	head = object;
}


status_t
MallocBackend::AllocatePages(BaseCache *cache, AllocationID *id, void **pages,
	size_t byteCount, uint32_t flags)
{
	size_t alignment = 16;

	if (flags & CACHE_ALIGN_TO_PAGE_TOTAL)
		alignment = byteCount;

	*pages = memalign(alignment, byteCount);
	if (*pages == NULL)
		return B_NO_MEMORY;

	*id = *pages;
	return B_OK;
}

void
MallocBackend::FreePages(BaseCache *cache, void *pages)
{
	free(pages);
}


status_t
AreaBackend::AllocatePages(BaseCache *cache, area_id *id, void **pages,
	size_t byteCount, uint32_t flags)
{
	if (flags & CACHE_ALIGN_TO_PAGE_TOTAL)
		; // panic()

	area_id areaId = create_area(cache->Name(), pages, B_ANY_ADDRESS, //B_ANY_KERNEL_ADDRESS,
		byteCount, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (areaId < 0)
		return areaId;

	printf("AreaBackend::AllocatePages() = { %ld, %p }\n", areaId, *pages);

	*id = areaId;
	return B_OK;
}

void
AreaBackend::FreePages(BaseCache *cache, area_id area)
{
	printf("AreaBackend::DeletePages(%ld)\n", area);
	delete_area(area);
}


BaseCache::BaseCache(const char *_name, size_t objectSize, size_t alignment,
	Constructor _constructor, Destructor _destructor, void *_cookie)
	: fConstructor(_constructor), fDestructor(_destructor), fCookie(_cookie)
{
	strncpy(fName, _name, sizeof(fName));
	fName[sizeof(fName) - 1] = 0;

	if (alignment > 0 && (objectSize & (alignment - 1)))
		fObjectSize = objectSize + alignment - (objectSize & (alignment - 1));
	else
		fObjectSize = objectSize;

	fCacheColorCycle = 0;
}


BaseCache::ObjectLink *BaseCache::AllocateObject()
{
	Slab *slab = fPartialSlabs.Head();

	printf("BaseCache::AllocateObject() from %p, %lu remaining\n",
		slab, slab->count);

	ObjectLink *link = SListPop(slab->free);
	slab->count--;
	if (slab->count == 0) {
		// move the partial slab to the full list
		fPartialSlabs.Remove(slab);
		fFullSlabs.Add(slab);
	}

	return link;
}


bool
BaseCache::ReturnObject(const ObjectInfo &object)
{
	Slab *slab = object.first;
	ObjectLink *link = object.second;

	// We return true if the slab is completely unused.

	SListPush(slab->free, link);
	slab->count++;
	if (slab->count == slab->size) {
		fPartialSlabs.Remove(slab);
		return true;
	} else if (slab->count == 1) {
		fFullSlabs.Remove(slab);
		fPartialSlabs.Add(slab);
	}

	return false;
}


BaseCache::Slab *
BaseCache::ConstructSlab(Slab *slab, void *pages, size_t byteCount,
	ObjectLink *(*getLink)(void *parent, void *object), void *parent)
{
	printf("BaseCache::ConstructSlab(%p, %p, %lu, %p, %p)\n", slab, pages,
		byteCount, getLink, parent);

	slab->pages = pages;
	slab->count = slab->size = byteCount / fObjectSize;
	slab->free = NULL;

	size_t spareBytes = byteCount - (slab->size * fObjectSize);
	size_t cycle = fCacheColorCycle;

	if (cycle > spareBytes)
		cycle = 0;
	else
		fCacheColorCycle += kCacheColorPeriod;

	printf("  %lu objects, %lu spare bytes, cycle %lu\n",
		slab->size, spareBytes, cycle);

	uint8_t *data = ((uint8_t *)pages) + cycle;

	for (size_t i = 0; i < slab->size; i++) {
		if (fConstructor)
			fConstructor(fCookie, data);
		SListPush(slab->free, getLink(parent, data));
		data += fObjectSize;
	}

	return slab;
}


void
BaseCache::DestructSlab(Slab *slab)
{
	if (fDestructor == NULL)
		return;

	uint8_t *data = (uint8_t *)slab->pages;

	for (size_t i = 0; i < slab->size; i++) {
		fDestructor(fCookie, data);
		data += fObjectSize;
	}
}


static inline bool
_IsMagazineEmpty(BaseDepot::Magazine *magazine)
{
	return magazine->current_round == 0;
}


static inline bool
_IsMagazineFull(BaseDepot::Magazine *magazine)
{
	return magazine->current_round == magazine->round_count;
}


static inline void *
_PopMagazine(BaseDepot::Magazine *magazine)
{
	return magazine->rounds[--magazine->current_round];
}


static inline bool
_PushMagazine(BaseDepot::Magazine *magazine, void *object)
{
	if (_IsMagazineFull(magazine))
		return false;
	magazine->rounds[magazine->current_round++] = object;
	return true;
}


BaseDepot::BaseDepot()
	: fFull(NULL), fEmpty(NULL), fFullCount(0), fEmptyCount(0)
{
	// benaphore_init(...)
	fStores = new (std::nothrow) CPUStore[smp_get_num_cpus()];

	if (fStores) {
		for (int i = 0; i < smp_get_num_cpus(); i++) {
			// benaphore_init(...)
			fStores[i].loaded = fStores[i].previous = NULL;
		}
	}
}


BaseDepot::~BaseDepot()
{
	// MakeEmpty may not be used here as ReturnObject is
	// no longer available by then.

	delete [] fStores;

	// benaphore_destroy()
}


status_t
BaseDepot::InitCheck() const
{
	return fStores ? B_OK : B_NO_MEMORY;
}


void *
BaseDepot::ObtainFromStore(CPUStore *store)
{
	BenaphoreLocker _(store->lock);

	// To better understand both the Alloc() and Free() logic refer to
	// Bonwick's ``Magazines and Vmem'' [in 2001 USENIX proceedings]

	// In a nutshell, we try to get an object from the loaded magazine
	// if it's not empty, or from the previous magazine if it's full
	// and finally from the Slab if the magazine depot has no full magazines.

	if (store->loaded == NULL)
		return NULL;

	while (true) {
		if (!_IsMagazineEmpty(store->loaded))
			return _PopMagazine(store->loaded);

		if (store->previous && (_IsMagazineFull(store->previous)
			|| _ExchangeWithFull(store->previous)))
			std::swap(store->previous, store->loaded);
		else
			return NULL;
	}
}


bool
BaseDepot::ReturnToStore(CPUStore *store, void *object)
{
	BenaphoreLocker _(store->lock);

	// We try to add the object to the loaded magazine if we have one
	// and it's not full, or to the previous one if it is empty. If
	// the magazine depot doesn't provide us with a new empty magazine
	// we return the object directly to the slab.

	while (true) {
		if (store->loaded && _PushMagazine(store->loaded, object))
			return true;

		if ((store->previous && _IsMagazineEmpty(store->previous))
			|| _ExchangeWithEmpty(store->previous))
			std::swap(store->loaded, store->previous);
		else
			return false;
	}
}


void
BaseDepot::MakeEmpty()
{
	for (int i = 0; i < smp_get_num_cpus(); i++) {
		if (fStores[i].loaded)
			_EmptyMagazine(fStores[i].loaded);
		if (fStores[i].previous)
			_EmptyMagazine(fStores[i].previous);
		fStores[i].loaded = fStores[i].previous = NULL;
	}

	while (fFull)
		_EmptyMagazine(SListPop(fFull));

	while (fEmpty)
		_EmptyMagazine(SListPop(fEmpty));
}


bool
BaseDepot::_ExchangeWithFull(Magazine* &magazine)
{
	BenaphoreLocker _(fLock);

	if (fFull == NULL)
		return false;

	fFullCount--;
	fEmptyCount++;

	SListPush(fEmpty, magazine);
	magazine = SListPop(fFull);
	return true;
}


bool
BaseDepot::_ExchangeWithEmpty(Magazine* &magazine)
{
	BenaphoreLocker _(fLock);

	if (fEmpty == NULL) {
		fEmpty = _AllocMagazine();
		if (fEmpty == NULL)
			return false;
	} else {
		fEmptyCount--;
	}

	if (magazine) {
		SListPush(fFull, magazine);
		fFullCount++;
	}

	magazine = SListPop(fEmpty);
	return true;
}


void
BaseDepot::_EmptyMagazine(Magazine *magazine)
{
	for (uint16_t i = 0; i < magazine->current_round; i++)
		ReturnObject(magazine->rounds[i]);
	_FreeMagazine(magazine);
}


BaseDepot::Magazine *
BaseDepot::_AllocMagazine()
{
	Magazine *magazine = (Magazine *)malloc(sizeof(Magazine)
		+ kMagazineCapacity * sizeof(void *));
	if (magazine) {
		magazine->next = NULL;
		magazine->current_round = 0;
		magazine->round_count = kMagazineCapacity;
	}

	return magazine;
}


void
BaseDepot::_FreeMagazine(Magazine *magazine)
{
	free(magazine);
}



typedef MergedLinkCacheStrategy<MallocBackend> MallocMergedCacheStrategy;
typedef Cache<MallocMergedCacheStrategy> MallocMergedCache;
typedef LocalCache<MallocMergedCache> MallocLocalCache;

typedef HashCacheStrategy<MallocBackend> MallocHashCacheStrategy;
typedef Cache<MallocHashCacheStrategy> MallocHashCache;

object_cache_t
object_cache_create(const char *name, size_t object_size, size_t alignment,
	void (*_constructor)(void *, void *), void (*_destructor)(void *, void *),
	void *cookie)
{
	return new (std::nothrow) MallocLocalCache(name, object_size, alignment,
		_constructor, _destructor, cookie);
}


void *
object_cache_alloc(object_cache_t cache)
{
	return ((MallocLocalCache *)cache)->Alloc(0);
}


void *
object_cache_alloc_etc(object_cache_t cache, uint32_t flags)
{
	return ((MallocLocalCache *)cache)->Alloc(flags);
}


void
object_cache_free(object_cache_t cache, void *object)
{
	((MallocLocalCache *)cache)->Free(object);
}


void
object_cache_destroy(object_cache_t cache)
{
	delete (MallocLocalCache *)cache;
}


void test1()
{
	MallocLocalCache cache("foobar", sizeof(int), 0, NULL, NULL, NULL);

	static const int N = 4096;
	void *buf[N];

	for (int i = 0; i < N; i++)
		buf[i] = cache.Alloc(0);

	for (int i = 0; i < N; i++)
		cache.Free(buf[i]);

	cache.Destroy();
}

void test2()
{
	TypedCache<int, MallocBackend> cache("int cache", 0);

	static const int N = 4096;
	int *buf[N];

	for (int i = 0; i < N; i++)
		buf[i] = cache.Alloc(0);

	for (int i = 0; i < N; i++)
		cache.Free(buf[i]);
}

void test3()
{
	Cache<HashCacheStrategy<AreaBackend> > cache("512byte hash cache", 512, 0, NULL,
		NULL, NULL);

	static const int N = 128;
	void *buf[N];

	for (int i = 0; i < N; i++)
		buf[i] = cache.AllocateObject(0);

	for (int i = 0; i < N; i++)
		cache.ReturnObject(buf[i]);
}

void test4()
{
	LocalCache<MallocHashCache> cache("foobar", 512, 0, NULL, NULL, NULL);

	static const int N = 128;
	void *buf[N];

	for (int i = 0; i < N; i++)
		buf[i] = cache.Alloc(0);

	for (int i = 0; i < N; i++)
		cache.Free(buf[i]);

	cache.Destroy();
}

void test5()
{
	object_cache_t cache = object_cache_create("foobar", 16, 0,
		NULL, NULL, NULL);

	static const int N = 1024;
	void *buf[N];

	for (int i = 0; i < N; i++)
		buf[i] = object_cache_alloc(cache);

	for (int i = 0; i < N; i++)
		object_cache_free(cache, buf[i]);

	object_cache_destroy(cache);
}


int main()
{
	//test1();
	//test2();
	test3();
	//test4();
	//test5();
	return 0;
}
