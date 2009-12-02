/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008, Jérôme Duval.
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>

#include <smp.h>
#include <util/AutoLock.h>
#include <vm/vm.h>
#include <vm/vm_page.h>
#include <vm/vm_priv.h>
#include <vm/VMAddressSpace.h>

#include <arch/vm.h>
#include <arch/int.h>
#include <arch/cpu.h>

#include <arch/x86/bios.h>

#include "x86_paging.h"


//#define TRACE_ARCH_VM
#ifdef TRACE_ARCH_VM
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define TRACE_MTRR_ARCH_VM
#ifdef TRACE_MTRR_ARCH_VM
#	define TRACE_MTRR(x...) dprintf(x)
#else
#	define TRACE_MTRR(x...)
#endif


static const uint32 kMaxMemoryTypeRanges	= 32;
static const uint32 kMaxMemoryTypeRegisters	= 32;
static const uint64 kMinMemoryTypeRangeSize	= 1 << 12;


struct memory_type_range_analysis_info {
	uint64	size;
	uint32	rangesNeeded;
	uint32	subtractiveRangesNeeded;
	uint64	bestSubtractiveRange;
};

struct memory_type_range_analysis {
	uint64							base;
	uint64							size;
	uint32							type;
	uint32							rangesNeeded;
	uint64							endRange;
	memory_type_range_analysis_info	left;
	memory_type_range_analysis_info	right;
};

struct memory_type_range {
	uint64						base;
	uint64						size;
	uint32						type;
	area_id						area;
};


void *gDmaAddress;

static memory_type_range sMemoryTypeRanges[kMaxMemoryTypeRanges];
static uint32 sMemoryTypeRangeCount;

static memory_type_range_analysis sMemoryTypeRangeAnalysis[
	kMaxMemoryTypeRanges];

static x86_mtrr_info sMemoryTypeRegisters[kMaxMemoryTypeRegisters];
static uint32 sMemoryTypeRegisterCount;
static uint32 sMemoryTypeRegistersUsed;

static mutex sMemoryTypeLock = MUTEX_INITIALIZER("memory type ranges");


static void
set_mtrrs()
{
	x86_set_mtrrs(sMemoryTypeRegisters, sMemoryTypeRegistersUsed);

#ifdef TRACE_MTRR_ARCH_VM
	TRACE_MTRR("set MTRRs to:\n");
	for (uint32 i = 0; i < sMemoryTypeRegistersUsed; i++) {
		const x86_mtrr_info& info = sMemoryTypeRegisters[i];
		TRACE_MTRR("  mtrr: %2lu: base: %#9llx, size: %#9llx, type: %u\n",
			i, info.base, info.size, info.type);
	}
#endif
}


static void
add_used_mtrr(uint64 base, uint64 size, uint32 type)
{
	ASSERT(sMemoryTypeRegistersUsed < sMemoryTypeRegisterCount);

	x86_mtrr_info& info = sMemoryTypeRegisters[sMemoryTypeRegistersUsed++];
	info.base = base;
	info.size = size;
	info.type = type;
}


static void
analyze_range(memory_type_range_analysis& analysis, uint64 previousEnd,
	uint64 nextBase)
{
	uint64 base = analysis.base;
	uint64 size = analysis.size;

	memory_type_range_analysis_info& left = analysis.left;
	memory_type_range_analysis_info& right = analysis.right;

	uint32 leftSubtractiveRangesNeeded = 2;
	int32 leftBestSubtractiveRangeDifference = 0;
	uint32 leftBestSubtractivePositiveRangesNeeded = 0;
	uint32 leftBestSubtractiveRangesNeeded = 0;

	uint32 rightSubtractiveRangesNeeded = 2;
	int32 rightBestSubtractiveRangeDifference = 0;
	uint32 rightBestSubtractivePositiveRangesNeeded = 0;
	uint32 rightBestSubtractiveRangesNeeded = 0;

	uint64 range = kMinMemoryTypeRangeSize;

	while (size > 0) {
		if ((base & range) != 0) {
			left.rangesNeeded++;

			bool replaceBestSubtractive = false;
			int32 rangeDifference = (int32)left.rangesNeeded
				- (int32)leftSubtractiveRangesNeeded;
			if (left.bestSubtractiveRange == 0
				|| leftBestSubtractiveRangeDifference < rangeDifference) {
				// check for intersection with previous range
				replaceBestSubtractive
					= previousEnd == 0 || base - range >= previousEnd;
			}

			if (replaceBestSubtractive) {
				leftBestSubtractiveRangeDifference = rangeDifference;
				leftBestSubtractiveRangesNeeded
					= leftSubtractiveRangesNeeded;
				left.bestSubtractiveRange = range;
				leftBestSubtractivePositiveRangesNeeded = 0;
			} else
				leftBestSubtractivePositiveRangesNeeded++;

			left.size += range;
			base += range;
			size -= range;
		} else if (left.bestSubtractiveRange > 0)
			leftSubtractiveRangesNeeded++;

		if ((size & range) != 0) {
			right.rangesNeeded++;

			bool replaceBestSubtractive = false;
			int32 rangeDifference = (int32)right.rangesNeeded
				- (int32)rightSubtractiveRangesNeeded;
			if (right.bestSubtractiveRange == 0
				|| rightBestSubtractiveRangeDifference < rangeDifference) {
				// check for intersection with previous range
				replaceBestSubtractive
					= nextBase == 0 || base + size + range <= nextBase;
			}

			if (replaceBestSubtractive) {
				rightBestSubtractiveRangeDifference = rangeDifference;
				rightBestSubtractiveRangesNeeded
					= rightSubtractiveRangesNeeded;
				right.bestSubtractiveRange = range;
				rightBestSubtractivePositiveRangesNeeded = 0;
			} else
				rightBestSubtractivePositiveRangesNeeded++;

			right.size += range;
			size -= range;
		} else if (right.bestSubtractiveRange > 0)
			rightSubtractiveRangesNeeded++;

		range <<= 1;
	}

	analysis.endRange = range;

	// If a subtractive setup doesn't have any advantages, don't use it.
	// Also compute analysis.rangesNeeded.
	if (leftBestSubtractiveRangesNeeded
			+ leftBestSubtractivePositiveRangesNeeded >= left.rangesNeeded) {
		left.bestSubtractiveRange = 0;
		left.subtractiveRangesNeeded = 0;
		analysis.rangesNeeded = left.rangesNeeded;
	} else {
		left.subtractiveRangesNeeded = leftBestSubtractiveRangesNeeded
			+ leftBestSubtractivePositiveRangesNeeded;
		analysis.rangesNeeded = left.subtractiveRangesNeeded;
	}

	if (rightBestSubtractiveRangesNeeded
			+ rightBestSubtractivePositiveRangesNeeded >= right.rangesNeeded) {
		right.bestSubtractiveRange = 0;
		right.subtractiveRangesNeeded = 0;
		analysis.rangesNeeded += right.rangesNeeded;
	} else {
		right.subtractiveRangesNeeded = rightBestSubtractiveRangesNeeded
			+ rightBestSubtractivePositiveRangesNeeded;
		analysis.rangesNeeded += right.subtractiveRangesNeeded;
	}
}

static void
compute_mtrrs(const memory_type_range_analysis& analysis)
{
	const memory_type_range_analysis_info& left = analysis.left;
	const memory_type_range_analysis_info& right = analysis.right;

	// generate a setup for the left side
	if (left.rangesNeeded > 0) {
		uint64 base = analysis.base;
		uint64 size = left.size;
		uint64 range = analysis.endRange;
		uint64 rangeEnd = base + size;
		bool subtractive = false;
		while (size > 0) {
			if (range == left.bestSubtractiveRange) {
				base = rangeEnd - 2 * range;
				add_used_mtrr(base, range, analysis.type);
				subtractive = true;
				break;
			}

			if ((size & range) != 0) {
				rangeEnd -= range;
				add_used_mtrr(rangeEnd, range, analysis.type);
				size -= range;
			}

			range >>= 1;
		}

		if (subtractive) {
			uint64 shortestRange = range;
			while (size > 0) {
				if ((size & range) != 0) {
					shortestRange = range;
					size -= range;
				} else {
					add_used_mtrr(base, range, IA32_MTR_UNCACHED);
					base += range;
				}

				range >>= 1;
			}

			add_used_mtrr(base, shortestRange, IA32_MTR_UNCACHED);
		}
	}

	// generate a setup for the right side
	if (right.rangesNeeded > 0) {
		uint64 base = analysis.base + left.size;
		uint64 size = right.size;
		uint64 range = analysis.endRange;
		bool subtractive = false;
		while (size > 0) {
			if (range == right.bestSubtractiveRange) {
				add_used_mtrr(base, range * 2, analysis.type);
				subtractive = true;
				break;
			}

			if ((size & range) != 0) {
				add_used_mtrr(base, range, analysis.type);
				base += range;
				size -= range;
			}

			range >>= 1;
		}

		if (subtractive) {
			uint64 rangeEnd = base + range * 2;
			uint64 shortestRange = range;
			while (size > 0) {
				if ((size & range) != 0) {
					shortestRange = range;
					size -= range;
				} else {
					rangeEnd -= range;
					add_used_mtrr(rangeEnd, range, IA32_MTR_UNCACHED);
				}

				range >>= 1;
			}

			rangeEnd -= shortestRange;
			add_used_mtrr(rangeEnd, shortestRange, IA32_MTR_UNCACHED);
		}
	}
}


static status_t
update_mttrs()
{
	// Transfer the range array to the analysis array, dropping all uncachable
	// ranges (that's the default anyway) and joining adjacent ranges with the
	// same type.
	memory_type_range_analysis* ranges = sMemoryTypeRangeAnalysis;
	uint32 rangeCount = 0;
	{
		uint32 previousRangeType = IA32_MTR_UNCACHED;
		uint64 previousRangeEnd = 0;
		for (uint32 i = 0; i < sMemoryTypeRangeCount; i++) {
			if (sMemoryTypeRanges[i].type != IA32_MTR_UNCACHED) {
				uint64 rangeEnd = sMemoryTypeRanges[i].base
					+ sMemoryTypeRanges[i].size;
				if (previousRangeType == sMemoryTypeRanges[i].type
					&& previousRangeEnd >= sMemoryTypeRanges[i].base) {
					// the range overlaps/continues the previous range -- just
					// enlarge that one
					if (rangeEnd > previousRangeEnd)
						previousRangeEnd = rangeEnd;
					ranges[rangeCount - 1].size  = previousRangeEnd
						- ranges[rangeCount - 1].base;
				} else {
					// add the new range
					memset(&ranges[rangeCount], 0, sizeof(ranges[rangeCount]));
					ranges[rangeCount].base = sMemoryTypeRanges[i].base;
					ranges[rangeCount].size = sMemoryTypeRanges[i].size;
					ranges[rangeCount].type = sMemoryTypeRanges[i].type;
					previousRangeEnd = rangeEnd;
					previousRangeType = sMemoryTypeRanges[i].type;
					rangeCount++;
				}
			}
		}
	}

	// analyze the ranges
	uint32 registersNeeded = 0;
	uint64 previousEnd = 0;
	for (uint32 i = 0; i < rangeCount; i++) {
		memory_type_range_analysis& range = ranges[i];
		uint64 nextBase = i + 1 < rangeCount ? ranges[i + 1].base : 0;
		analyze_range(range, previousEnd, nextBase);
		registersNeeded += range.rangesNeeded;
		previousEnd = range.base + range.size;
	}

	// fail when we need more registers than we have
	if (registersNeeded > sMemoryTypeRegisterCount)
		return B_BUSY;

	sMemoryTypeRegistersUsed = 0;

	for (uint32 i = 0; i < rangeCount; i++) {
		memory_type_range_analysis& range = ranges[i];
		compute_mtrrs(range);
	}

	set_mtrrs();

	return B_OK;
}


static void
remove_memory_type_range_locked(uint32 index)
{
	sMemoryTypeRangeCount--;
	if (index < sMemoryTypeRangeCount) {
		memmove(sMemoryTypeRanges + index, sMemoryTypeRanges + index + 1,
			(sMemoryTypeRangeCount - index) * sizeof(memory_type_range));
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

	TRACE_MTRR("add_memory_type_range(%ld, %#llx, %#llx, %lu)\n", areaID, base,
		size, type);

	// base and size must at least be aligned to the minimum range size
	if (((base | size) & (kMinMemoryTypeRangeSize - 1)) != 0) {
		dprintf("add_memory_type_range(%ld, %#llx, %#llx, %lu): Memory base or "
			"size not minimally aligned!\n", areaID, base, size, type);
		return B_BAD_VALUE;
	}

	MutexLocker locker(sMemoryTypeLock);

	if (sMemoryTypeRangeCount == kMaxMemoryTypeRanges) {
		dprintf("add_memory_type_range(%ld, %#llx, %#llx, %lu): Out of "
			"memory ranges!\n", areaID, base, size, type);
		return B_BUSY;
	}

	// iterate through the existing ranges and check for clashes
	bool foundInsertionIndex = false;
	uint32 index = 0;
	for (uint32 i = 0; i < sMemoryTypeRangeCount; i++) {
		const memory_type_range& range = sMemoryTypeRanges[i];
		if (range.base > base) {
			if (range.base - base < size && range.type != type) {
				dprintf("add_memory_type_range(%ld, %#llx, %#llx, %lu): Memory "
					"range intersects with existing one (%#llx, %#llx, %lu).\n",
					areaID, base, size, type, range.base, range.size,
					range.type);
				return B_BAD_VALUE;
			}

			// found the insertion index
			if (!foundInsertionIndex) {
				index = i;
				foundInsertionIndex = true;
			}
			break;
		} else if (base - range.base < range.size && range.type != type) {
			dprintf("add_memory_type_range(%ld, %#llx, %#llx, %lu): Memory "
				"range intersects with existing one (%#llx, %#llx, %lu).\n",
				areaID, base, size, type, range.base, range.size, range.type);
			return B_BAD_VALUE;
		}
	}

	if (!foundInsertionIndex)
		index = sMemoryTypeRangeCount;

	// make room for the new range
	if (index < sMemoryTypeRangeCount) {
		memmove(sMemoryTypeRanges + index + 1, sMemoryTypeRanges + index,
			(sMemoryTypeRangeCount - index) * sizeof(memory_type_range));
	}
	sMemoryTypeRangeCount++;

	memory_type_range& rangeInfo = sMemoryTypeRanges[index];
	rangeInfo.base = base;
	rangeInfo.size = size;
	rangeInfo.type = type;
	rangeInfo.area = areaID;

	uint64 range = kMinMemoryTypeRangeSize;
	status_t error;
	do {
		error = update_mttrs();
		if (error == B_OK) {
			if (rangeInfo.size != size) {
				dprintf("add_memory_type_range(%ld, %#llx, %#llx, %lu): "
					"update_mtrrs() succeeded only with simplified range: "
					"base: %#llx, size: %#llx\n", areaID, base, size, type,
					rangeInfo.base, rangeInfo.size);
			}
			return B_OK;
		}

		// update_mttrs() failed -- try to simplify (i.e. shrink) the range
		while (rangeInfo.size != 0) {
			if ((rangeInfo.base & range) != 0) {
				rangeInfo.base += range;
				rangeInfo.size -= range;
				// don't shift the range yet -- we might still have an
				// unaligned size
				break;
			}
			if ((rangeInfo.size & range) != 0) {
				rangeInfo.size -= range;
				range <<= 1;
				break;
			}

			range <<= 1;
		}
	} while (rangeInfo.size > 0);

	dprintf("add_memory_type_range(%ld, %#llx, %#llx, %lu): update_mtrrs() "
		"failed.\n", areaID, base, size, type);
	remove_memory_type_range_locked(index);
	return error;
}


static void
remove_memory_type_range(area_id areaID)
{
	MutexLocker locker(sMemoryTypeLock);

	for (uint32 i = 0; i < sMemoryTypeRangeCount; i++) {
		if (sMemoryTypeRanges[i].area == areaID) {
			TRACE_MTRR("remove_memory_type_range(%ld, %#llx, %#llxd)\n",
				areaID, sMemoryTypeRanges[i].base, sMemoryTypeRanges[i].size);
			remove_memory_type_range_locked(i);
			update_mttrs();
				// TODO: It's actually possible that this call fails, since
				// compute_mtrrs() joins ranges and removing one might cause a
				// previously joined big simple range to be split into several
				// ranges (or just make it more complicated).
			return;
		}
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
	id = map_physical_memory("dma_region", (void *)0x0, 0xa0000,
		B_ANY_KERNEL_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		&gDmaAddress);
	if (id < 0) {
		panic("arch_vm_init_post_area: unable to map dma region\n");
		return B_NO_MEMORY;
	}

	return bios_init();
}


/*!	Gets rid of all yet unmapped (and therefore now unused) page tables */
status_t
arch_vm_init_end(kernel_args *args)
{
	TRACE(("arch_vm_init_endvm: entry\n"));

	// throw away anything in the kernel_args.pgtable[] that's not yet mapped
	vm_free_unused_boot_loader_range(KERNEL_BASE,
		0x400000 * args->arch_args.num_pgtables);

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
		&& protection & B_KERNEL_WRITE_AREA)
		return false;

	return true;
}


void
arch_vm_unset_memory_type(struct VMArea *area)
{
	if (area->memory_type == 0)
		return;

	remove_memory_type_range(area->id);
}


status_t
arch_vm_set_memory_type(struct VMArea *area, addr_t physicalBase,
	uint32 type)
{
	area->memory_type = type >> MEMORY_TYPE_SHIFT;
	return add_memory_type_range(area->id, physicalBase, area->size, type);
}
