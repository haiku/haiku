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
#include <vm/vm_priv.h>


#define AREA_HASH_TABLE_SIZE 1024


rw_lock VMAreaHash::sLock = RW_LOCK_INITIALIZER("area hash");
VMAreaHashTable VMAreaHash::sTable;
static area_id sNextAreaID = 1;


// #pragma mark - VMArea


/*static*/ VMArea*
VMArea::Create(VMAddressSpace* addressSpace, const char* name,
	uint32 wiring, uint32 protection)
{
	// restrict the area name to B_OS_NAME_LENGTH
	size_t length = strlen(name) + 1;
	if (length > B_OS_NAME_LENGTH)
		length = B_OS_NAME_LENGTH;

	VMArea* area = (VMArea*)malloc_nogrow(sizeof(VMArea));
	if (area == NULL)
		return NULL;

	area->name = (char*)malloc_nogrow(length);
	if (area->name == NULL) {
		free(area);
		return NULL;
	}
	strlcpy(area->name, name, length);

	area->id = atomic_add(&sNextAreaID, 1);
	area->fBase = 0;
	area->fSize = 0;
	area->protection = protection;
	area->wiring = wiring;
	area->memory_type = 0;

	area->cache = NULL;
	area->cache_offset = 0;

	area->address_space = addressSpace;
	area->cache_next = area->cache_prev = NULL;
	area->hash_next = NULL;
	new (&area->mappings) VMAreaMappings;
	area->page_protections = NULL;

	return area;
}


/*static*/ VMArea*
VMArea::CreateReserved(VMAddressSpace* addressSpace, uint32 flags)
{
	VMArea* reserved = (VMArea*)malloc_nogrow(sizeof(VMArea));
	if (reserved == NULL)
		return NULL;

	memset(reserved, 0, sizeof(VMArea));
	reserved->id = RESERVED_AREA_ID;
		// this marks it as reserved space
	reserved->protection = flags;
	reserved->address_space = addressSpace;

	return reserved;
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
		if (area->id == RESERVED_AREA_ID)
			continue;

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
