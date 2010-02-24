/*
 * Copyright 2008, Axel Dörfler. All Rights Reserved.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#ifndef HASHED_OBJECT_CACHE_H
#define HASHED_OBJECT_CACHE_H


#include <util/OpenHashTable.h>

#include "ObjectCache.h"
#include "slab_private.h"


struct HashedSlab : slab {
	HashedSlab*	hash_next;
};


struct HashedObjectCache : ObjectCache {
								HashedObjectCache();

	static	HashedObjectCache*	Create(const char* name, size_t object_size,
									size_t alignment, size_t maximum,
									size_t magazineCapacity,
									size_t maxMagazineCount,
									uint32 flags, void* cookie,
									object_cache_constructor constructor,
									object_cache_destructor destructor,
									object_cache_reclaimer reclaimer);
	virtual	void				Delete();

	virtual	slab*				CreateSlab(uint32 flags);
	virtual	void				ReturnSlab(slab* slab, uint32 flags);
	virtual slab*				ObjectSlab(void* object) const;

private:
			struct Definition {
				typedef HashedObjectCache	ParentType;
				typedef const void*			KeyType;
				typedef HashedSlab			ValueType;

				Definition(HashedObjectCache* parent)
					:
					parent(parent)
				{
				}

				Definition(const Definition& definition)
					:
					parent(definition.parent)
				{
				}

				size_t HashKey(const void* key) const
				{
					return (addr_t)::lower_boundary(key, parent->slab_size)
						>> parent->lower_boundary;
				}

				size_t Hash(HashedSlab* value) const
				{
					return HashKey(value->pages);
				}

				bool Compare(const void* key, HashedSlab* value) const
				{
					return value->pages == key;
				}

				HashedSlab*& GetLink(HashedSlab* value) const
				{
					return value->hash_next;
				}

				HashedObjectCache*	parent;
			};

			struct InternalAllocator {
				void* Allocate(size_t size) const
				{
					return slab_internal_alloc(size, 0);
				}

				void Free(void* memory) const
				{
					slab_internal_free(memory, 0);
				}
			};

			typedef BOpenHashTable<Definition, false, false,
				InternalAllocator> HashTable;

			friend class Definition;

private:
			void				_ResizeHashTableIfNeeded(uint32 flags);

private:
			HashTable hash_table;
			size_t lower_boundary;
};



#endif	// HASHED_OBJECT_CACHE_H
