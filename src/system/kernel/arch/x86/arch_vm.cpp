/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2008, Jérôme Duval.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <KernelExport.h>
#include <smp.h>
#include <util/AutoLock.h>
#include <vm.h>
#include <vm_page.h>
#include <vm_priv.h>

#include <arch/vm.h>
#include <arch/int.h>
#include <arch/cpu.h>

#include <arch/x86/bios.h>

#include <stdlib.h>
#include <string.h>


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


#define kMaxMemoryTypeRegisters 32

void *gDmaAddress;

static uint32 sMemoryTypeBitmap;
static int32 sMemoryTypeIDs[kMaxMemoryTypeRegisters];
static uint32 sMemoryTypeRegisterCount;
static spinlock sMemoryTypeLock;


static int32
allocate_mtrr(void)
{
	InterruptsSpinLocker _(&sMemoryTypeLock);

	// find free bit

	for (uint32 index = 0; index < sMemoryTypeRegisterCount; index++) {
		if (sMemoryTypeBitmap & (1UL << index))
			continue;

		sMemoryTypeBitmap |= 1UL << index;
		return index;
	}

	return -1;
}


static void
free_mtrr(int32 index)
{
	InterruptsSpinLocker _(&sMemoryTypeLock);

	sMemoryTypeBitmap &= ~(1UL << index);
}


static uint64
nearest_power(uint64 value)
{
	uint64 power = 1UL << 12;
		// 12 bits is the smallest supported alignment/length

	while (value > power)
		power <<= 1;

	return power;
}


static void
nearest_powers(uint64 value, uint64 *lower, uint64 *upper)
{
	uint64 power = 1UL << 12;
	*lower = power;
		// 12 bits is the smallest supported alignment/length

	while (value >= power) {
		*lower = power;
		power <<= 1;
	}

	*upper = power;
}


static status_t
set_memory_type(int32 id, uint64 base, uint64 length, uint32 type)
{
	int32 index;

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

	if (sMemoryTypeRegisterCount == 0)
		return B_NOT_SUPPORTED;

	// length must be a power of 2; just round it up to the next value
	length = nearest_power(length); 	

	if (length + base <= base) {
		// 4GB overflow
		return B_BAD_VALUE;
	}

	// base must be aligned to the length
	if (base & (length - 1))
		return B_BAD_VALUE;

	index = allocate_mtrr();
	if (index < 0)
		return B_ERROR;

	TRACE_MTRR("allocate MTRR slot %ld, base = %Lx, length = %Lx, type=0x%lx\n", 
		index, base, length, type);

	sMemoryTypeIDs[index] = id;
	x86_set_mtrr(index, base, length, type);

	return B_OK;
}


#define MTRR_MAX_SOLUTIONS 	5	// usually MTRR count is eight, keep a few for other needs
#define MTRR_MIN_SIZE 		0x100000	// 1 MB
static int64 sSolutions[MTRR_MAX_SOLUTIONS];
static int32 sSolutionCount;
static int64 sPropositions[MTRR_MAX_SOLUTIONS];


/*!	Find the nearest powers of two for a value, save current iteration,
  	then make recursives calls for the remaining values.
  	It uses at most MTRR_MAX_SOLUTIONS levels of recursion because
  	only that count of MTRR registers are available to map the memory.
*/
static void
find_nearest(uint64 value, int iteration)
{
	TRACE_MTRR("find_nearest %Lx %d\n", value, iteration);
	if (iteration > (MTRR_MAX_SOLUTIONS - 1) || (iteration + 1) >= sSolutionCount)
		return;
	uint64 down, up;
	int i;
	nearest_powers(value, &down, &up);
	sPropositions[iteration] = down;
	if (value - down < MTRR_MIN_SIZE) {
		for (i=0; i<=iteration; i++)
			sSolutions[i] = sPropositions[i];
		sSolutionCount = iteration + 1;
		return;
	}
	find_nearest(value - down, iteration + 1);
	sPropositions[iteration] = -up;
	if (up - value < MTRR_MIN_SIZE) {
		for (i=0; i<=iteration; i++)
			sSolutions[i] = sPropositions[i];
		sSolutionCount = iteration + 1;
		return;
	}
	find_nearest(up - value, iteration + 1);
}


/*!	Set up MTRR to map the memory to write-back using uncached if necessary */
static void
set_memory_write_back(int32 id, uint64 base, uint64 length)
{
	status_t err;
	TRACE_MTRR("set_memory_write_back base %Lx length %Lx\n", base, length);
	sSolutionCount = MTRR_MAX_SOLUTIONS;
	find_nearest(length, 0);

#ifdef TRACE_MTRR
	dprintf("solutions: ");
	for (int i=0; i<sSolutionCount; i++) {
                dprintf("0x%Lx ", sSolutions[i]);
        }
        dprintf("\n");
#endif

	bool nextDown = false;
	for (int i = 0; i < sSolutionCount; i++) {
		if (sSolutions[i] < 0) {
			if (nextDown)
				base += sSolutions[i];
			err = set_memory_type(id, base, -sSolutions[i], nextDown ? B_MTR_UC : B_MTR_WB);
			if (err != B_OK) {
				dprintf("set_memory_type returned %s (0x%lx)\n", strerror(err), err);
			}
			if (!nextDown)
				base -= sSolutions[i];
			nextDown = !nextDown;
		} else {
			if (nextDown)
				base -= sSolutions[i];
			err = set_memory_type(id, base, sSolutions[i], nextDown ? B_MTR_UC : B_MTR_WB);
			if (err != B_OK) {
				dprintf("set_memory_type returned %s (0x%lx)\n", strerror(err), err);
			}
			if (!nextDown)
				base += sSolutions[i];
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
//	void *cookie;

	// the x86 CPU modules are now accessible

	sMemoryTypeRegisterCount = x86_count_mtrrs();
	if (sMemoryTypeRegisterCount == 0)
		return B_OK;

	// not very likely, but play safe here
	if (sMemoryTypeRegisterCount > kMaxMemoryTypeRegisters)
		sMemoryTypeRegisterCount = kMaxMemoryTypeRegisters;

	// init memory type ID table

	for (uint32 i = 0; i < sMemoryTypeRegisterCount; i++) {
		sMemoryTypeIDs[i] = -1;
	}

	// set the physical memory ranges to write-back mode

	for (uint32 i = 0; i < args->num_physical_memory_ranges; i++) {
		set_memory_write_back(-1, args->physical_memory_range[i].start,
			args->physical_memory_range[i].size);
	}

	return B_OK;
}


void
arch_vm_aspace_swap(vm_address_space *aspace)
{
	i386_swap_pgdir((addr_t)i386_translation_map_get_pgdir(
		&aspace->translation_map));
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
arch_vm_unset_memory_type(struct vm_area *area)
{
	uint32 index;

	if (area->memory_type == 0)
		return;

	// find index for area ID

	for (index = 0; index < sMemoryTypeRegisterCount; index++) {
		if (sMemoryTypeIDs[index] == area->id) {
			x86_set_mtrr(index, 0, 0, 0);

			sMemoryTypeIDs[index] = -1;
			free_mtrr(index);
			break;
		}
	}
}


status_t
arch_vm_set_memory_type(struct vm_area *area, addr_t physicalBase,
	uint32 type)
{
	area->memory_type = type >> MEMORY_TYPE_SHIFT;
	return set_memory_type(area->id, physicalBase, area->size, type);
}
