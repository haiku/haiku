/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Jérôme Duval.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <new>

#include <KernelExport.h>

#include <boot/kernel_args.h>
#include <smp.h>
#include <util/AutoLock.h>
#include <vm/vm.h>
#include <vm/vm_page.h>
#include <vm/vm_priv.h>
#include <vm/VMAddressSpace.h>
#include <vm/VMArea.h>

#include <arch/vm.h>
#include <arch/int.h>
#include <arch/cpu.h>

#include <arch/x86/bios.h>


//#define TRACE_ARCH_VM
#ifdef TRACE_ARCH_VM
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

// 0: disabled, 1: some, 2: more
#define TRACE_MTRR_ARCH_VM 1

#if TRACE_MTRR_ARCH_VM >= 1
#	define TRACE_MTRR(x...) dprintf(x)
#else
#	define TRACE_MTRR(x...)
#endif

#if TRACE_MTRR_ARCH_VM >= 2
#	define TRACE_MTRR2(x...) dprintf(x)
#else
#	define TRACE_MTRR2(x...)
#endif


void *gDmaAddress;


struct memory_type_range : DoublyLinkedListLinkImpl<memory_type_range> {
	uint64						base;
	uint64						size;
	uint32						type;
	area_id						area;
};


struct memory_type_range_point
		: DoublyLinkedListLinkImpl<memory_type_range_point> {
	uint64				address;
	memory_type_range*	range;

	bool IsStart() const	{ return range->base == address; }

	bool operator<(const memory_type_range_point& other) const
	{
		return address < other.address;
	}
};


struct update_mtrr_info {
	uint64	ignoreUncacheableSize;
	uint64	shortestUncacheableSize;
};


typedef DoublyLinkedList<memory_type_range> MemoryTypeRangeList;

static mutex sMemoryTypeLock = MUTEX_INITIALIZER("memory type ranges");
static MemoryTypeRangeList sMemoryTypeRanges;
static int32 sMemoryTypeRangeCount = 0;

static const uint32 kMaxMemoryTypeRegisters	= 32;
static x86_mtrr_info sMemoryTypeRegisters[kMaxMemoryTypeRegisters];
static uint32 sMemoryTypeRegisterCount;
static uint32 sMemoryTypeRegistersUsed;

static memory_type_range* sTemporaryRanges = NULL;
static memory_type_range_point* sTemporaryRangePoints = NULL;
static int32 sTemporaryRangeCount = 0;
static int32 sTemporaryRangePointCount = 0;


static void
set_mtrrs()
{
	x86_set_mtrrs(IA32_MTR_WRITE_BACK, sMemoryTypeRegisters,
		sMemoryTypeRegistersUsed);

#if TRACE_MTRR_ARCH_VM
	TRACE_MTRR("set MTRRs to:\n");
	for (uint32 i = 0; i < sMemoryTypeRegistersUsed; i++) {
		const x86_mtrr_info& info = sMemoryTypeRegisters[i];
		TRACE_MTRR("  mtrr: %2" B_PRIu32 ": base: %#10" B_PRIx64  ", size: %#10"
			B_PRIx64 ", type: %u\n", i, info.base, info.size,
			info.type);
	}
#endif
}


static bool
add_used_mtrr(uint64 base, uint64 size, uint32 type)
{
	if (sMemoryTypeRegistersUsed == sMemoryTypeRegisterCount)
		return false;

	x86_mtrr_info& mtrr = sMemoryTypeRegisters[sMemoryTypeRegistersUsed++];
	mtrr.base = base;
	mtrr.size = size;
	mtrr.type = type;

	return true;
}


static bool
add_mtrrs_for_range(uint64 base, uint64 size, uint32 type)
{
	for (uint64 interval = B_PAGE_SIZE; size > 0; interval <<= 1) {
		if ((base & interval) != 0) {
			if (!add_used_mtrr(base, interval, type))
				return false;
			base += interval;
			size -= interval;
		}

		if ((size & interval) != 0) {
			if (!add_used_mtrr(base + size - interval, interval, type))
				return false;
			size -= interval;
		}
	}

	return true;
}


static memory_type_range*
find_range(area_id areaID)
{
	for (MemoryTypeRangeList::Iterator it = sMemoryTypeRanges.GetIterator();
			memory_type_range* range = it.Next();) {
		if (range->area == areaID)
			return range;
	}

	return NULL;
}


static void
optimize_memory_ranges(MemoryTypeRangeList& ranges, uint32 type,
	bool removeRanges)
{
	uint64 previousEnd = 0;
	uint64 nextStart = 0;
	MemoryTypeRangeList::Iterator it = ranges.GetIterator();
	memory_type_range* range = it.Next();
	while (range != NULL) {
		if (range->type != type) {
			previousEnd = range->base + range->size;
			nextStart = 0;
			range = it.Next();
			continue;
		}

		// find the start of the next range we cannot join this one with
		if (nextStart == 0) {
			MemoryTypeRangeList::Iterator nextIt = it;
			while (memory_type_range* nextRange = nextIt.Next()) {
				if (nextRange->type != range->type) {
					nextStart = nextRange->base;
					break;
				}
			}

			if (nextStart == 0) {
				// no upper limit -- set an artificial one, so we don't need to
				// special case below
				nextStart = (uint64)1 << 32;
			}
		}

		// Align the range's base and end to the greatest power of two possible.
		// As long as we can align both without intersecting any differently
		// range, we can extend the range without making it more complicated.
		// Once one side hit a limit we need to be careful. We can still
		// continue aligning the other side, if the range crosses the power of
		// two boundary.
		uint64 rangeBase = range->base;
		uint64 rangeEnd = rangeBase + range->size;
		uint64 interval = B_PAGE_SIZE * 2;
		while (true) {
			uint64 alignedBase = rangeBase & ~(interval - 1);
			uint64 alignedEnd = (rangeEnd + interval - 1) & ~(interval - 1);

			if (alignedBase < previousEnd)
				alignedBase += interval;

			if (alignedEnd > nextStart)
				alignedEnd -= interval;

			if (alignedBase >= alignedEnd)
				break;

			rangeBase = std::min(rangeBase, alignedBase);
			rangeEnd = std::max(rangeEnd, alignedEnd);

			interval <<= 1;
		}

		range->base = rangeBase;
		range->size = rangeEnd - rangeBase;

		if (removeRanges)
			it.Remove();

		previousEnd = rangeEnd;

		// Skip the subsequent ranges we have swallowed and possible cut one
		// we now partially intersect with.
		while ((range = it.Next()) != NULL) {
			if (range->base >= rangeEnd)
				break;

			if (range->base + range->size > rangeEnd) {
				// we partially intersect -- cut the range
				range->size = range->base + range->size - rangeEnd;
				range->base = rangeEnd;
				break;
			}

			// we have swallowed this range completely
			range->size = 0;
			it.Remove();
		}
	}
}


static bool
ensure_temporary_ranges_space(int32 count)
{
	if (sTemporaryRangeCount >= count && sTemporaryRangePointCount >= count)
		return true;

	// round count to power of 2
	int32 unalignedCount = count;
	count = 8;
	while (count < unalignedCount)
		count <<= 1;

	// resize ranges array
	if (sTemporaryRangeCount < count) {
		memory_type_range* ranges = new(std::nothrow) memory_type_range[count];
		if (ranges == NULL)
			return false;

		delete[] sTemporaryRanges;

		sTemporaryRanges = ranges;
		sTemporaryRangeCount = count;
	}

	// resize points array
	if (sTemporaryRangePointCount < count) {
		memory_type_range_point* points
			= new(std::nothrow) memory_type_range_point[count];
		if (points == NULL)
			return false;

		delete[] sTemporaryRangePoints;

		sTemporaryRangePoints = points;
		sTemporaryRangePointCount = count;
	}

	return true;
}


status_t
update_mtrrs(update_mtrr_info& updateInfo)
{
	// resize the temporary points/ranges arrays, if necessary
	if (!ensure_temporary_ranges_space(sMemoryTypeRangeCount * 2))
		return B_NO_MEMORY;

	// get the range points and sort them
	memory_type_range_point* rangePoints = sTemporaryRangePoints;
	int32 pointCount = 0;
	for (MemoryTypeRangeList::Iterator it = sMemoryTypeRanges.GetIterator();
			memory_type_range* range = it.Next();) {
		if (range->type == IA32_MTR_UNCACHED) {
			// Ignore uncacheable ranges below a certain size, if requested.
			// Since we always enforce uncacheability via the PTE attributes,
			// this is no problem (though not recommended for performance
			// reasons).
			if (range->size <= updateInfo.ignoreUncacheableSize)
				continue;
			if (range->size < updateInfo.shortestUncacheableSize)
				updateInfo.shortestUncacheableSize = range->size;
		}

		rangePoints[pointCount].address = range->base;
		rangePoints[pointCount++].range = range;
		rangePoints[pointCount].address = range->base + range->size;
		rangePoints[pointCount++].range = range;
	}

	std::sort(rangePoints, rangePoints + pointCount);

#if TRACE_MTRR_ARCH_VM >= 2
	TRACE_MTRR2("memory type range points:\n");
	for (int32 i = 0; i < pointCount; i++) {
		TRACE_MTRR2("%12" B_PRIx64 " (%p)\n", rangePoints[i].address,
			rangePoints[i].range);
	}
#endif

	// Compute the effective ranges. When ranges overlap, we go with the
	// stricter requirement. The types are not necessarily totally ordered, so
	// the order we use below is not always correct. To keep it simple we
	// consider it the reponsibility of the callers not to define overlapping
	// memory ranges with uncomparable types.

	memory_type_range* ranges = sTemporaryRanges;
	typedef DoublyLinkedList<memory_type_range_point> PointList;
	PointList pendingPoints;
	memory_type_range* activeRange = NULL;
	int32 rangeCount = 0;

	for (int32 i = 0; i < pointCount; i++) {
		memory_type_range_point* point = &rangePoints[i];
		bool terminateRange = false;
		if (point->IsStart()) {
			// a range start point
			pendingPoints.Add(point);
			if (activeRange != NULL && activeRange->type > point->range->type)
				terminateRange = true;
		} else {
			// a range end point -- remove the pending start point
			for (PointList::Iterator it = pendingPoints.GetIterator();
					memory_type_range_point* pendingPoint = it.Next();) {
				if (pendingPoint->range == point->range) {
					it.Remove();
					break;
				}
			}

			if (point->range == activeRange)
				terminateRange = true;
		}

		if (terminateRange) {
			ranges[rangeCount].size = point->address - ranges[rangeCount].base;
			rangeCount++;
			activeRange = NULL;
		}

		if (activeRange != NULL || pendingPoints.IsEmpty())
			continue;

		// we need to start a new range -- find the strictest pending range
		for (PointList::Iterator it = pendingPoints.GetIterator();
				memory_type_range_point* pendingPoint = it.Next();) {
			memory_type_range* pendingRange = pendingPoint->range;
			if (activeRange == NULL || activeRange->type > pendingRange->type)
				activeRange = pendingRange;
		}

		memory_type_range* previousRange = rangeCount > 0
			? &ranges[rangeCount - 1] : NULL;
		if (previousRange == NULL || previousRange->type != activeRange->type
				|| previousRange->base + previousRange->size
					< activeRange->base) {
			// we can't join with the previous range -- add a new one
			ranges[rangeCount].base = point->address;
			ranges[rangeCount].type = activeRange->type;
		} else
			rangeCount--;
	}

#if TRACE_MTRR_ARCH_VM >= 2
	TRACE_MTRR2("effective memory type ranges:\n");
	for (int32 i = 0; i < rangeCount; i++) {
		TRACE_MTRR2("%12" B_PRIx64 " - %12" B_PRIx64 ": %" B_PRIu32 "\n",
			ranges[i].base, ranges[i].base + ranges[i].size, ranges[i].type);
	}
#endif

	// Extend ranges to be more MTRR-friendly. A range is MTRR friendly, when it
	// has a power of two size and a base address aligned to the size. For
	// ranges without this property we need more than one MTRR. We improve
	// MTRR-friendliness by aligning a range's base and end address to the
	// greatest power of two (base rounded down, end up) such that the extended
	// range does not intersect with any other differently typed range. We join
	// equally typed ranges, if possible. There are two exceptions to the
	// intersection requirement: Uncached ranges may intersect with any other
	// range; the resulting type will still be uncached. Hence we can ignore
	// uncached ranges when extending the other ranges. Write-through ranges may
	// intersect with write-back ranges; the resulting type will be
	// write-through. Hence we can ignore write-through ranges when extending
	// write-back ranges.

	MemoryTypeRangeList rangeList;
	for (int32 i = 0; i < rangeCount; i++)
		rangeList.Add(&ranges[i]);

	static const uint32 kMemoryTypes[] = {
		IA32_MTR_UNCACHED,
		IA32_MTR_WRITE_COMBINING,
		IA32_MTR_WRITE_PROTECTED,
		IA32_MTR_WRITE_THROUGH,
		IA32_MTR_WRITE_BACK
	};
	static const int32 kMemoryTypeCount = sizeof(kMemoryTypes)
		/ sizeof(*kMemoryTypes);

	for (int32 i = 0; i < kMemoryTypeCount; i++) {
		uint32 type = kMemoryTypes[i];

		// Remove uncached and write-through ranges after processing them. This
		// let's us leverage their intersection property with any other
		// respectively write-back ranges.
		bool removeRanges = type == IA32_MTR_UNCACHED
			|| type == IA32_MTR_WRITE_THROUGH;

		optimize_memory_ranges(rangeList, type, removeRanges);
	}

#if TRACE_MTRR_ARCH_VM >= 2
	TRACE_MTRR2("optimized memory type ranges:\n");
	for (int32 i = 0; i < rangeCount; i++) {
		if (ranges[i].size > 0) {
			TRACE_MTRR2("%12" B_PRIx64 " - %12" B_PRIx64 ": %" B_PRIu32 "\n",
				ranges[i].base, ranges[i].base + ranges[i].size,
				ranges[i].type);
		}
	}
#endif

	// compute the mtrrs from the ranges
	sMemoryTypeRegistersUsed = 0;
	for (int32 i = 0; i < kMemoryTypeCount; i++) {
		uint32 type = kMemoryTypes[i];

		// skip write-back ranges -- that'll be the default type anyway
		if (type == IA32_MTR_WRITE_BACK)
			continue;

		for (int32 i = 0; i < rangeCount; i++) {
			if (ranges[i].size == 0 || ranges[i].type != type)
				continue;

			if (!add_mtrrs_for_range(ranges[i].base, ranges[i].size, type))
				return B_BUSY;
		}
	}

	set_mtrrs();

	return B_OK;
}


status_t
update_mtrrs()
{
	// Until we know how many MTRRs we have, pretend everything is OK.
	if (sMemoryTypeRegisterCount == 0)
		return B_OK;

	update_mtrr_info updateInfo;
	updateInfo.ignoreUncacheableSize = 0;

	while (true) {
		TRACE_MTRR2("update_mtrrs(): Trying with ignoreUncacheableSize %#"
			B_PRIx64 ".\n", updateInfo.ignoreUncacheableSize);

		updateInfo.shortestUncacheableSize = ~(uint64)0;
		status_t error = update_mtrrs(updateInfo);
		if (error != B_BUSY) {
			if (error == B_OK && updateInfo.ignoreUncacheableSize > 0) {
				TRACE_MTRR("update_mtrrs(): Succeeded setting MTRRs after "
					"ignoring uncacheable ranges up to size %#" B_PRIx64 ".\n",
					updateInfo.ignoreUncacheableSize);
			}
			return error;
		}

		// Not enough MTRRs. Retry with less uncacheable ranges.
		if (updateInfo.shortestUncacheableSize == ~(uint64)0) {
			// Ugh, even without any uncacheable ranges the available MTRRs do
			// not suffice.
			panic("update_mtrrs(): Out of MTRRs!");
			return B_BUSY;
		}

		ASSERT(updateInfo.ignoreUncacheableSize
			< updateInfo.shortestUncacheableSize);

		updateInfo.ignoreUncacheableSize = updateInfo.shortestUncacheableSize;
	}
}


static status_t
add_memory_type_range(area_id areaID, uint64 base, uint64 size, uint32 type)
{
	// translate the type
	if (type == 0)
		return B_OK;

	switch (type) {
		case B_MTR_UC:
			type = IA32_MTR_UNCACHED;
			break;
		case B_MTR_WC:
			type = IA32_MTR_WRITE_COMBINING;
			break;
		case B_MTR_WT:
			type = IA32_MTR_WRITE_THROUGH;
			break;
		case B_MTR_WP:
			type = IA32_MTR_WRITE_PROTECTED;
			break;
		case B_MTR_WB:
			type = IA32_MTR_WRITE_BACK;
			break;
		default:
			return B_BAD_VALUE;
	}

	TRACE_MTRR("add_memory_type_range(%" B_PRId32 ", %#" B_PRIx64 ", %#"
		B_PRIx64 ", %" B_PRIu32 ")\n", areaID, base, size, type);

	MutexLocker locker(sMemoryTypeLock);

	memory_type_range* range = areaID >= 0 ? find_range(areaID) : NULL;
	int32 oldRangeType = -1;
	if (range != NULL) {
		if (range->base != base || range->size != size)
			return B_BAD_VALUE;
		if (range->type == type)
			return B_OK;

		oldRangeType = range->type;
		range->type = type;
	} else {
		range = new(std::nothrow) memory_type_range;
		if (range == NULL)
			return B_NO_MEMORY;

		range->area = areaID;
		range->base = base;
		range->size = size;
		range->type = type;
		sMemoryTypeRanges.Add(range);
		sMemoryTypeRangeCount++;
	}

	status_t error = update_mtrrs();
	if (error != B_OK) {
		// revert the addition of the range/change of its type
		if (oldRangeType < 0) {
			sMemoryTypeRanges.Remove(range);
			sMemoryTypeRangeCount--;
			delete range;
		} else
			range->type = oldRangeType;

		update_mtrrs();
		return error;
	}

	return B_OK;
}


static void
remove_memory_type_range(area_id areaID)
{
	MutexLocker locker(sMemoryTypeLock);

	memory_type_range* range = find_range(areaID);
	if (range != NULL) {
		TRACE_MTRR("remove_memory_type_range(%" B_PRId32 ", %#" B_PRIx64 ", %#"
			B_PRIx64 ", %" B_PRIu32 ")\n", range->area, range->base,
			range->size, range->type);

		sMemoryTypeRanges.Remove(range);
		sMemoryTypeRangeCount--;
		delete range;

		update_mtrrs();
	} else {
		dprintf("remove_memory_type_range(): no range known for area %" B_PRId32
			"\n", areaID);
	}
}


//	#pragma mark -


status_t
arch_vm_init(kernel_args *args)
{
	TRACE(("arch_vm_init: entry\n"));
	return 0;
}


/*!	Marks DMA region as in-use, and maps it into the kernel space */
status_t
arch_vm_init_post_area(kernel_args *args)
{
	area_id id;

	TRACE(("arch_vm_init_post_area: entry\n"));

	// account for DMA area and mark the pages unusable
	vm_mark_page_range_inuse(0x0, 0xa0000 / B_PAGE_SIZE);

	// map 0 - 0xa0000 directly
	id = map_physical_memory("dma_region", 0x0, 0xa0000,
		B_ANY_KERNEL_ADDRESS | B_MTR_WB,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, &gDmaAddress);
	if (id < 0) {
		panic("arch_vm_init_post_area: unable to map dma region\n");
		return B_NO_MEMORY;
	}

	// TODO x86_64
#ifndef __x86_64__
	return bios_init();
#else
	return B_OK;
#endif
}


/*!	Gets rid of all yet unmapped (and therefore now unused) page tables */
status_t
arch_vm_init_end(kernel_args *args)
{
	TRACE(("arch_vm_init_endvm: entry\n"));

	// throw away anything in the kernel_args.pgtable[] that's not yet mapped
	vm_free_unused_boot_loader_range(KERNEL_BASE,
		args->arch_args.virtual_end - KERNEL_BASE);

	return B_OK;
}


status_t
arch_vm_init_post_modules(kernel_args *args)
{
	// the x86 CPU modules are now accessible

	sMemoryTypeRegisterCount = x86_count_mtrrs();
	if (sMemoryTypeRegisterCount == 0)
		return B_OK;

	// not very likely, but play safe here
	if (sMemoryTypeRegisterCount > kMaxMemoryTypeRegisters)
		sMemoryTypeRegisterCount = kMaxMemoryTypeRegisters;

	// set the physical memory ranges to write-back mode
	for (uint32 i = 0; i < args->num_physical_memory_ranges; i++) {
		add_memory_type_range(-1, args->physical_memory_range[i].start,
			args->physical_memory_range[i].size, B_MTR_WB);
	}

	return B_OK;
}


void
arch_vm_aspace_swap(struct VMAddressSpace *from, struct VMAddressSpace *to)
{
	// This functions is only invoked when a userland thread is in the process
	// of dying. It switches to the kernel team and does whatever cleanup is
	// necessary (in case it is the team's main thread, it will delete the
	// team).
	// It is however not necessary to change the page directory. Userland team's
	// page directories include all kernel mappings as well. Furthermore our
	// arch specific translation map data objects are ref-counted, so they won't
	// go away as long as they are still used on any CPU.
}


bool
arch_vm_supports_protection(uint32 protection)
{
	// x86 always has the same read/write properties for userland and the
	// kernel.
	// That's why we do not support user-read/kernel-write access. While the
	// other way around is not supported either, we don't care in this case
	// and give the kernel full access.
	if ((protection & (B_READ_AREA | B_WRITE_AREA)) == B_READ_AREA
		&& (protection & B_KERNEL_WRITE_AREA) != 0) {
		return false;
	}

	return true;
}


void
arch_vm_unset_memory_type(struct VMArea *area)
{
	if (area->MemoryType() == 0)
		return;

	remove_memory_type_range(area->id);
}


status_t
arch_vm_set_memory_type(struct VMArea *area, phys_addr_t physicalBase,
	uint32 type)
{
	return add_memory_type_range(area->id, physicalBase, area->Size(), type);
}
