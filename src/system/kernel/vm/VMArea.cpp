/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <vm/VMArea.h>

#include <heap.h>


#define AREA_HASH_TABLE_SIZE 1024


rw_lock VMAreaHash::sLock = RW_LOCK_INITIALIZER("area hash");
VMAreaHashTable VMAreaHash::sTable;
static area_id sNextAreaID = 1;


// #pragma mark - VMArea

VMArea::VMArea(VMAddressSpace* addressSpace, uint32 wiring, uint32 protection)
	:
	name(NULL),
	protection(protection),
	wiring(wiring),
	memory_type(0),
	cache(NULL),
	no_cache_change(0),
	cache_offset(0),
	cache_type(0),
	page_protections(NULL),
	address_space(addressSpace),
	cache_next(NULL),
	cache_prev(NULL),
	hash_next(NULL)
{
	new (&mappings) VMAreaMappings;
}


VMArea::~VMArea()
{
	free(page_protections);
	free(name);
}


status_t
VMArea::Init(const char* name)
{
	// restrict the area name to B_OS_NAME_LENGTH
	size_t length = strlen(name) + 1;
	if (length > B_OS_NAME_LENGTH)
		length = B_OS_NAME_LENGTH;

	// clone the name
	this->name = (char*)malloc_nogrow(length);
	if (this->name == NULL)
		return B_NO_MEMORY;
	strlcpy(this->name, name, length);

	id = atomic_add(&sNextAreaID, 1);
	return B_OK;
}


// #pragma mark - VMAreaHash


/*static*/ status_t
VMAreaHash::Init()
{
	return sTable.Init(AREA_HASH_TABLE_SIZE);
}


/*static*/ VMArea*
VMAreaHash::Lookup(area_id id)
{
	ReadLock();
	VMArea* area = LookupLocked(id);
	ReadUnlock();
	return area;
}


/*static*/ area_id
VMAreaHash::Find(const char* name)
{
	ReadLock();

	area_id id = B_NAME_NOT_FOUND;

	// TODO: Iterating through the whole table can be very slow and the whole
	// time we're holding the lock! Use a second hash table!

	for (VMAreaHashTable::Iterator it = sTable.GetIterator();
			VMArea* area = it.Next();) {
		if (strcmp(area->name, name) == 0) {
			id = area->id;
			break;
		}
	}

	ReadUnlock();

	return id;
}


/*static*/ void
VMAreaHash::Insert(VMArea* area)
{
	WriteLock();
	sTable.Insert(area);
	WriteUnlock();
}


/*static*/ void
VMAreaHash::Remove(VMArea* area)
{
	WriteLock();
	sTable.Remove(area);
	WriteUnlock();
}
