/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <vm/VMAddressSpace.h>

#include <stdlib.h>

#include <KernelExport.h>

#include <util/OpenHashTable.h>

#include <heap.h>
#include <thread.h>
#include <vm/vm.h>
#include <vm/VMArea.h>


//#define TRACE_VM
#ifdef TRACE_VM
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define ASPACE_HASH_TABLE_SIZE 1024


/*!	Verifies that an area with the given aligned base and size fits into
	the spot defined by base and limit and checks for overflows.
*/
static inline bool
is_valid_spot(addr_t base, addr_t alignedBase, addr_t size, addr_t limit)
{
	return (alignedBase >= base && alignedBase + (size - 1) > alignedBase
		&& alignedBase + (size - 1) <= limit);
}


// #pragma mark - AddressSpaceHashDefinition


struct AddressSpaceHashDefinition {
	typedef team_id			KeyType;
	typedef VMAddressSpace	ValueType;

	size_t HashKey(team_id key) const
	{
		return key;
	}

	size_t Hash(const VMAddressSpace* value) const
	{
		return HashKey(value->ID());
	}

	bool Compare(team_id key, const VMAddressSpace* value) const
	{
		return value->ID() == key;
	}

	VMAddressSpace*& GetLink(VMAddressSpace* value) const
	{
		return value->HashTableLink();
	}
};

typedef BOpenHashTable<AddressSpaceHashDefinition> AddressSpaceTable;

static AddressSpaceTable	sAddressSpaceTable;
static rw_lock				sAddressSpaceTableLock;

VMAddressSpace* VMAddressSpace::sKernelAddressSpace;


// #pragma mark - VMAddressSpace


VMAddressSpace::VMAddressSpace(team_id id, addr_t base, size_t size,
	bool kernel)
	:
	fBase(base),
	fEndAddress(base + (size - 1)),
	fFreeSpace(size),
	fID(id),
	fRefCount(1),
	fFaultCount(0),
	fChangeCount(0),
	fAreaHint(NULL),
	fDeleting(false)
{
	rw_lock_init(&fLock, kernel ? "kernel address space" : "address space");
}


VMAddressSpace::~VMAddressSpace()
{
	if (this == sKernelAddressSpace)
		panic("deleting the kernel aspace!\n");

	TRACE(("VMAddressSpace::~VMAddressSpace: called on aspace %" B_PRId32 "\n",
		ID()));

	WriteLock();

	fTranslationMap.ops->destroy(&fTranslationMap);

	rw_lock_destroy(&fLock);
}


/*static*/ status_t
VMAddressSpace::Init()
{
	rw_lock_init(&sAddressSpaceTableLock, "address spaces table");

	// create the area and address space hash tables
	{
		new(&sAddressSpaceTable) AddressSpaceTable;
		status_t error = sAddressSpaceTable.Init(ASPACE_HASH_TABLE_SIZE);
		if (error != B_OK)
			panic("vm_init: error creating aspace hash table\n");
	}

	// create the initial kernel address space
	if (Create(B_SYSTEM_TEAM, KERNEL_BASE, KERNEL_SIZE, true,
			&sKernelAddressSpace) != B_OK) {
		panic("vm_init: error creating kernel address space!\n");
	}

	add_debugger_command("aspaces", &_DumpListCommand,
		"Dump a list of all address spaces");
	add_debugger_command("aspace", &_DumpCommand,
		"Dump info about a particular address space");

	return B_OK;
}


/*static*/ status_t
VMAddressSpace::InitPostSem()
{
	status_t status = arch_vm_translation_map_init_kernel_map_post_sem(
		&sKernelAddressSpace->fTranslationMap);
	if (status != B_OK)
		return status;

	return B_OK;
}


void
VMAddressSpace::Put()
{
	bool remove = false;

	rw_lock_write_lock(&sAddressSpaceTableLock);
	if (atomic_add(&fRefCount, -1) == 1) {
		sAddressSpaceTable.RemoveUnchecked(this);
		remove = true;
	}
	rw_lock_write_unlock(&sAddressSpaceTableLock);

	if (remove)
		delete this;
}


/*! Deletes all areas in the specified address space, and the address
	space by decreasing all reference counters. It also marks the
	address space of being in deletion state, so that no more areas
	can be created in it.
	After this, the address space is not operational anymore, but might
	still be in memory until the last reference has been released.
*/
void
VMAddressSpace::RemoveAndPut()
{
	WriteLock();
	fDeleting = true;
	WriteUnlock();

	vm_delete_areas(this);
	Put();
}


/*static*/ status_t
VMAddressSpace::Create(team_id teamID, addr_t base, size_t size, bool kernel,
	VMAddressSpace** _addressSpace)
{
	VMAddressSpace* addressSpace = new(nogrow) VMAddressSpace(teamID, base,
		size, kernel);
	if (addressSpace == NULL)
		return B_NO_MEMORY;

	TRACE(("vm_create_aspace: team %ld (%skernel): %#lx bytes starting at "
		"%#lx => %p\n", id, kernel ? "!" : "", size, base, addressSpace));

	// initialize the corresponding translation map
	status_t status = arch_vm_translation_map_init_map(
		&addressSpace->fTranslationMap, kernel);
	if (status != B_OK) {
		delete addressSpace;
		return status;
	}

	// add the aspace to the global hash table
	rw_lock_write_lock(&sAddressSpaceTableLock);
	sAddressSpaceTable.InsertUnchecked(addressSpace);
	rw_lock_write_unlock(&sAddressSpaceTableLock);

	*_addressSpace = addressSpace;
	return B_OK;
}


/*static*/ VMAddressSpace*
VMAddressSpace::GetKernel()
{
	// we can treat this one a little differently since it can't be deleted
	sKernelAddressSpace->Get();
	return sKernelAddressSpace;
}


/*static*/ team_id
VMAddressSpace::CurrentID()
{
	struct thread* thread = thread_get_current_thread();

	if (thread != NULL && thread->team->address_space != NULL)
		return thread->team->id;

	return B_ERROR;
}


/*static*/ VMAddressSpace*
VMAddressSpace::GetCurrent()
{
	struct thread* thread = thread_get_current_thread();

	if (thread != NULL) {
		VMAddressSpace* addressSpace = thread->team->address_space;
		if (addressSpace != NULL) {
			addressSpace->Get();
			return addressSpace;
		}
	}

	return NULL;
}


/*static*/ VMAddressSpace*
VMAddressSpace::Get(team_id teamID)
{
	rw_lock_read_lock(&sAddressSpaceTableLock);
	VMAddressSpace* addressSpace = sAddressSpaceTable.Lookup(teamID);
	if (addressSpace)
		addressSpace->Get();
	rw_lock_read_unlock(&sAddressSpaceTableLock);

	return addressSpace;
}


//! You must hold the address space's read lock.
VMArea*
VMAddressSpace::LookupArea(addr_t address) const
{
	// check the area hint first
	if (fAreaHint != NULL && fAreaHint->ContainsAddress(address))
		return fAreaHint;

	for (VMAddressSpaceAreaList::ConstIterator it = fAreas.GetIterator();
			VMArea* area = it.Next();) {
		if (area->id == RESERVED_AREA_ID)
			continue;

		if (area->ContainsAddress(address)) {
			fAreaHint = area;
			return area;
		}
	}

	return NULL;
}


/*!	This inserts the area you pass into the address space.
	It will also set the "_address" argument to its base address when
	the call succeeds.
	You need to hold the VMAddressSpace write lock.
*/
status_t
VMAddressSpace::InsertArea(void** _address, uint32 addressSpec, addr_t size,
	VMArea* area)
{
	addr_t searchBase, searchEnd;
	status_t status;

	switch (addressSpec) {
		case B_EXACT_ADDRESS:
			searchBase = (addr_t)*_address;
			searchEnd = (addr_t)*_address + (size - 1);
			break;

		case B_BASE_ADDRESS:
			searchBase = (addr_t)*_address;
			searchEnd = fEndAddress;
			break;

		case B_ANY_ADDRESS:
		case B_ANY_KERNEL_ADDRESS:
		case B_ANY_KERNEL_BLOCK_ADDRESS:
			searchBase = fBase;
			// TODO: remove this again when vm86 mode is moved into the kernel
			// completely (currently needs a userland address space!)
			if (searchBase == USER_BASE)
				searchBase = USER_BASE_ANY;
			searchEnd = fEndAddress;
			break;

		default:
			return B_BAD_VALUE;
	}

	status = _InsertAreaSlot(searchBase, size, searchEnd, addressSpec, area);
	if (status == B_OK) {
		*_address = (void*)area->Base();
		fFreeSpace -= area->Size();
	}

	return status;
}


//! You must hold the address space's write lock.
void
VMAddressSpace::RemoveArea(VMArea* area)
{
	fAreas.Remove(area);

	if (area->id != RESERVED_AREA_ID) {
		IncrementChangeCount();
		fFreeSpace += area->Size();

		if (area == fAreaHint)
			fAreaHint = NULL;
	}
}


bool
VMAddressSpace::CanResizeArea(VMArea* area, size_t newSize)
{
	VMArea* next = fAreas.GetNext(area);
	addr_t newEnd = area->Base() + (newSize - 1);
	if (next == NULL) {
		if (fEndAddress >= newEnd)
			return true;
	} else {
		if (next->Base() > newEnd)
			return true;
	}

	// If the area was created inside a reserved area, it can
	// also be resized in that area
	// TODO: if there is free space after the reserved area, it could
	// be used as well...
	if (next->id == RESERVED_AREA_ID && next->cache_offset <= area->Base()
		&& next->Base() + (next->Size() - 1) >= newEnd) {
		return true;
	}

	return false;
}


status_t
VMAddressSpace::ResizeArea(VMArea* area, size_t newSize)
{
	addr_t newEnd = area->Base() + (newSize - 1);
	VMArea* next = fAreas.GetNext(area);
	if (next != NULL && next->Base() <= newEnd) {
		if (next->id != RESERVED_AREA_ID
			|| next->cache_offset > area->Base()
			|| next->Base() + (next->Size() - 1) < newEnd) {
			panic("resize situation for area %p has changed although we "
				"should have the address space lock", area);
			return B_ERROR;
		}

		// resize reserved area
		addr_t offset = area->Base() + newSize - next->Base();
		if (next->Size() <= offset) {
			RemoveArea(next);
			free(next);
		} else {
			status_t error = ResizeAreaHead(next, next->Size() - offset);
			if (error != B_OK)
				return error;
		}
	}

	return ResizeAreaTail(area, newSize);
		// TODO: In case of error we should undo the change to the reserved
		// area.
}


status_t
VMAddressSpace::ResizeAreaHead(VMArea* area, size_t size)
{
	size_t oldSize = area->Size();
	if (size == oldSize)
		return B_OK;

	area->SetBase(area->Base() + oldSize - size);
	area->SetSize(size);

	return B_OK;
}


status_t
VMAddressSpace::ResizeAreaTail(VMArea* area, size_t size)
{
	size_t oldSize = area->Size();
	if (size == oldSize)
		return B_OK;

	area->SetSize(size);

	return B_OK;
}


status_t
VMAddressSpace::ReserveAddressRange(void** _address, uint32 addressSpec,
	size_t size, uint32 flags)
{
	// check to see if this address space has entered DELETE state
	if (fDeleting) {
		// okay, someone is trying to delete this address space now, so we
		// can't insert the area, let's back out
		return B_BAD_TEAM_ID;
	}

	VMArea* area = VMArea::CreateReserved(this, flags);
	if (area == NULL)
		return B_NO_MEMORY;

	status_t status = InsertArea(_address, addressSpec, size, area);
	if (status != B_OK) {
		free(area);
		return status;
	}

	area->cache_offset = area->Base();
		// we cache the original base address here

	Get();
	return B_OK;
}


status_t
VMAddressSpace::UnreserveAddressRange(addr_t address, size_t size)
{
	// check to see if this address space has entered DELETE state
	if (fDeleting) {
		// okay, someone is trying to delete this address space now, so we can't
		// insert the area, so back out
		return B_BAD_TEAM_ID;
	}

	// search area list and remove any matching reserved ranges
	addr_t endAddress = address + (size - 1);
	for (VMAddressSpaceAreaList::Iterator it = fAreas.GetIterator();
			VMArea* area = it.Next();) {
		// the area must be completely part of the reserved range
		if (area->Base() + (area->Size() - 1) > endAddress)
			break;
		if (area->id == RESERVED_AREA_ID && area->Base() >= (addr_t)address) {
			// remove reserved range
			RemoveArea(area);
			Put();
			free(area);
		}
	}

	return B_OK;
}


void
VMAddressSpace::UnreserveAllAddressRanges()
{
	for (VMAddressSpaceAreaList::Iterator it = fAreas.GetIterator();
			VMArea* area = it.Next();) {
		if (area->id == RESERVED_AREA_ID) {
			RemoveArea(area);
			Put();
			free(area);
		}
	}
}


void
VMAddressSpace::Dump() const
{
	kprintf("dump of address space at %p:\n", this);
	kprintf("id: 0x%lx\n", fID);
	kprintf("ref_count: %ld\n", fRefCount);
	kprintf("fault_count: %ld\n", fFaultCount);
	kprintf("translation_map: %p\n", &fTranslationMap);
	kprintf("base: 0x%lx\n", fBase);
	kprintf("end: 0x%lx\n", fEndAddress);
	kprintf("change_count: 0x%lx\n", fChangeCount);
	kprintf("area_hint: %p\n", fAreaHint);

	kprintf("area_list:\n");

	for (VMAddressSpaceAreaList::ConstIterator it = fAreas.GetIterator();
			VMArea* area = it.Next();) {
		kprintf(" area 0x%lx: ", area->id);
		kprintf("base_addr = 0x%lx ", area->Base());
		kprintf("size = 0x%lx ", area->Size());
		kprintf("name = '%s' ", area->name);
		kprintf("protection = 0x%lx\n", area->protection);
	}
}


/*!	Finds a reserved area that covers the region spanned by \a start and
	\a size, inserts the \a area into that region and makes sure that
	there are reserved regions for the remaining parts.
*/
status_t
VMAddressSpace::_InsertAreaIntoReservedRegion(addr_t start, size_t size,
	VMArea* area)
{
	VMArea* next;

	for (VMAddressSpaceAreaList::Iterator it = fAreas.GetIterator();
			(next = it.Next()) != NULL;) {
		if (next->Base() <= start
			&& next->Base() + (next->Size() - 1) >= start + (size - 1)) {
			// This area covers the requested range
			if (next->id != RESERVED_AREA_ID) {
				// but it's not reserved space, it's a real area
				return B_BAD_VALUE;
			}

			break;
		}
	}

	if (next == NULL)
		return B_ENTRY_NOT_FOUND;

	// Now we have to transfer the requested part of the reserved
	// range to the new area - and remove, resize or split the old
	// reserved area.

	if (start == next->Base()) {
		// the area starts at the beginning of the reserved range
		fAreas.Insert(next, area);

		if (size == next->Size()) {
			// the new area fully covers the reversed range
			fAreas.Remove(next);
			Put();
			free(next);
		} else {
			// resize the reserved range behind the area
			next->SetBase(next->Base() + size);
			next->SetSize(next->Size() - size);
		}
	} else if (start + size == next->Base() + next->Size()) {
		// the area is at the end of the reserved range
		fAreas.Insert(fAreas.GetNext(next), area);

		// resize the reserved range before the area
		next->SetSize(start - next->Base());
	} else {
		// the area splits the reserved range into two separate ones
		// we need a new reserved area to cover this space
		VMArea* reserved = VMArea::CreateReserved(this, next->protection);
		if (reserved == NULL)
			return B_NO_MEMORY;

		Get();
		fAreas.Insert(fAreas.GetNext(next), reserved);
		fAreas.Insert(reserved, area);

		// resize regions
		reserved->SetSize(next->Base() + next->Size() - start - size);
		next->SetSize(start - next->Base());
		reserved->SetBase(start + size);
		reserved->cache_offset = next->cache_offset;
	}

	area->SetBase(start);
	area->SetSize(size);
	IncrementChangeCount();

	return B_OK;
}


/*!	Must be called with this address space's write lock held */
status_t
VMAddressSpace::_InsertAreaSlot(addr_t start, addr_t size, addr_t end,
	uint32 addressSpec, VMArea* area)
{
	VMArea* last = NULL;
	VMArea* next;
	bool foundSpot = false;

	TRACE(("VMAddressSpace::InsertAreaSlot: address space %p, start 0x%lx, "
		"size %ld, end 0x%lx, addressSpec %ld, area %p\n", this, start,
		size, end, addressSpec, area));

	// do some sanity checking
	if (start < fBase || size == 0 || end > fEndAddress
		|| start + (size - 1) > end)
		return B_BAD_ADDRESS;

	if (addressSpec == B_EXACT_ADDRESS && area->id != RESERVED_AREA_ID) {
		// search for a reserved area
		status_t status = _InsertAreaIntoReservedRegion(start, size, area);
		if (status == B_OK || status == B_BAD_VALUE)
			return status;

		// There was no reserved area, and the slot doesn't seem to be used
		// already
		// TODO: this could be further optimized.
	}

	size_t alignment = B_PAGE_SIZE;
	if (addressSpec == B_ANY_KERNEL_BLOCK_ADDRESS) {
		// align the memory to the next power of two of the size
		while (alignment < size)
			alignment <<= 1;
	}

	start = ROUNDUP(start, alignment);

	// walk up to the spot where we should start searching
second_chance:
	VMAddressSpaceAreaList::Iterator it = fAreas.GetIterator();
	while ((next = it.Next()) != NULL) {
		if (next->Base() > start + (size - 1)) {
			// we have a winner
			break;
		}

		last = next;
	}

	// find the right spot depending on the address specification - the area
	// will be inserted directly after "last" ("next" is not referenced anymore)

	switch (addressSpec) {
		case B_ANY_ADDRESS:
		case B_ANY_KERNEL_ADDRESS:
		case B_ANY_KERNEL_BLOCK_ADDRESS:
		{
			// find a hole big enough for a new area
			if (last == NULL) {
				// see if we can build it at the beginning of the virtual map
				addr_t alignedBase = ROUNDUP(fBase, alignment);
				if (is_valid_spot(fBase, alignedBase, size,
						next == NULL ? end : next->Base())) {
					foundSpot = true;
					area->SetBase(alignedBase);
					break;
				}

				last = next;
				next = it.Next();
			}

			// keep walking
			while (next != NULL) {
				addr_t alignedBase = ROUNDUP(last->Base() + last->Size(),
					alignment);
				if (is_valid_spot(last->Base() + (last->Size() - 1),
						alignedBase, size, next->Base())) {
					foundSpot = true;
					area->SetBase(alignedBase);
					break;
				}

				last = next;
				next = it.Next();
			}

			if (foundSpot)
				break;

			addr_t alignedBase = ROUNDUP(last->Base() + last->Size(),
				alignment);
			if (is_valid_spot(last->Base() + (last->Size() - 1), alignedBase,
					size, end)) {
				// got a spot
				foundSpot = true;
				area->SetBase(alignedBase);
				break;
			} else if (area->id != RESERVED_AREA_ID) {
				// We didn't find a free spot - if there are any reserved areas,
				// we can now test those for free space
				// TODO: it would make sense to start with the biggest of them
				it.Rewind();
				next = it.Next();
				for (last = NULL; next != NULL; next = it.Next()) {
					if (next->id != RESERVED_AREA_ID) {
						last = next;
						continue;
					}

					// TODO: take free space after the reserved area into
					// account!
					addr_t alignedBase = ROUNDUP(next->Base(), alignment);
					if (next->Base() == alignedBase && next->Size() == size) {
						// The reserved area is entirely covered, and thus,
						// removed
						fAreas.Remove(next);

						foundSpot = true;
						area->SetBase(alignedBase);
						free(next);
						break;
					}

					if ((next->protection & RESERVED_AVOID_BASE) == 0
						&&  alignedBase == next->Base()
						&& next->Size() >= size) {
						// The new area will be placed at the beginning of the
						// reserved area and the reserved area will be offset
						// and resized
						foundSpot = true;
						next->SetBase(next->Base() + size);
						next->SetSize(next->Size() - size);
						area->SetBase(alignedBase);
						break;
					}

					if (is_valid_spot(next->Base(), alignedBase, size,
							next->Base() + (next->Size() - 1))) {
						// The new area will be placed at the end of the
						// reserved area, and the reserved area will be resized
						// to make space
						alignedBase = ROUNDDOWN(
							next->Base() + next->Size() - size, alignment);

						foundSpot = true;
						next->SetSize(alignedBase - next->Base());
						area->SetBase(alignedBase);
						last = next;
						break;
					}

					last = next;
				}
			}
			break;
		}

		case B_BASE_ADDRESS:
		{
			// find a hole big enough for a new area beginning with "start"
			if (last == NULL) {
				// see if we can build it at the beginning of the specified
				// start
				if (next == NULL || next->Base() > start + (size - 1)) {
					foundSpot = true;
					area->SetBase(start);
					break;
				}

				last = next;
				next = it.Next();
			}

			// keep walking
			while (next != NULL) {
				if (next->Base() - (last->Base() + last->Size()) >= size) {
					// we found a spot (it'll be filled up below)
					break;
				}

				last = next;
				next = it.Next();
			}

			addr_t lastEnd = last->Base() + (last->Size() - 1);
			if (next != NULL || end - lastEnd >= size) {
				// got a spot
				foundSpot = true;
				if (lastEnd < start)
					area->SetBase(start);
				else
					area->SetBase(lastEnd + 1);
				break;
			}

			// we didn't find a free spot in the requested range, so we'll
			// try again without any restrictions
			start = fBase;
			addressSpec = B_ANY_ADDRESS;
			last = NULL;
			goto second_chance;
		}

		case B_EXACT_ADDRESS:
			// see if we can create it exactly here
			if ((last == NULL || last->Base() + (last->Size() - 1) < start)
				&& (next == NULL || next->Base() > start + (size - 1))) {
				foundSpot = true;
				area->SetBase(start);
				break;
			}
			break;
		default:
			return B_BAD_VALUE;
	}

	if (!foundSpot)
		return addressSpec == B_EXACT_ADDRESS ? B_BAD_VALUE : B_NO_MEMORY;

	area->SetSize(size);
	if (last)
		fAreas.Insert(fAreas.GetNext(last), area);
	else
		fAreas.Insert(fAreas.Head(), area);

	IncrementChangeCount();
	return B_OK;
}


/*static*/ int
VMAddressSpace::_DumpCommand(int argc, char** argv)
{
	VMAddressSpace* aspace;

	if (argc < 2) {
		kprintf("aspace: not enough arguments\n");
		return 0;
	}

	// if the argument looks like a number, treat it as such

	{
		team_id id = strtoul(argv[1], NULL, 0);

		aspace = sAddressSpaceTable.Lookup(id);
		if (aspace == NULL) {
			kprintf("invalid aspace id\n");
		} else {
			aspace->Dump();
		}
		return 0;
	}
	return 0;
}


/*static*/ int
VMAddressSpace::_DumpListCommand(int argc, char** argv)
{
	kprintf("   address      id         base          end   area count   "
		" area size\n");

	AddressSpaceTable::Iterator it = sAddressSpaceTable.GetIterator();
	while (VMAddressSpace* space = it.Next()) {
		int32 areaCount = 0;
		off_t areaSize = 0;
		for (VMAddressSpaceAreaList::Iterator it = space->fAreas.GetIterator();
				VMArea* area = it.Next();) {
			if (area->id != RESERVED_AREA_ID
				&& area->cache->type != CACHE_TYPE_NULL) {
				areaCount++;
				areaSize += area->Size();
			}
		}
		kprintf("%p  %6ld   %#010lx   %#10lx   %10ld   %10lld\n",
			space, space->ID(), space->Base(), space->EndAddress(), areaCount,
			areaSize);
	}

	return 0;
}
