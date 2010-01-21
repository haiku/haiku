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


struct HashedObjectCache : ObjectCache {
								HashedObjectCache();

	static	HashedObjectCache*	Create(const char* name, size_t object_size,
									size_t alignment, size_t maximum,
									uint32 flags, void* cookie,
									object_cache_constructor constructor,
									object_cache_destructor destructor,
									object_cache_reclaimer reclaimer);
	virtual	void				Delete();

	virtual	slab*				CreateSlab(uint32 flags);
	virtual	void				ReturnSlab(slab* slab, uint32 flags);
	virtual slab*				ObjectSlab(void* object) const;

	virtual	status_t			PrepareObject(slab* source, void* object,
									uint32 flags);
	virtual	void				UnprepareObject(slab* source, void* object,
									uint32 flags);

private:
			struct Link {
				const void*	buffer;
				slab*		parent;
				Link*		next;
			};

			struct Definition {
				typedef HashedObjectCache	ParentType;
				typedef const void*			KeyType;
				typedef Link				ValueType;

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
					return (((const uint8*)key) - ((const uint8*)0))
						>> parent->lower_boundary;
				}

				size_t Hash(Link* value) const
				{
					return HashKey(value->buffer);
				}

				bool Compare(const void* key, Link* value) const
				{
					return value->buffer == key;
				}

				Link*& GetLink(Link* value) const
				{
					return value->next;
				}

				HashedObjectCache*	parent;
			};

			typedef BOpenHashTable<Definition> HashTable;

			friend class Definition;

private:
	static	Link*				_AllocateLink(uint32 flags);
	static	void				_FreeLink(HashedObjectCache::Link* link,
									uint32 flags);

private:
			HashTable hash_table;
			size_t lower_boundary;
};



#endif	// HASHED_OBJECT_CACHE_H
