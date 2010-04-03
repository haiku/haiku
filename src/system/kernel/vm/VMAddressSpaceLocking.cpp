/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "VMAddressSpaceLocking.h"

#include <AutoDeleter.h>

#include <vm/vm.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMArea.h>
#include <vm/VMCache.h>


//	#pragma mark - AddressSpaceLockerBase


/*static*/ VMAddressSpace*
AddressSpaceLockerBase::GetAddressSpaceByAreaID(area_id id)
{
	VMAddressSpace* addressSpace = NULL;

	VMAreaHash::ReadLock();

	VMArea* area = VMAreaHash::LookupLocked(id);
	if (area != NULL) {
		addressSpace = area->address_space;
		addressSpace->Get();
	}

	VMAreaHash::ReadUnlock();

	return addressSpace;
}


//	#pragma mark - AddressSpaceReadLocker


AddressSpaceReadLocker::AddressSpaceReadLocker(team_id team)
	:
	fSpace(NULL),
	fLocked(false)
{
	SetTo(team);
}


/*! Takes over the reference of the address space, if \a getNewReference is
	\c false.
*/
AddressSpaceReadLocker::AddressSpaceReadLocker(VMAddressSpace* space,
		bool getNewReference)
	:
	fSpace(NULL),
	fLocked(false)
{
	SetTo(space, getNewReference);
}


AddressSpaceReadLocker::AddressSpaceReadLocker()
	:
	fSpace(NULL),
	fLocked(false)
{
}


AddressSpaceReadLocker::~AddressSpaceReadLocker()
{
	Unset();
}


void
AddressSpaceReadLocker::Unset()
{
	Unlock();
	if (fSpace != NULL)
		fSpace->Put();
}


status_t
AddressSpaceReadLocker::SetTo(team_id team)
{
	fSpace = VMAddressSpace::Get(team);
	if (fSpace == NULL)
		return B_BAD_TEAM_ID;

	fSpace->ReadLock();
	fLocked = true;
	return B_OK;
}


/*! Takes over the reference of the address space, if \a getNewReference is
	\c false.
*/
void
AddressSpaceReadLocker::SetTo(VMAddressSpace* space, bool getNewReference)
{
	fSpace = space;

	if (getNewReference)
		fSpace->Get();

	fSpace->ReadLock();
	fLocked = true;
}


status_t
AddressSpaceReadLocker::SetFromArea(area_id areaID, VMArea*& area)
{
	fSpace = GetAddressSpaceByAreaID(areaID);
	if (fSpace == NULL)
		return B_BAD_TEAM_ID;

	fSpace->ReadLock();

	area = VMAreaHash::Lookup(areaID);

	if (area == NULL || area->address_space != fSpace) {
		fSpace->ReadUnlock();
		return B_BAD_VALUE;
	}

	fLocked = true;
	return B_OK;
}


bool
AddressSpaceReadLocker::Lock()
{
	if (fLocked)
		return true;
	if (fSpace == NULL)
		return false;

	fSpace->ReadLock();
	fLocked = true;

	return true;
}


void
AddressSpaceReadLocker::Unlock()
{
	if (fLocked) {
		fSpace->ReadUnlock();
		fLocked = false;
	}
}


//	#pragma mark - AddressSpaceWriteLocker


AddressSpaceWriteLocker::AddressSpaceWriteLocker(team_id team)
	:
	fSpace(NULL),
	fLocked(false),
	fDegraded(false)
{
	SetTo(team);
}


AddressSpaceWriteLocker::AddressSpaceWriteLocker(VMAddressSpace* space,
	bool getNewReference)
	:
	fSpace(NULL),
	fLocked(false),
	fDegraded(false)
{
	SetTo(space, getNewReference);
}


AddressSpaceWriteLocker::AddressSpaceWriteLocker()
	:
	fSpace(NULL),
	fLocked(false),
	fDegraded(false)
{
}


AddressSpaceWriteLocker::~AddressSpaceWriteLocker()
{
	Unset();
}


void
AddressSpaceWriteLocker::Unset()
{
	Unlock();
	if (fSpace != NULL)
		fSpace->Put();
}


status_t
AddressSpaceWriteLocker::SetTo(team_id team)
{
	fSpace = VMAddressSpace::Get(team);
	if (fSpace == NULL)
		return B_BAD_TEAM_ID;

	fSpace->WriteLock();
	fLocked = true;
	return B_OK;
}


void
AddressSpaceWriteLocker::SetTo(VMAddressSpace* space, bool getNewReference)
{
	fSpace = space;

	if (getNewReference)
		fSpace->Get();

	fSpace->WriteLock();
	fLocked = true;
}


status_t
AddressSpaceWriteLocker::SetFromArea(area_id areaID, VMArea*& area)
{
	fSpace = GetAddressSpaceByAreaID(areaID);
	if (fSpace == NULL)
		return B_BAD_VALUE;

	fSpace->WriteLock();

	area = VMAreaHash::Lookup(areaID);

	if (area == NULL || area->address_space != fSpace) {
		fSpace->WriteUnlock();
		return B_BAD_VALUE;
	}

	fLocked = true;
	return B_OK;
}


status_t
AddressSpaceWriteLocker::SetFromArea(team_id team, area_id areaID,
	bool allowKernel, VMArea*& area)
{
	VMAreaHash::ReadLock();

	area = VMAreaHash::LookupLocked(areaID);
	if (area != NULL
		&& (area->address_space->ID() == team
			|| (allowKernel && team == VMAddressSpace::KernelID()))) {
		fSpace = area->address_space;
		fSpace->Get();
	}

	VMAreaHash::ReadUnlock();

	if (fSpace == NULL)
		return B_BAD_VALUE;

	// Second try to get the area -- this time with the address space
	// write lock held

	fSpace->WriteLock();

	area = VMAreaHash::Lookup(areaID);

	if (area == NULL) {
		fSpace->WriteUnlock();
		return B_BAD_VALUE;
	}

	fLocked = true;
	return B_OK;
}


status_t
AddressSpaceWriteLocker::SetFromArea(team_id team, area_id areaID,
	VMArea*& area)
{
	return SetFromArea(team, areaID, false, area);
}


void
AddressSpaceWriteLocker::Unlock()
{
	if (fLocked) {
		if (fDegraded)
			fSpace->ReadUnlock();
		else
			fSpace->WriteUnlock();
		fLocked = false;
		fDegraded = false;
	}
}


void
AddressSpaceWriteLocker::DegradeToReadLock()
{
	fSpace->ReadLock();
	fSpace->WriteUnlock();
	fDegraded = true;
}


//	#pragma mark - MultiAddressSpaceLocker


MultiAddressSpaceLocker::MultiAddressSpaceLocker()
	:
	fItems(NULL),
	fCapacity(0),
	fCount(0),
	fLocked(false)
{
}


MultiAddressSpaceLocker::~MultiAddressSpaceLocker()
{
	Unset();
	free(fItems);
}


/*static*/ int
MultiAddressSpaceLocker::_CompareItems(const void* _a, const void* _b)
{
	lock_item* a = (lock_item*)_a;
	lock_item* b = (lock_item*)_b;
	return b->space->ID() - a->space->ID();
		// descending order, i.e. kernel address space last
}


bool
MultiAddressSpaceLocker::_ResizeIfNeeded()
{
	if (fCount == fCapacity) {
		lock_item* items = (lock_item*)realloc(fItems,
			(fCapacity + 4) * sizeof(lock_item));
		if (items == NULL)
			return false;

		fCapacity += 4;
		fItems = items;
	}

	return true;
}


int32
MultiAddressSpaceLocker::_IndexOfAddressSpace(VMAddressSpace* space) const
{
	for (int32 i = 0; i < fCount; i++) {
		if (fItems[i].space == space)
			return i;
	}

	return -1;
}


status_t
MultiAddressSpaceLocker::_AddAddressSpace(VMAddressSpace* space,
	bool writeLock, VMAddressSpace** _space)
{
	if (!space)
		return B_BAD_VALUE;

	int32 index = _IndexOfAddressSpace(space);
	if (index < 0) {
		if (!_ResizeIfNeeded()) {
			space->Put();
			return B_NO_MEMORY;
		}

		lock_item& item = fItems[fCount++];
		item.space = space;
		item.write_lock = writeLock;
	} else {

		// one reference is enough
		space->Put();

		fItems[index].write_lock |= writeLock;
	}

	if (_space != NULL)
		*_space = space;

	return B_OK;
}


void
MultiAddressSpaceLocker::Unset()
{
	Unlock();

	for (int32 i = 0; i < fCount; i++)
		fItems[i].space->Put();

	fCount = 0;
}


status_t
MultiAddressSpaceLocker::Lock()
{
	ASSERT(!fLocked);

	qsort(fItems, fCount, sizeof(lock_item), &_CompareItems);

	for (int32 i = 0; i < fCount; i++) {
		status_t status;
		if (fItems[i].write_lock)
			status = fItems[i].space->WriteLock();
		else
			status = fItems[i].space->ReadLock();

		if (status < B_OK) {
			while (--i >= 0) {
				if (fItems[i].write_lock)
					fItems[i].space->WriteUnlock();
				else
					fItems[i].space->ReadUnlock();
			}
			return status;
		}
	}

	fLocked = true;
	return B_OK;
}


void
MultiAddressSpaceLocker::Unlock()
{
	if (!fLocked)
		return;

	for (int32 i = 0; i < fCount; i++) {
		if (fItems[i].write_lock)
			fItems[i].space->WriteUnlock();
		else
			fItems[i].space->ReadUnlock();
	}

	fLocked = false;
}


/*!	Adds all address spaces of the areas associated with the given area's cache,
	locks them, and locks the cache (including a reference to it). It retries
	until the situation is stable (i.e. the neither cache nor cache's areas
	changed) or an error occurs.
*/
status_t
MultiAddressSpaceLocker::AddAreaCacheAndLock(area_id areaID,
	bool writeLockThisOne, bool writeLockOthers, VMArea*& _area,
	VMCache** _cache)
{
	// remember the original state
	int originalCount = fCount;
	lock_item* originalItems = NULL;
	if (fCount > 0) {
		originalItems = new(nothrow) lock_item[fCount];
		if (originalItems == NULL)
			return B_NO_MEMORY;
		memcpy(originalItems, fItems, fCount * sizeof(lock_item));
	}
	ArrayDeleter<lock_item> _(originalItems);

	// get the cache
	VMCache* cache;
	VMArea* area;
	status_t error;
	{
		AddressSpaceReadLocker locker;
		error = locker.SetFromArea(areaID, area);
		if (error != B_OK)
			return error;

		cache = vm_area_get_locked_cache(area);
	}

	while (true) {
		// add all areas
		VMArea* firstArea = cache->areas;
		for (VMArea* current = firstArea; current;
				current = current->cache_next) {
			error = AddArea(current->id,
				current == area ? writeLockThisOne : writeLockOthers);
			if (error != B_OK) {
				vm_area_put_locked_cache(cache);
				return error;
			}
		}

		// unlock the cache and attempt to lock the address spaces
		vm_area_put_locked_cache(cache);

		error = Lock();
		if (error != B_OK)
			return error;

		// lock the cache again and check whether anything has changed

		// check whether the area is gone in the meantime
		area = VMAreaHash::Lookup(areaID);

		if (area == NULL) {
			Unlock();
			return B_BAD_VALUE;
		}

		// lock the cache
		VMCache* oldCache = cache;
		cache = vm_area_get_locked_cache(area);

		// If neither the area's cache has changed nor its area list we're
		// done.
		if (cache == oldCache && firstArea == cache->areas) {
			_area = area;
			if (_cache != NULL)
				*_cache = cache;
			return B_OK;
		}

		// Restore the original state and try again.

		// Unlock the address spaces, but keep the cache locked for the next
		// iteration.
		Unlock();

		// Get an additional reference to the original address spaces.
		for (int32 i = 0; i < originalCount; i++)
			originalItems[i].space->Get();

		// Release all references to the current address spaces.
		for (int32 i = 0; i < fCount; i++)
			fItems[i].space->Put();

		// Copy over the original state.
		fCount = originalCount;
		if (originalItems != NULL)
			memcpy(fItems, originalItems, fCount * sizeof(lock_item));
	}
}


