/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <vm/VMArea.h>

#include <vm/vm_priv.h>


#define AREA_HASH_TABLE_SIZE 1024


rw_lock VMAreaHash::sLock = RW_LOCK_INITIALIZER("area hash");
VMAreaHashTable VMAreaHash::sTable;


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
