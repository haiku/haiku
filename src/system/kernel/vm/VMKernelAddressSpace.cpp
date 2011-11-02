/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include "VMKernelAddressSpace.h"

#include <stdlib.h>

#include <KernelExport.h>

#include <heap.h>
#include <slab/Slab.h>
#include <thread.h>
#include <vm/vm.h>
#include <vm/VMArea.h>


//#define TRACE_VM
#ifdef TRACE_VM
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


//#define PARANOIA_CHECKS
#ifdef PARANOIA_CHECKS
#	define PARANOIA_CHECK_STRUCTURES()	_CheckStructures()
#else
#	define PARANOIA_CHECK_STRUCTURES()	do {} while (false)
#endif


static int
ld(size_t value)
{
	int index = -1;
	while (value > 0) {
		value >>= 1;
		index++;
	}

	return index;
}


/*!	Verifies that an area with the given aligned base and size fits into
	the spot defined by base and limit and checks for overflows.
	\param base Minimum base address valid for the area.
	\param alignedBase The base address of the range to check.
	\param size The size of the area.
	\param limit The last (inclusive) addresss of the range to check.
*/
static inline bool
is_valid_spot(addr_t base, addr_t alignedBase, addr_t size, addr_t limit)
{
	return (alignedBase >= base && alignedBase + (size - 1) > alignedBase
		&& alignedBase + (size - 1) <= limit);
}


// #pragma mark - VMKernelAddressSpace


VMKernelAddressSpace::VMKernelAddressSpace(team_id id, addr_t base, size_t size)
	:
	VMAddressSpace(id, base, size, "kernel address space"),
	fAreaObjectCache(NULL),
	fRangesObjectCache(NULL)
{
}


VMKernelAddressSpace::~VMKernelAddressSpace()
{
	panic("deleting the kernel aspace!\n");
}


status_t
VMKernelAddressSpace::InitObject()
{
	fAreaObjectCache = create_object_cache("kernel areas",
		sizeof(VMKernelArea), 0, NULL, NULL, NULL);
	if (fAreaObjectCache == NULL)
		return B_NO_MEMORY;

	fRangesObjectCache = create_object_cache("kernel address ranges",
		sizeof(Range), 0, NULL, NULL, NULL);
	if (fRangesObjectCache == NULL)
		return B_NO_MEMORY;

	// create the free lists
	size_t size = fEndAddress - fBase + 1;
	fFreeListCount = ld(size) - PAGE_SHIFT + 1;
	fFreeLists = new(std::nothrow) RangeFreeList[fFreeListCount];
	if (fFreeLists == NULL)
		return B_NO_MEMORY;

	Range* range = new(fRangesObjectCache, 0) Range(fBase, size,
		Range::RANGE_FREE);
	if (range == NULL)
		return B_NO_MEMORY;

	_InsertRange(range);

	TRACE("VMKernelAddressSpace::InitObject(): address range: %#" B_PRIxADDR
		" - %#" B_PRIxADDR ", free lists: %d\n", fBase, fEndAddress,
		fFreeListCount);

	return B_OK;
}


inline VMArea*
VMKernelAddressSpace::FirstArea() const
{
	Range* range = fRangeList.Head();
	while (range != NULL && range->type != Range::RANGE_AREA)
		range = fRangeList.GetNext(range);
	return range != NULL ? range->area : NULL;
}


inline VMArea*
VMKernelAddressSpace::NextArea(VMArea* _area) const
{
	Range* range = static_cast<VMKernelArea*>(_area)->Range();
	do {
		range = fRangeList.GetNext(range);
	} while (range != NULL && range->type != Range::RANGE_AREA);
	return range != NULL ? range->area : NULL;
}


VMArea*
VMKernelAddressSpace::CreateArea(const char* name, uint32 wiring,
	uint32 protection, uint32 allocationFlags)
{
	return VMKernelArea::Create(this, name, wiring, protection,
		fAreaObjectCache, allocationFlags);
}


void
VMKernelAddressSpace::DeleteArea(VMArea* _area, uint32 allocationFlags)
{
	TRACE("VMKernelAddressSpace::DeleteArea(%p)\n", area);

	VMKernelArea* area = static_cast<VMKernelArea*>(_area);
	object_cache_delete(fAreaObjectCache, area);
}


//! You must hold the address space's read lock.
VMArea*
VMKernelAddressSpace::LookupArea(addr_t address) const
{
	Range* range = fRangeTree.FindClosest(address, true);
	if (range == NULL || range->type != Range::RANGE_AREA)
		return NULL;

	VMKernelArea* area = range->area;
	return area->ContainsAddress(address) ? area : NULL;
}


/*!	This inserts the area you pass into the address space.
	It will also set the "_address" argument to its base address when
	the call succeeds.
	You need to hold the VMAddressSpace write lock.
*/
status_t
VMKernelAddressSpace::InsertArea(VMArea* _area, size_t size,
	const virtual_address_restrictions* addressRestrictions,
	uint32 allocationFlags, void** _address)
{
	TRACE("VMKernelAddressSpace::InsertArea(%p, %" B_PRIu32 ", %#" B_PRIxSIZE
		", %p \"%s\")\n", addressRestrictions->address,
		addressRestrictions->address_specification, size, _area, _area->name);

	VMKernelArea* area = static_cast<VMKernelArea*>(_area);

	Range* range;
	status_t error = _AllocateRange(addressRestrictions, size,
		addressRestrictions->address_specification == B_EXACT_ADDRESS,
		allocationFlags, range);
	if (error != B_OK)
		return error;

	range->type = Range::RANGE_AREA;
	range->area = area;
	area->SetRange(range);
	area->SetBase(range->base);
	area->SetSize(range->size);

	if (_address != NULL)
		*_address = (void*)area->Base();
	fFreeSpace -= area->Size();

	PARANOIA_CHECK_STRUCTURES();

	return B_OK;
}


//! You must hold the address space's write lock.
void
VMKernelAddressSpace::RemoveArea(VMArea* _area, uint32 allocationFlags)
{
	TRACE("VMKernelAddressSpace::RemoveArea(%p)\n", _area);

	VMKernelArea* area = static_cast<VMKernelArea*>(_area);

	_FreeRange(area->Range(), allocationFlags);

	fFreeSpace += area->Size();

	PARANOIA_CHECK_STRUCTURES();
}


bool
VMKernelAddressSpace::CanResizeArea(VMArea* area, size_t newSize)
{
	Range* range = static_cast<VMKernelArea*>(area)->Range();

	if (newSize <= range->size)
		return true;

	Range* nextRange = fRangeList.GetNext(range);
	if (nextRange == NULL || nextRange->type == Range::RANGE_AREA)
		return false;

	if (nextRange->type == Range::RANGE_RESERVED
		&& nextRange->reserved.base > range->base) {
		return false;
	}

	// TODO: If there is free space after a reserved range (or vice versa), it
	// could be used as well.
	return newSize - range->size <= nextRange->size;
}


status_t
VMKernelAddressSpace::ResizeArea(VMArea* _area, size_t newSize,
	uint32 allocationFlags)
{
	TRACE("VMKernelAddressSpace::ResizeArea(%p, %#" B_PRIxSIZE ")\n", _area,
		newSize);

	VMKernelArea* area = static_cast<VMKernelArea*>(_area);
	Range* range = area->Range();

	if (newSize == range->size)
		return B_OK;

	Range* nextRange = fRangeList.GetNext(range);

	if (newSize < range->size) {
		if (nextRange != NULL && nextRange->type == Range::RANGE_FREE) {
			// a free range is following -- just enlarge it
			_FreeListRemoveRange(nextRange, nextRange->size);
			nextRange->size += range->size - newSize;
			nextRange->base = range->base + newSize;
			_FreeListInsertRange(nextRange, nextRange->size);
		} else {
			// no free range following -- we need to allocate a new one and
			// insert it
			nextRange = new(fRangesObjectCache, allocationFlags) Range(
				range->base + newSize, range->size - newSize,
				Range::RANGE_FREE);
			if (nextRange == NULL)
				return B_NO_MEMORY;
			_InsertRange(nextRange);
		}
	} else {
		if (nextRange == NULL
			|| (nextRange->type == Range::RANGE_RESERVED
				&& nextRange->reserved.base > range->base)) {
			return B_BAD_VALUE;
		}
		// TODO: If there is free space after a reserved range (or vice versa),
		// it could be used as well.
		size_t sizeDiff = newSize - range->size;
		if (sizeDiff > nextRange->size)
			return B_BAD_VALUE;

		if (sizeDiff == nextRange->size) {
			// The next range is completely covered -- remove and delete it.
			_RemoveRange(nextRange);
			object_cache_delete(fRangesObjectCache, nextRange, allocationFlags);
		} else {
			// The next range is only partially covered -- shrink it.
			if (nextRange->type == Range::RANGE_FREE)
				_FreeListRemoveRange(nextRange, nextRange->size);
			nextRange->size -= sizeDiff;
			nextRange->base = range->base + newSize;
			if (nextRange->type == Range::RANGE_FREE)
				_FreeListInsertRange(nextRange, nextRange->size);
		}
	}

	range->size = newSize;
	area->SetSize(newSize);

	IncrementChangeCount();
	PARANOIA_CHECK_STRUCTURES();
	return B_OK;
}


status_t
VMKernelAddressSpace::ShrinkAreaHead(VMArea* _area, size_t newSize,
	uint32 allocationFlags)
{
	TRACE("VMKernelAddressSpace::ShrinkAreaHead(%p, %#" B_PRIxSIZE ")\n", _area,
		newSize);

	VMKernelArea* area = static_cast<VMKernelArea*>(_area);
	Range* range = area->Range();

	if (newSize == range->size)
		return B_OK;

	if (newSize > range->size)
		return B_BAD_VALUE;

	Range* previousRange = fRangeList.GetPrevious(range);

	size_t sizeDiff = range->size - newSize;
	if (previousRange != NULL && previousRange->type == Range::RANGE_FREE) {
		// the previous range is free -- just enlarge it
		_FreeListRemoveRange(previousRange, previousRange->size);
		previousRange->size += sizeDiff;
		_FreeListInsertRange(previousRange, previousRange->size);
		range->base += sizeDiff;
		range->size = newSize;
	} else {
		// no free range before -- we need to allocate a new one and
		// insert it
		previousRange = new(fRangesObjectCache, allocationFlags) Range(
			range->base, sizeDiff, Range::RANGE_FREE);
		if (previousRange == NULL)
			return B_NO_MEMORY;
		range->base += sizeDiff;
		range->size = newSize;
		_InsertRange(previousRange);
	}

	area->SetBase(range->base);
	area->SetSize(range->size);

	IncrementChangeCount();
	PARANOIA_CHECK_STRUCTURES();
	return B_OK;
}


status_t
VMKernelAddressSpace::ShrinkAreaTail(VMArea* area, size_t newSize,
	uint32 allocationFlags)
{
	return ResizeArea(area, newSize, allocationFlags);
}


status_t
VMKernelAddressSpace::ReserveAddressRange(size_t size,
	const virtual_address_restrictions* addressRestrictions,
	uint32 flags, uint32 allocationFlags, void** _address)
{
	TRACE("VMKernelAddressSpace::ReserveAddressRange(%p, %" B_PRIu32 ", %#"
		B_PRIxSIZE ", %#" B_PRIx32 ")\n", addressRestrictions->address,
		addressRestrictions->address_specification, size, flags);

	// Don't allow range reservations, if the address space is about to be
	// deleted.
	if (fDeleting)
		return B_BAD_TEAM_ID;

	Range* range;
	status_t error = _AllocateRange(addressRestrictions, size, false,
		allocationFlags, range);
	if (error != B_OK)
		return error;

	range->type = Range::RANGE_RESERVED;
	range->reserved.base = range->base;
	range->reserved.flags = flags;

	if (_address != NULL)
		*_address = (void*)range->base;

	Get();
	PARANOIA_CHECK_STRUCTURES();
	return B_OK;
}


status_t
VMKernelAddressSpace::UnreserveAddressRange(addr_t address, size_t size,
	uint32 allocationFlags)
{
	TRACE("VMKernelAddressSpace::UnreserveAddressRange(%#" B_PRIxADDR ", %#"
		B_PRIxSIZE ")\n", address, size);

	// Don't allow range unreservations, if the address space is about to be
	// deleted. UnreserveAllAddressRanges() must be used.
	if (fDeleting)
		return B_BAD_TEAM_ID;

	// search range list and remove any matching reserved ranges
	addr_t endAddress = address + (size - 1);
	Range* range = fRangeTree.FindClosest(address, false);
	while (range != NULL && range->base + (range->size - 1) <= endAddress) {
		// Get the next range for the iteration -- we need to skip free ranges,
		// since _FreeRange() might join them with the current range and delete
		// them.
		Range* nextRange = fRangeList.GetNext(range);
		while (nextRange != NULL && nextRange->type == Range::RANGE_FREE)
			nextRange = fRangeList.GetNext(nextRange);

		if (range->type == Range::RANGE_RESERVED) {
			_FreeRange(range, allocationFlags);
			Put();
		}

		range = nextRange;
	}

	PARANOIA_CHECK_STRUCTURES();
	return B_OK;
}


void
VMKernelAddressSpace::UnreserveAllAddressRanges(uint32 allocationFlags)
{
	Range* range = fRangeList.Head();
	while (range != NULL) {
		// Get the next range for the iteration -- we need to skip free ranges,
		// since _FreeRange() might join them with the current range and delete
		// them.
		Range* nextRange = fRangeList.GetNext(range);
		while (nextRange != NULL && nextRange->type == Range::RANGE_FREE)
			nextRange = fRangeList.GetNext(nextRange);

		if (range->type == Range::RANGE_RESERVED) {
			_FreeRange(range, allocationFlags);
			Put();
		}

		range = nextRange;
	}

	PARANOIA_CHECK_STRUCTURES();
}


void
VMKernelAddressSpace::Dump() const
{
	VMAddressSpace::Dump();

	kprintf("area_list:\n");

	for (RangeList::ConstIterator it = fRangeList.GetIterator();
			Range* range = it.Next();) {
		if (range->type != Range::RANGE_AREA)
			continue;

		VMKernelArea* area = range->area;
		kprintf(" area %" B_PRId32 ": ", area->id);
		kprintf("base_addr = %#" B_PRIxADDR " ", area->Base());
		kprintf("size = %#" B_PRIxSIZE " ", area->Size());
		kprintf("name = '%s' ", area->name);
		kprintf("protection = %#" B_PRIx32 "\n", area->protection);
	}
}


inline void
VMKernelAddressSpace::_FreeListInsertRange(Range* range, size_t size)
{
	TRACE("    VMKernelAddressSpace::_FreeListInsertRange(%p (%#" B_PRIxADDR
		", %#" B_PRIxSIZE ", %d), %#" B_PRIxSIZE " (%d))\n", range, range->base,
		range->size, range->type, size, ld(size) - PAGE_SHIFT);

	fFreeLists[ld(size) - PAGE_SHIFT].Add(range);
}


inline void
VMKernelAddressSpace::_FreeListRemoveRange(Range* range, size_t size)
{
	TRACE("    VMKernelAddressSpace::_FreeListRemoveRange(%p (%#" B_PRIxADDR
		", %#" B_PRIxSIZE ", %d), %#" B_PRIxSIZE " (%d))\n", range, range->base,
		range->size, range->type, size, ld(size) - PAGE_SHIFT);

	fFreeLists[ld(size) - PAGE_SHIFT].Remove(range);
}


void
VMKernelAddressSpace::_InsertRange(Range* range)
{
	TRACE("    VMKernelAddressSpace::_InsertRange(%p (%#" B_PRIxADDR ", %#"
		B_PRIxSIZE ", %d))\n", range, range->base, range->size, range->type);

	// insert at the correct position in the range list
	Range* insertBeforeRange = fRangeTree.FindClosest(range->base, true);
	fRangeList.Insert(
		insertBeforeRange != NULL
			? fRangeList.GetNext(insertBeforeRange) : fRangeList.Head(),
		range);

	// insert into tree
	fRangeTree.Insert(range);

	// insert in the free ranges list, if the range is free
	if (range->type == Range::RANGE_FREE)
		_FreeListInsertRange(range, range->size);
}


void
VMKernelAddressSpace::_RemoveRange(Range* range)
{
	TRACE("    VMKernelAddressSpace::_RemoveRange(%p (%#" B_PRIxADDR ", %#"
		B_PRIxSIZE ", %d))\n", range, range->base, range->size, range->type);

	// remove from tree and range list
	// insert at the correct position in the range list
	fRangeTree.Remove(range);
	fRangeList.Remove(range);

	// if it is a free range, also remove it from the free list
	if (range->type == Range::RANGE_FREE)
		_FreeListRemoveRange(range, range->size);
}


status_t
VMKernelAddressSpace::_AllocateRange(
	const virtual_address_restrictions* addressRestrictions,
	size_t size, bool allowReservedRange, uint32 allocationFlags,
	Range*& _range)
{
	TRACE("  VMKernelAddressSpace::_AllocateRange(address: %p, size: %#"
		B_PRIxSIZE ", addressSpec: %#" B_PRIx32 ", reserved allowed: %d)\n",
		addressRestrictions->address, size,
		addressRestrictions->address_specification, allowReservedRange);

	// prepare size, alignment and the base address for the range search
	addr_t address = (addr_t)addressRestrictions->address;
	size = ROUNDUP(size, B_PAGE_SIZE);
	size_t alignment = addressRestrictions->alignment != 0
		? addressRestrictions->alignment : B_PAGE_SIZE;

	switch (addressRestrictions->address_specification) {
		case B_EXACT_ADDRESS:
		{
			if (address % B_PAGE_SIZE != 0)
				return B_BAD_VALUE;
			break;
		}

		case B_BASE_ADDRESS:
			address = ROUNDUP(address, B_PAGE_SIZE);
			break;

		case B_ANY_KERNEL_BLOCK_ADDRESS:
			// align the memory to the next power of two of the size
			while (alignment < size)
				alignment <<= 1;

			// fall through...

		case B_ANY_ADDRESS:
		case B_ANY_KERNEL_ADDRESS:
			address = fBase;
			// TODO: remove this again when vm86 mode is moved into the kernel
			// completely (currently needs a userland address space!)
			if (address == USER_BASE)
				address = USER_BASE_ANY;
			break;

		default:
			return B_BAD_VALUE;
	}

	// find a range
	Range* range = _FindFreeRange(address, size, alignment,
		addressRestrictions->address_specification, allowReservedRange,
		address);
	if (range == NULL) {
		return addressRestrictions->address_specification == B_EXACT_ADDRESS
			? B_BAD_VALUE : B_NO_MEMORY;
	}

	TRACE("  VMKernelAddressSpace::_AllocateRange() found range:(%p (%#"
		B_PRIxADDR ", %#" B_PRIxSIZE ", %d)\n", range, range->base, range->size,
		range->type);

	// We have found a range. It might not be a perfect fit, in which case
	// we have to split the range.
	size_t rangeSize = range->size;

	if (address == range->base) {
		// allocation at the beginning of the range
		if (range->size > size) {
			// only partial -- split the range
			Range* leftOverRange = new(fRangesObjectCache, allocationFlags)
				Range(address + size, range->size - size, range);
			if (leftOverRange == NULL)
				return B_NO_MEMORY;

			range->size = size;
			_InsertRange(leftOverRange);
		}
	} else if (address + size == range->base + range->size) {
		// allocation at the end of the range -- split the range
		Range* leftOverRange = new(fRangesObjectCache, allocationFlags) Range(
			range->base, range->size - size, range);
		if (leftOverRange == NULL)
			return B_NO_MEMORY;

		range->base = address;
		range->size = size;
		_InsertRange(leftOverRange);
	} else {
		// allocation in the middle of the range -- split the range in three
		Range* leftOverRange1 = new(fRangesObjectCache, allocationFlags) Range(
			range->base, address - range->base, range);
		if (leftOverRange1 == NULL)
			return B_NO_MEMORY;
		Range* leftOverRange2 = new(fRangesObjectCache, allocationFlags) Range(
			address + size, range->size - size - leftOverRange1->size, range);
		if (leftOverRange2 == NULL) {
			object_cache_delete(fRangesObjectCache, leftOverRange1,
				allocationFlags);
			return B_NO_MEMORY;
		}

		range->base = address;
		range->size = size;
		_InsertRange(leftOverRange1);
		_InsertRange(leftOverRange2);
	}

	// If the range is a free range, remove it from the respective free list.
	if (range->type == Range::RANGE_FREE)
		_FreeListRemoveRange(range, rangeSize);

	IncrementChangeCount();

	TRACE("  VMKernelAddressSpace::_AllocateRange() -> %p (%#" B_PRIxADDR ", %#"
		B_PRIxSIZE ")\n", range, range->base, range->size);

	_range = range;
	return B_OK;
}


VMKernelAddressSpace::Range*
VMKernelAddressSpace::_FindFreeRange(addr_t start, size_t size,
	size_t alignment, uint32 addressSpec, bool allowReservedRange,
	addr_t& _foundAddress)
{
	TRACE("  VMKernelAddressSpace::_FindFreeRange(start: %#" B_PRIxADDR
		", size: %#" B_PRIxSIZE ", alignment: %#" B_PRIxSIZE ", addressSpec: %#"
		B_PRIx32 ", reserved allowed: %d)\n", start, size, alignment,
		addressSpec, allowReservedRange);

	switch (addressSpec) {
		case B_BASE_ADDRESS:
		{
			// We have to iterate through the range list starting at the given
			// address. This is the most inefficient case.
			Range* range = fRangeTree.FindClosest(start, true);
			while (range != NULL) {
				if (range->type == Range::RANGE_FREE) {
					addr_t alignedBase = ROUNDUP(range->base, alignment);
					if (is_valid_spot(start, alignedBase, size,
							range->base + (range->size - 1))) {
						_foundAddress = alignedBase;
						return range;
					}
				}
				range = fRangeList.GetNext(range);
			}

			// We didn't find a free spot in the requested range, so we'll
			// try again without any restrictions.
			start = fBase;
			addressSpec = B_ANY_ADDRESS;
			// fall through...
		}

		case B_ANY_ADDRESS:
		case B_ANY_KERNEL_ADDRESS:
		case B_ANY_KERNEL_BLOCK_ADDRESS:
		{
			// We want to allocate from the first non-empty free list that is
			// guaranteed to contain the size. Finding a free range is O(1),
			// unless there are constraints (min base address, alignment).
			int freeListIndex = ld((size * 2 - 1) >> PAGE_SHIFT);

			for (int32 i = freeListIndex; i < fFreeListCount; i++) {
				RangeFreeList& freeList = fFreeLists[i];
				if (freeList.IsEmpty())
					continue;

				for (RangeFreeList::Iterator it = freeList.GetIterator();
						Range* range = it.Next();) {
					addr_t alignedBase = ROUNDUP(range->base, alignment);
					if (is_valid_spot(start, alignedBase, size,
							range->base + (range->size - 1))) {
						_foundAddress = alignedBase;
						return range;
					}
				}
			}

			if (!allowReservedRange)
				return NULL;

			// We haven't found any free ranges, but we're supposed to look
			// for reserved ones, too. Iterate through the range list starting
			// at the given address.
			Range* range = fRangeTree.FindClosest(start, true);
			while (range != NULL) {
				if (range->type == Range::RANGE_RESERVED) {
					addr_t alignedBase = ROUNDUP(range->base, alignment);
					if (is_valid_spot(start, alignedBase, size,
							range->base + (range->size - 1))) {
						// allocation from the back might be preferred
						// -- adjust the base accordingly
						if ((range->reserved.flags & RESERVED_AVOID_BASE)
								!= 0) {
							alignedBase = ROUNDDOWN(
								range->base + (range->size - size), alignment);
						}

						_foundAddress = alignedBase;
						return range;
					}
				}
				range = fRangeList.GetNext(range);
			}

			return NULL;
		}

		case B_EXACT_ADDRESS:
		{
			Range* range = fRangeTree.FindClosest(start, true);
TRACE("    B_EXACT_ADDRESS: range: %p\n", range);
			if (range == NULL || range->type == Range::RANGE_AREA
				|| range->base + (range->size - 1) < start + (size - 1)) {
				// TODO: Support allocating if the area range covers multiple
				// free and reserved ranges!
TRACE("    -> no suitable range\n");
				return NULL;
			}

			if (range->type != Range::RANGE_FREE && !allowReservedRange)
{
TRACE("    -> reserved range not allowed\n");
				return NULL;
}

			_foundAddress = start;
			return range;
		}

		default:
			return NULL;
	}
}


void
VMKernelAddressSpace::_FreeRange(Range* range, uint32 allocationFlags)
{
	TRACE("  VMKernelAddressSpace::_FreeRange(%p (%#" B_PRIxADDR ", %#"
		B_PRIxSIZE ", %d))\n", range, range->base, range->size, range->type);

	// Check whether one or both of the neighboring ranges are free already,
	// and join them, if so.
	Range* previousRange = fRangeList.GetPrevious(range);
	Range* nextRange = fRangeList.GetNext(range);

	if (previousRange != NULL && previousRange->type == Range::RANGE_FREE) {
		if (nextRange != NULL && nextRange->type == Range::RANGE_FREE) {
			// join them all -- keep the first one, delete the others
			_FreeListRemoveRange(previousRange, previousRange->size);
			_RemoveRange(range);
			_RemoveRange(nextRange);
			previousRange->size += range->size + nextRange->size;
			object_cache_delete(fRangesObjectCache, range, allocationFlags);
			object_cache_delete(fRangesObjectCache, nextRange, allocationFlags);
			_FreeListInsertRange(previousRange, previousRange->size);
		} else {
			// join with the previous range only, delete the supplied one
			_FreeListRemoveRange(previousRange, previousRange->size);
			_RemoveRange(range);
			previousRange->size += range->size;
			object_cache_delete(fRangesObjectCache, range, allocationFlags);
			_FreeListInsertRange(previousRange, previousRange->size);
		}
	} else {
		if (nextRange != NULL && nextRange->type == Range::RANGE_FREE) {
			// join with the next range and delete it
			_RemoveRange(nextRange);
			range->size += nextRange->size;
			object_cache_delete(fRangesObjectCache, nextRange, allocationFlags);
		}

		// mark the range free and add it to the respective free list
		range->type = Range::RANGE_FREE;
		_FreeListInsertRange(range, range->size);
	}

	IncrementChangeCount();
}


#ifdef PARANOIA_CHECKS

void
VMKernelAddressSpace::_CheckStructures() const
{
	// general tree structure check
	fRangeTree.CheckTree();

	// check range list and tree
	size_t spaceSize = fEndAddress - fBase + 1;
	addr_t nextBase = fBase;
	Range* previousRange = NULL;
	int previousRangeType = Range::RANGE_AREA;
	uint64 freeRanges = 0;

	RangeList::ConstIterator listIt = fRangeList.GetIterator();
	RangeTree::ConstIterator treeIt = fRangeTree.GetIterator();
	while (true) {
		Range* range = listIt.Next();
		Range* treeRange = treeIt.Next();
		if (range != treeRange) {
			panic("VMKernelAddressSpace::_CheckStructures(): list/tree range "
				"mismatch: %p vs %p", range, treeRange);
		}
		if (range == NULL)
			break;

		if (range->base != nextBase) {
			panic("VMKernelAddressSpace::_CheckStructures(): range base %#"
				B_PRIxADDR ", expected: %#" B_PRIxADDR, range->base, nextBase);
		}

		if (range->size == 0) {
			panic("VMKernelAddressSpace::_CheckStructures(): empty range %p",
				range);
		}

		if (range->size % B_PAGE_SIZE != 0) {
			panic("VMKernelAddressSpace::_CheckStructures(): range %p (%#"
				B_PRIxADDR ", %#" B_PRIxSIZE ") not page aligned", range,
				range->base, range->size);
		}

		if (range->size > spaceSize - (range->base - fBase)) {
			panic("VMKernelAddressSpace::_CheckStructures(): range too large: "
				"(%#" B_PRIxADDR ", %#" B_PRIxSIZE "), address space end: %#"
				B_PRIxADDR, range->base, range->size, fEndAddress);
		}

		if (range->type == Range::RANGE_FREE) {
			freeRanges++;

			if (previousRangeType == Range::RANGE_FREE) {
				panic("VMKernelAddressSpace::_CheckStructures(): adjoining "
					"free ranges: %p (%#" B_PRIxADDR ", %#" B_PRIxSIZE
					"), %p (%#" B_PRIxADDR ", %#" B_PRIxSIZE ")", previousRange,
					previousRange->base, previousRange->size, range,
					range->base, range->size);
			}
		}

		previousRange = range;
		nextBase = range->base + range->size;
		previousRangeType = range->type;
	}

	if (nextBase - 1 != fEndAddress) {
		panic("VMKernelAddressSpace::_CheckStructures(): space not fully "
			"covered by ranges: last: %#" B_PRIxADDR ", expected %#" B_PRIxADDR,
			nextBase - 1, fEndAddress);
	}

	// check free lists
	uint64 freeListRanges = 0;
	for (int i = 0; i < fFreeListCount; i++) {
		RangeFreeList& freeList = fFreeLists[i];
		if (freeList.IsEmpty())
			continue;

		for (RangeFreeList::Iterator it = freeList.GetIterator();
				Range* range = it.Next();) {
			if (range->type != Range::RANGE_FREE) {
				panic("VMKernelAddressSpace::_CheckStructures(): non-free "
					"range %p (%#" B_PRIxADDR ", %#" B_PRIxSIZE ", %d) in "
					"free list %d", range, range->base, range->size,
					range->type, i);
			}

			if (fRangeTree.Find(range->base) != range) {
				panic("VMKernelAddressSpace::_CheckStructures(): unknown "
					"range %p (%#" B_PRIxADDR ", %#" B_PRIxSIZE ", %d) in "
					"free list %d", range, range->base, range->size,
					range->type, i);
			}

			if (ld(range->size) - PAGE_SHIFT != i) {
				panic("VMKernelAddressSpace::_CheckStructures(): "
					"range %p (%#" B_PRIxADDR ", %#" B_PRIxSIZE ", %d) in "
					"wrong free list %d", range, range->base, range->size,
					range->type, i);
			}

			freeListRanges++;
		}
	}
}

#endif	// PARANOIA_CHECKS
