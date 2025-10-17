/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "VMUserAddressSpace.h"

#include <stdlib.h>

#include <algorithm>

#include <KernelExport.h>

#include <heap.h>
#include <thread.h>
#include <util/atomic.h>
#include <util/Random.h>
#include <vm/vm.h>
#include <vm/VMArea.h>


//#define TRACE_VM
#ifdef TRACE_VM
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#ifdef B_HAIKU_64_BIT
const addr_t VMUserAddressSpace::kMaxRandomize			=  0x8000000000ul;
const addr_t VMUserAddressSpace::kMaxInitialRandomize	= 0x20000000000ul;
#else
const addr_t VMUserAddressSpace::kMaxRandomize			=  0x800000ul;
const addr_t VMUserAddressSpace::kMaxInitialRandomize	= 0x2000000ul;
#endif


/*!	Verifies that an area with the given aligned base and size fits into
	the spot defined by base and limit and checks for overflows.
*/
static inline bool
is_valid_spot(addr_t base, addr_t alignedBase, addr_t size, addr_t limit)
{
	return (alignedBase >= base && alignedBase + (size - 1) > alignedBase
		&& alignedBase + (size - 1) <= limit);
}


static inline bool
is_base_address_spec(uint32 addressSpec)
{
	return addressSpec == B_BASE_ADDRESS
		|| addressSpec == B_RANDOMIZED_BASE_ADDRESS;
}


static inline addr_t
align_address(addr_t address, size_t alignment)
{
	return ROUNDUP(address, alignment);
}


static inline addr_t
align_address(addr_t address, size_t alignment, uint32 addressSpec,
	addr_t baseAddress)
{
	if (is_base_address_spec(addressSpec))
		address = std::max(address, baseAddress);
	return align_address(address, alignment);
}


// #pragma mark - VMUserAddressSpace


VMUserAddressSpace::VMUserAddressSpace(team_id id, addr_t base, size_t size)
	:
	VMAddressSpace(id, base, size, "address space"),
	fNextInsertHint(0)
{
}


VMUserAddressSpace::~VMUserAddressSpace()
{
}


inline VMArea*
VMUserAddressSpace::FirstArea() const
{
	VMUserArea* area = fAreas.LeftMost();
	while (area != NULL && area->id == RESERVED_AREA_ID)
		area = fAreas.Next(area);
	return area;
}


inline VMArea*
VMUserAddressSpace::NextArea(VMArea* _area) const
{
	VMUserArea* area = static_cast<VMUserArea*>(_area);
	area = fAreas.Next(area);
	while (area != NULL && area->id == RESERVED_AREA_ID)
		area = fAreas.Next(area);
	return area;
}


VMArea*
VMUserAddressSpace::CreateArea(const char* name, uint32 wiring,
	uint32 protection, uint32 allocationFlags)
{
	return VMUserArea::Create(this, name, wiring, protection, allocationFlags);
}


void
VMUserAddressSpace::DeleteArea(VMArea* _area, uint32 allocationFlags)
{
	VMUserArea* area = static_cast<VMUserArea*>(_area);
	area->~VMUserArea();
	free_etc(area, allocationFlags);
}


//! You must hold the address space's read lock.
VMArea*
VMUserAddressSpace::LookupArea(addr_t address) const
{
	VMUserArea* area = fAreas.FindClosest(address, true);
	if (area == NULL || area->id == RESERVED_AREA_ID)
		return NULL;

	return area->ContainsAddress(address) ? area : NULL;
}


//! You must hold the address space's read lock.
VMArea*
VMUserAddressSpace::FindClosestArea(addr_t address, bool less) const
{
	VMUserArea* area = fAreas.FindClosest(address, less);
	while (area != NULL && area->id == RESERVED_AREA_ID)
		area = less ? fAreas.Previous(area) : fAreas.Next(area);
	return area;
}


/*!	This inserts the area you pass into the address space.
	It will also set the "_address" argument to its base address when
	the call succeeds.
	You need to hold the VMAddressSpace write lock.
*/
status_t
VMUserAddressSpace::InsertArea(VMArea* _area, size_t size,
	const virtual_address_restrictions* addressRestrictions,
	uint32 allocationFlags, void** _address)
{
	ASSERT_WRITE_LOCKED_RW_LOCK(&fLock);

	VMUserArea* area = static_cast<VMUserArea*>(_area);

	addr_t searchBase, searchEnd;
	status_t status;

	switch (addressRestrictions->address_specification) {
		case B_EXACT_ADDRESS:
			searchBase = (addr_t)addressRestrictions->address;
			searchEnd = (addr_t)addressRestrictions->address + (size - 1);
			break;

		case B_BASE_ADDRESS:
		case B_RANDOMIZED_BASE_ADDRESS:
			searchBase = std::max(fBase, (addr_t)addressRestrictions->address);
			searchEnd = fEndAddress;
			break;

		case B_ANY_ADDRESS:
		case B_ANY_KERNEL_ADDRESS:
		case B_ANY_KERNEL_BLOCK_ADDRESS:
		case B_RANDOMIZED_ANY_ADDRESS:
			searchBase = std::max(fBase, (addr_t)USER_BASE_ANY);
			searchEnd = fEndAddress;
			break;

		default:
			return B_BAD_VALUE;
	}

	status = _InsertAreaSlot(searchBase, size, searchEnd,
		addressRestrictions->address_specification,
		addressRestrictions->alignment, area, allocationFlags);
	if (status == B_OK) {
		if (_address != NULL)
			*_address = (void*)area->Base();
		fFreeSpace -= area->Size();
	}

	return status;
}


//! You must hold the address space's write lock.
void
VMUserAddressSpace::RemoveArea(VMArea* _area, uint32 allocationFlags)
{
	ASSERT_WRITE_LOCKED_RW_LOCK(&fLock);

	VMUserArea* area = static_cast<VMUserArea*>(_area);

	fAreas.Remove(area);

	if (area->id != RESERVED_AREA_ID) {
		IncrementChangeCount();
		fFreeSpace += area->Size();
	}

	if ((area->Base() + area->Size()) == fNextInsertHint)
		fNextInsertHint -= area->Size();
}


bool
VMUserAddressSpace::CanResizeArea(VMArea* area, size_t newSize)
{
	const addr_t newEnd = area->Base() + (newSize - 1);
	if (newEnd < area->Base())
		return false;

	VMUserArea* next = fAreas.Next(static_cast<VMUserArea*>(area));
	while (next != NULL) {
		if (next->Base() > newEnd)
			return true;

		// If the next area is a reservation, then we can resize into it.
		if (next->id != RESERVED_AREA_ID)
			return false;
		if ((next->Base() + (next->Size() - 1)) >= newEnd)
			return true;

		// This "next" area is a reservation, but it's not large enough.
		// See if we can resize past it as well.
		next = fAreas.Next(next);
	}

	return fEndAddress >= newEnd;
}


status_t
VMUserAddressSpace::ResizeArea(VMArea* _area, size_t newSize,
	uint32 allocationFlags)
{
	VMUserArea* area = static_cast<VMUserArea*>(_area);

	const addr_t oldEnd = area->Base() + (area->Size() - 1);
	const addr_t newEnd = area->Base() + (newSize - 1);

	VMUserArea* next = fAreas.Next(area);
	if (oldEnd < newEnd) {
		while (next != NULL) {
			if (next->Base() > newEnd)
				break;

			if (next->id != RESERVED_AREA_ID && next->Base() < newEnd) {
				panic("resize situation for area %p has changed although we "
					"should have the address space lock", area);
				return B_ERROR;
			}

			// shrink reserved area
			addr_t offset = (area->Base() + newSize) - next->Base();
			if (next->Size() <= offset) {
				VMUserArea* nextNext = fAreas.Next(next);
				RemoveArea(next, allocationFlags);
				next->~VMUserArea();
				free_etc(next, allocationFlags);
				next = nextNext;
			} else {
				status_t error = ShrinkAreaHead(next, next->Size() - offset,
					allocationFlags);
				if (error != B_OK)
					return error;
				break;
			}
		}
	} else {
		if (next != NULL && next->id == RESERVED_AREA_ID
				&& next->Base() == (oldEnd + 1)) {
			// expand reserved area (at most to its original size)
			const addr_t oldNextBase = next->Base();
			addr_t newNextBase = oldNextBase - (oldEnd - newEnd);
			if (newNextBase < (addr_t)next->cache_offset)
				newNextBase = next->cache_offset;

			next->SetBase(newNextBase);
			next->SetSize(next->Size() + (oldNextBase - newNextBase));
		}
	}

	area->SetSize(newSize);
	return B_OK;
}


status_t
VMUserAddressSpace::ShrinkAreaHead(VMArea* area, size_t size,
	uint32 allocationFlags)
{
	size_t oldSize = area->Size();
	if (size == oldSize)
		return B_OK;

	area->SetBase(area->Base() + oldSize - size);
	area->SetSize(size);

	return B_OK;
}


status_t
VMUserAddressSpace::ShrinkAreaTail(VMArea* area, size_t size,
	uint32 allocationFlags)
{
	size_t oldSize = area->Size();
	if (size == oldSize)
		return B_OK;

	area->SetSize(size);

	return B_OK;
}


status_t
VMUserAddressSpace::ReserveAddressRange(size_t size,
	const virtual_address_restrictions* addressRestrictions,
	uint32 flags, uint32 allocationFlags, void** _address)
{
	// check to see if this address space has entered DELETE state
	if (fDeleting) {
		// okay, someone is trying to delete this address space now, so we
		// can't insert the area, let's back out
		return B_BAD_TEAM_ID;
	}

	VMUserArea* area = VMUserArea::CreateReserved(this, flags, allocationFlags);
	if (area == NULL)
		return B_NO_MEMORY;

	status_t status = InsertArea(area, size, addressRestrictions,
		allocationFlags, _address);
	if (status != B_OK) {
		area->~VMUserArea();
		free_etc(area, allocationFlags);
		return status;
	}

	area->cache_offset = area->Base();
		// we cache the original base address here

	Get();
	return B_OK;
}


status_t
VMUserAddressSpace::UnreserveAddressRange(addr_t address, size_t size,
	uint32 allocationFlags)
{
	// check to see if this address space has entered DELETE state
	if (fDeleting) {
		// okay, someone is trying to delete this address space now, so we can't
		// remove the area, so back out
		return B_BAD_TEAM_ID;
	}

	// the area must be completely part of the reserved range
	VMUserArea* area = fAreas.FindClosest(address, false);
	if (area == NULL)
		return B_OK;

	addr_t endAddress = address + size - 1;
	for (VMUserAreaTree::Iterator it = fAreas.GetIterator(area);
		(area = it.Next()) != NULL
			&& area->Base() + area->Size() - 1 <= endAddress;) {

		if (area->id == RESERVED_AREA_ID) {
			// remove reserved range
			RemoveArea(area, allocationFlags);
			Put();
			area->~VMUserArea();
			free_etc(area, allocationFlags);
		}
	}

	return B_OK;
}


void
VMUserAddressSpace::UnreserveAllAddressRanges(uint32 allocationFlags)
{
	for (VMUserAreaTree::Iterator it = fAreas.GetIterator();
			VMUserArea* area = it.Next();) {
		if (area->id == RESERVED_AREA_ID) {
			RemoveArea(area, allocationFlags);
			Put();
			area->~VMUserArea();
			free_etc(area, allocationFlags);
		}
	}
}


void
VMUserAddressSpace::Dump() const
{
	VMAddressSpace::Dump();
	kprintf("area_list:\n");

	for (VMUserAreaTree::ConstIterator it = fAreas.GetIterator();
			VMUserArea* area = it.Next();) {
		kprintf(" area 0x%" B_PRIx32 ": ", area->id);
		kprintf("base_addr = 0x%lx ", area->Base());
		kprintf("size = 0x%lx ", area->Size());
		kprintf("name = '%s' ",
			area->id != RESERVED_AREA_ID ? area->name : "reserved");
		kprintf("protection = 0x%" B_PRIx32 "\n", area->protection);
	}
}


inline bool
VMUserAddressSpace::_IsRandomized(uint32 addressSpec) const
{
	return fRandomizingEnabled
		&& (addressSpec == B_RANDOMIZED_ANY_ADDRESS
			|| addressSpec == B_RANDOMIZED_BASE_ADDRESS);
}


addr_t
VMUserAddressSpace::_RandomizeAddress(addr_t start, addr_t end,
	size_t alignment, bool initial)
{
	ASSERT((start & addr_t(alignment - 1)) == 0);
	ASSERT(start <= end);

	if (start == end)
		return start;

	addr_t range = end - start + 1;
	if (initial)
		range = std::min(range, kMaxInitialRandomize);
	else
		range = std::min(range, kMaxRandomize);

	addr_t random = secure_get_random<addr_t>();
	random %= range;
	random &= ~addr_t(alignment - 1);

	return start + random;
}


/*!	Finds a reserved area that covers the region spanned by \a start and
	\a size, inserts the \a area into that region and makes sure that
	there are reserved regions for the remaining parts.
*/
status_t
VMUserAddressSpace::_InsertAreaIntoReservedRegion(addr_t start, size_t size,
	VMUserArea* area, uint32 allocationFlags)
{
	VMUserArea* reserved = fAreas.FindClosest(start, true);
	if (reserved == NULL
		|| !reserved->ContainsAddress(start)
		|| !reserved->ContainsAddress(start + size - 1)) {
		return B_ENTRY_NOT_FOUND;
	}

	// This area covers the requested range
	if (reserved->id != RESERVED_AREA_ID) {
		// but it's not reserved space, it's a real area
		return B_BAD_VALUE;
	}

	// Now we have to transfer the requested part of the reserved
	// range to the new area - and remove, resize or split the old
	// reserved area.

	if (start == reserved->Base()) {
		// the area starts at the beginning of the reserved range

		if (size == reserved->Size()) {
			// the new area fully covers the reserved range
			fAreas.Remove(reserved);
			Put();
			reserved->~VMUserArea();
			free_etc(reserved, allocationFlags);
		} else {
			// resize the reserved range behind the area
			reserved->SetBase(reserved->Base() + size);
			reserved->SetSize(reserved->Size() - size);
		}
	} else if (start + size == reserved->Base() + reserved->Size()) {
		// the area is at the end of the reserved range
		// resize the reserved range before the area
		reserved->SetSize(start - reserved->Base());
	} else {
		// the area splits the reserved range into two separate ones
		// we need a new reserved area to cover this space
		VMUserArea* newReserved = VMUserArea::CreateReserved(this,
			reserved->protection, allocationFlags);
		if (newReserved == NULL)
			return B_NO_MEMORY;

		Get();

		// resize regions
		newReserved->SetBase(start + size);
		newReserved->SetSize(
			reserved->Base() + reserved->Size() - start - size);
		newReserved->cache_offset = reserved->cache_offset;

		reserved->SetSize(start - reserved->Base());

		fAreas.Insert(newReserved);
	}

	area->SetBase(start);
	area->SetSize(size);
	fAreas.Insert(area);
	IncrementChangeCount();

	return B_OK;
}


/*!	Must be called with this address space's write lock held */
status_t
VMUserAddressSpace::_InsertAreaSlot(addr_t start, addr_t size, addr_t end,
	uint32 addressSpec, size_t alignment, VMUserArea* area,
	uint32 allocationFlags)
{
	TRACE(("VMUserAddressSpace::_InsertAreaSlot: address space %p, start "
		"0x%lx, size %ld, end 0x%lx, addressSpec %" B_PRIu32 ", area %p\n",
		this, start, size, end, addressSpec, area));

	// do some sanity checking
	if (start < fBase || size == 0 || end > fEndAddress
		|| start + (size - 1) > end)
		return B_BAD_ADDRESS;

	if (addressSpec == B_EXACT_ADDRESS && area->id != RESERVED_AREA_ID) {
		// search for a reserved area
		status_t status = _InsertAreaIntoReservedRegion(start, size, area,
			allocationFlags);
		if (status == B_OK || status == B_BAD_VALUE)
			return status;

		// There was no reserved area, and the slot doesn't seem to be used
		// already
		// TODO: this could be further optimized.
	}

	if (alignment == 0)
		alignment = B_PAGE_SIZE;
	if (addressSpec == B_ANY_KERNEL_BLOCK_ADDRESS) {
		// align the memory to the next power of two of the size
		while (alignment < size)
			alignment <<= 1;
	}

	start = align_address(start, alignment);

	bool useHint = addressSpec != B_EXACT_ADDRESS
		&& !is_base_address_spec(addressSpec)
		&& fFreeSpace > (Size() / 2);

	addr_t originalStart = 0;
	if (fRandomizingEnabled && addressSpec == B_RANDOMIZED_BASE_ADDRESS) {
		originalStart = start;
		start = _RandomizeAddress(start, end - size + 1, alignment, true);
	} else if (useHint
			&& start <= fNextInsertHint && fNextInsertHint <= (end - size + 1)) {
		originalStart = start;
		start = fNextInsertHint;
	}

	// walk up to the spot where we should start searching
second_chance:
	VMUserArea* next = fAreas.FindClosest(start + size, false);
	VMUserArea* last = next != NULL
		? fAreas.Previous(next) : fAreas.FindClosest(start + size, true);

	// find the right spot depending on the address specification - the area
	// will be inserted directly after "last" ("next" is not referenced anymore)

	bool foundSpot = false;
	switch (addressSpec) {
		case B_ANY_ADDRESS:
		case B_ANY_KERNEL_ADDRESS:
		case B_ANY_KERNEL_BLOCK_ADDRESS:
		case B_RANDOMIZED_ANY_ADDRESS:
		case B_BASE_ADDRESS:
		case B_RANDOMIZED_BASE_ADDRESS:
		{
			VMUserAreaTree::Iterator it = fAreas.GetIterator(
				next != NULL ? next : fAreas.LeftMost());

			// find a hole big enough for a new area
			if (last == NULL) {
				// see if we can build it at the beginning of the virtual map
				addr_t alignedBase = align_address(start, alignment);
				addr_t nextBase = next == NULL
					? end : std::min(next->Base() - 1, end);
				if (is_valid_spot(start, alignedBase, size, nextBase)) {
					addr_t rangeEnd = std::min(nextBase - size + 1, end);
					if (_IsRandomized(addressSpec)) {
						alignedBase = _RandomizeAddress(alignedBase, rangeEnd,
							alignment);
					}

					foundSpot = true;
					area->SetBase(alignedBase);
					break;
				}

				last = next;
				next = it.Next();
			}

			// keep walking
			while (next != NULL && next->Base() + next->Size() - 1 <= end) {
				addr_t alignedBase = align_address(last->Base() + last->Size(),
					alignment, addressSpec, start);
				addr_t nextBase = std::min(end, next->Base() - 1);

				if (is_valid_spot(last->Base() + (last->Size() - 1),
						alignedBase, size, nextBase)) {
					addr_t rangeEnd = std::min(nextBase - size + 1, end);
					if (_IsRandomized(addressSpec)) {
						alignedBase = _RandomizeAddress(alignedBase,
							rangeEnd, alignment);
					}

					foundSpot = true;
					area->SetBase(alignedBase);
					break;
				}

				last = next;
				next = it.Next();
			}

			if (foundSpot)
				break;

			addr_t alignedBase = align_address(last->Base() + last->Size(),
				alignment, addressSpec, start);

			if (next == NULL && is_valid_spot(last->Base() + (last->Size() - 1),
					alignedBase, size, end)) {
				if (_IsRandomized(addressSpec)) {
					alignedBase = _RandomizeAddress(alignedBase, end - size + 1,
						alignment);
				}

				// got a spot
				foundSpot = true;
				area->SetBase(alignedBase);
				break;
			} else if (is_base_address_spec(addressSpec)) {
				// we didn't find a free spot in the requested range, so we'll
				// try again without any restrictions
				if (!_IsRandomized(addressSpec)) {
					start = USER_BASE_ANY;
					addressSpec = B_ANY_ADDRESS;
				} else if (start == originalStart) {
					start = USER_BASE_ANY;
					addressSpec = B_RANDOMIZED_ANY_ADDRESS;
				} else {
					start = originalStart;
					addressSpec = B_RANDOMIZED_BASE_ADDRESS;
				}

				goto second_chance;
			} else if (useHint
					&& originalStart != 0 && start != originalStart) {
				start = originalStart;
				goto second_chance;
			} else if (area->id != RESERVED_AREA_ID) {
				// We didn't find a free spot - if there are any reserved areas,
				// we can now test those for free space
				// TODO: it would make sense to start with the biggest of them
				it = fAreas.GetIterator();
				next = it.Next();
				for (last = NULL; next != NULL; next = it.Next()) {
					if (next->id != RESERVED_AREA_ID) {
						last = next;
						continue;
					} else if (next->Base() + size - 1 > end)
						break;

					// TODO: take free space after the reserved area into
					// account!
					addr_t alignedBase = align_address(next->Base(), alignment);
					if (next->Base() == alignedBase && next->Size() == size) {
						// The reserved area is entirely covered, and thus,
						// removed
						fAreas.Remove(next);

						foundSpot = true;
						area->SetBase(alignedBase);
						next->~VMUserArea();
						free_etc(next, allocationFlags);
						break;
					}

					if ((next->protection & RESERVED_AVOID_BASE) == 0
						&& alignedBase == next->Base()
						&& next->Size() >= size) {
						addr_t rangeEnd = std::min(
							next->Base() + next->Size() - size, end);
						if (_IsRandomized(addressSpec)) {
							alignedBase = _RandomizeAddress(next->Base(),
								rangeEnd, alignment);
						}
						addr_t offset = alignedBase - next->Base();

						// The new area will be placed at the beginning of the
						// reserved area and the reserved area will be offset
						// and resized
						foundSpot = true;
						next->SetBase(next->Base() + offset + size);
						next->SetSize(next->Size() - offset - size);
						area->SetBase(alignedBase);
						break;
					}

					if (is_valid_spot(next->Base(), alignedBase, size,
							std::min(next->Base() + next->Size() - 1, end))) {
						// The new area will be placed at the end of the
						// reserved area, and the reserved area will be resized
						// to make space

						if (_IsRandomized(addressSpec)) {
							addr_t alignedNextBase = align_address(next->Base(),
								alignment);

							addr_t startRange = next->Base() + next->Size();
							startRange -= size + kMaxRandomize;
							startRange = ROUNDDOWN(startRange, alignment);
							startRange = std::max(startRange, alignedNextBase);

							addr_t rangeEnd
								= std::min(next->Base() + next->Size() - size,
									end);
							alignedBase = _RandomizeAddress(startRange,
								rangeEnd, alignment);
						} else {
							alignedBase = ROUNDDOWN(
								next->Base() + next->Size() - size, alignment);
						}

						foundSpot = true;
						next->SetSize(alignedBase - next->Base());
						area->SetBase(alignedBase);
						break;
					}

					last = next;
				}
			}

			break;
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

	if (useHint)
		fNextInsertHint = area->Base() + size;

	area->SetSize(size);
	fAreas.Insert(area);
	IncrementChangeCount();
	return B_OK;
}
