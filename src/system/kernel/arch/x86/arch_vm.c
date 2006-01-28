/*
 * Copyright 2002-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <KernelExport.h>
#include <smp.h>
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


#define kMaxMemoryTypeRegisters 32

void *gDmaAddress;

static uint32 sMemoryTypeBitmap;
static int32 sMemoryTypeIDs[kMaxMemoryTypeRegisters];
static int32 sMemoryTypeRegisterCount;
static spinlock sMemoryTypeLock;


static int32
allocate_mtrr(void)
{
	int32 index;

	cpu_status state = disable_interrupts();
	acquire_spinlock(&sMemoryTypeLock);

	// find free bit

	for (index = 0; index < sMemoryTypeRegisterCount; index++) {
		if (sMemoryTypeBitmap & (1UL << index))
			continue;

		sMemoryTypeBitmap |= 1UL << index;

		release_spinlock(&sMemoryTypeLock);
		restore_interrupts(state);
		return index;
	}

	release_spinlock(&sMemoryTypeLock);
	restore_interrupts(state);

	return -1;
}


static void
free_mtrr(int32 index)
{
	cpu_status state = disable_interrupts();
	acquire_spinlock(&sMemoryTypeLock);

	sMemoryTypeBitmap &= ~(1UL << index);

	release_spinlock(&sMemoryTypeLock);
	restore_interrupts(state);
}


static uint64
nearest_power(addr_t value)
{
	uint64 power = 1UL << 12;
		// 12 bits is the smallest supported alignment/length

	while (value > power)
		power <<= 1;

	return power;
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

	TRACE(("allocate MTRR slot %ld, base = %Lx, length = %Lx\n", index,
		base, length));

	sMemoryTypeIDs[index] = id;
	x86_set_mtrr(index, base, length, type);

	return B_OK;
}


//	#pragma mark -


status_t
arch_vm_init(kernel_args *args)
{
	TRACE(("arch_vm_init: entry\n"));
	return 0;
}


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


status_t
arch_vm_init_end(kernel_args *args)
{
	TRACE(("arch_vm_init_endvm: entry\n"));

	// throw away anything in the kernel_args.pgtable[] that's not yet mapped
	vm_free_unused_boot_loader_range(KERNEL_BASE, 0x400000 * args->arch_args.num_pgtables);

	return B_OK;
}


status_t
arch_vm_init_post_modules(kernel_args *args)
{
	void *cookie;
	int32 i;

	// the x86 CPU modules are now accessible

	sMemoryTypeRegisterCount = x86_count_mtrrs();
	if (sMemoryTypeRegisterCount == 0)
		return B_OK;

	// not very likely, but play safe here
	if (sMemoryTypeRegisterCount > kMaxMemoryTypeRegisters)
		sMemoryTypeRegisterCount = kMaxMemoryTypeRegisters;

	// init memory type ID table

	for (i = 0; i < sMemoryTypeRegisterCount; i++) {
		sMemoryTypeIDs[i] = -1;
	}

	// set the physical memory ranges to write-back mode

	for (i = 0; i < args->num_physical_memory_ranges; i++) {
		set_memory_type(-1, args->physical_memory_range[i].start,
			args->physical_memory_range[i].size, B_MTR_WB);
	}

	return B_OK;
}


void
arch_vm_aspace_swap(vm_address_space *aspace)
{
	i386_swap_pgdir((addr_t)i386_translation_map_get_pgdir(&aspace->translation_map));
}


bool
arch_vm_supports_protection(uint32 protection)
{
	// x86 always has the same read/write properties for userland and the kernel.
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
	int32 index;

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
arch_vm_set_memory_type(struct vm_area *area, addr_t physicalBase, uint32 type)
{
	area->memory_type = type >> MEMORY_TYPE_SHIFT;
	return set_memory_type(area->id, physicalBase, area->size, type);
}
