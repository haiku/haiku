/*
 * Copyright 2003-2006, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */

/*	(bonefish) Some explanatory words on how address translation is implemented
	for the 32 bit PPC architecture.

	I use the address type nomenclature as used in the PPC architecture
	specs, i.e.
	- effective address: An address as used by program instructions, i.e.
	  that's what elsewhere (e.g. in the VM implementation) is called
	  virtual address.
	- virtual address: An intermediate address computed from the effective
	  address via the segment registers.
	- physical address: An address referring to physical storage.

	The hardware translates an effective address to a physical address using
	either of two mechanisms: 1) Block Address Translation (BAT) or
	2) segment + page translation. The first mechanism does this directly
	using two sets (for data/instructions) of special purpose registers.
	The latter mechanism is of more relevance here, though:

	effective address (32 bit):	     [ 0 ESID  3 | 4  PIX 19 | 20 Byte 31 ]
								           |           |            |
							     (segment registers)   |            |
									       |           |            |
	virtual address (52 bit):   [ 0      VSID 23 | 24 PIX 39 | 40 Byte 51 ]
	                            [ 0             VPN       39 | 40 Byte 51 ]
								                 |                  |
										   (page table)             |
											     |                  |
	physical address (32 bit):       [ 0        PPN       19 | 20 Byte 31 ]


	ESID: Effective Segment ID
	VSID: Virtual Segment ID
	PIX:  Page Index
	VPN:  Virtual Page Number
	PPN:  Physical Page Number


	Unlike on x86 we can't just switch the context to another team by just
	setting a register to another page directory, since we only have one
	page table containing both kernel and user address mappings. Instead we
	map the effective address space of kernel and *all* teams
	non-intersectingly into the virtual address space (which fortunately is
	20 bits wider), and use the segment registers to select the section of
	the virtual address space for the current team. Half of the 16 segment
	registers (8 - 15) map the kernel addresses, so they remain unchanged.

	The range of the virtual address space a team's effective address space
	is mapped to is defined by its vm_translation_map_arch_info::vsid_base,
	which is the first of the 8 successive VSID values used for the team.

	Which vsid_base values are already taken is defined by the set bits in
	the bitmap sVSIDBaseBitmap.


	TODO:
	* If we want to continue to use the OF services, we would need to add
	  its address mappings to the kernel space. Unfortunately some stuff
	  (especially RAM) is mapped in an address range without the kernel
	  address space. We probably need to map those into each team's address
	  space as kernel read/write areas.
	* The current locking scheme is insufficient. The page table is a resource
	  shared by all teams. We need to synchronize access to it. Probably via a
	  spinlock.
 */

#include <KernelExport.h>
#include <kernel.h>
#include <vm.h>
#include <vm_address_space.h>
#include <vm_priv.h>
#include <int.h>
#include <boot/kernel_args.h>
#include <arch/vm_translation_map.h>
#include <arch/cpu.h>
#include <arch_mmu.h>
#include <stdlib.h>

#include "generic_vm_physical_page_mapper.h"

static struct page_table_entry_group *sPageTable;
static size_t sPageTableSize;
static uint32 sPageTableHashMask;
static area_id sPageTableArea;


// 64 MB of iospace
#define IOSPACE_SIZE (64*1024*1024)
// We only have small (4 KB) pages. The only reason for choosing greater chunk
// size is to keep the waste of memory limited, since the generic page mapper
// allocates structures per physical/virtual chunk.
// TODO: Implement a page mapper more suitable for small pages!
#define IOSPACE_CHUNK_SIZE (16 * B_PAGE_SIZE)

static addr_t sIOSpaceBase;


// The VSID is a 24 bit number. The lower three bits are defined by the
// (effective) segment number, which leaves us with a 21 bit space of
// VSID bases (= 2 * 1024 * 1024).
#define MAX_VSID_BASES (PAGE_SIZE * 8)
static uint32 sVSIDBaseBitmap[MAX_VSID_BASES / (sizeof(uint32) * 8)];
static spinlock sVSIDBaseBitmapLock;

#define VSID_BASE_SHIFT 3
#define VADDR_TO_VSID(map, vaddr) \
	((map)->arch_data->vsid_base + ((vaddr) >> 28))

// vm_translation object stuff
typedef struct vm_translation_map_arch_info {
	int vsid_base;	// used VSIDs are vside_base ... vsid_base + 7
} vm_translation_map_arch_info;


void 
ppc_translation_map_change_asid(vm_translation_map *map)
{
// this code depends on the kernel being at 0x80000000, fix if we change that
#if KERNEL_BASE != 0x80000000
#error fix me
#endif
	int vsidBase = map->arch_data->vsid_base;

	isync();	// synchronize context
	asm("mtsr	0,%0" : : "g"(vsidBase));
	asm("mtsr	1,%0" : : "g"(vsidBase + 1));
	asm("mtsr	2,%0" : : "g"(vsidBase + 2));
	asm("mtsr	3,%0" : : "g"(vsidBase + 3));
	asm("mtsr	4,%0" : : "g"(vsidBase + 4));
	asm("mtsr	5,%0" : : "g"(vsidBase + 5));
	asm("mtsr	6,%0" : : "g"(vsidBase + 6));
	asm("mtsr	7,%0" : : "g"(vsidBase + 7));
	isync();	// synchronize context
}


static status_t
lock_tmap(vm_translation_map *map)
{
	recursive_lock_lock(&map->lock);
	return 0;
}


static status_t
unlock_tmap(vm_translation_map *map)
{
	recursive_lock_unlock(&map->lock);
	return 0;
}


static void 
destroy_tmap(vm_translation_map *map)
{
	if (map->map_count > 0) {
		panic("vm_translation_map.destroy_tmap: map %p has positive map count %ld\n",
			map, map->map_count);
	}

	// mark the vsid base not in use
	int baseBit = map->arch_data->vsid_base >> VSID_BASE_SHIFT;
	atomic_and((vint32 *)&sVSIDBaseBitmap[baseBit / 32], 
			~(1 << (baseBit % 32)));

	free(map->arch_data);
	recursive_lock_destroy(&map->lock);
}


static void
fill_page_table_entry(page_table_entry *entry, uint32 virtualSegmentID,
	addr_t virtualAddress, addr_t physicalAddress, uint8 protection, 
	bool secondaryHash)
{
	// lower 32 bit - set at once
	entry->physical_page_number = physicalAddress / B_PAGE_SIZE;
	entry->_reserved0 = 0;
	entry->referenced = false;
	entry->changed = false;
	entry->write_through = false;
	entry->caching_inhibited = false;
	entry->memory_coherent = false;
	entry->guarded = false;
	entry->_reserved1 = 0;
	entry->page_protection = protection & 0x3;
	eieio();
		// we need to make sure that the lower 32 bit were
		// already written when the entry becomes valid

	// upper 32 bit
	entry->virtual_segment_id = virtualSegmentID;
	entry->secondary_hash = secondaryHash;
	entry->abbr_page_index = (virtualAddress >> 22) & 0x3f;
	entry->valid = true;

	ppc_sync();
}


static status_t
map_tmap(vm_translation_map *map, addr_t virtualAddress, addr_t physicalAddress, uint32 attributes)
{
	// lookup the vsid based off the va
	uint32 virtualSegmentID = VADDR_TO_VSID(map, virtualAddress);
	uint32 protection = 0;

	// ToDo: check this
	// all kernel mappings are R/W to supervisor code
	if (attributes & (B_READ_AREA | B_WRITE_AREA))
		protection = (attributes & B_WRITE_AREA) ? PTE_READ_WRITE : PTE_READ_ONLY;

	//dprintf("vm_translation_map.map_tmap: vsid %d, pa 0x%lx, va 0x%lx\n", vsid, pa, va);

	// Search for a free page table slot using the primary hash value

	uint32 hash = page_table_entry::PrimaryHash(virtualSegmentID, virtualAddress);
	page_table_entry_group *group = &sPageTable[hash & sPageTableHashMask];

	for (int i = 0; i < 8; i++) {
		page_table_entry *entry = &group->entry[i];

		if (entry->valid)
			continue;

		fill_page_table_entry(entry, virtualSegmentID, virtualAddress, physicalAddress, 
			protection, false);
		map->map_count++;
		return B_OK;
	}

	// Didn't found one, try the secondary hash value

	hash = page_table_entry::SecondaryHash(hash);
	group = &sPageTable[hash & sPageTableHashMask];

	for (int i = 0; i < 8; i++) {
		page_table_entry *entry = &group->entry[i];

		if (entry->valid)
			continue;

		fill_page_table_entry(entry, virtualSegmentID, virtualAddress, physicalAddress, 
			protection, false);
		map->map_count++;
		return B_OK;
	}

	panic("vm_translation_map.map_tmap: hash table full\n");
	return B_ERROR;
}


static page_table_entry *
lookup_page_table_entry(vm_translation_map *map, addr_t virtualAddress)
{
	// lookup the vsid based off the va
	uint32 virtualSegmentID = VADDR_TO_VSID(map, virtualAddress);

//	dprintf("vm_translation_map.lookup_page_table_entry: vsid %d, va 0x%lx\n", vsid, va);


	// Search for the page table entry using the primary hash value

	uint32 hash = page_table_entry::PrimaryHash(virtualSegmentID, virtualAddress);
	page_table_entry_group *group = &sPageTable[hash & sPageTableHashMask];

	for (int i = 0; i < 8; i++) {
		page_table_entry *entry = &group->entry[i];

		if (entry->virtual_segment_id == virtualSegmentID
			&& entry->secondary_hash == false
			&& entry->abbr_page_index == ((virtualAddress >> 22) & 0x3f))
			return entry;
	}

	// Didn't found it, try the secondary hash value

	hash = page_table_entry::SecondaryHash(hash);
	group = &sPageTable[hash & sPageTableHashMask];

	for (int i = 0; i < 8; i++) {
		page_table_entry *entry = &group->entry[i];

		if (entry->virtual_segment_id == virtualSegmentID
			&& entry->secondary_hash == true
			&& entry->abbr_page_index == ((virtualAddress >> 22) & 0x3f))
			return entry;
	}

	return NULL;
}


static bool
remove_page_table_entry(vm_translation_map *map, addr_t virtualAddress)
{
	page_table_entry *entry = lookup_page_table_entry(map, virtualAddress);
	if (entry) {
		entry->valid = 0;
		ppc_sync();
		tlbie(virtualAddress);
		eieio();
		tlbsync();
		ppc_sync();
	}

	return entry;
}


static status_t
unmap_tmap(vm_translation_map *map, addr_t start, addr_t end)
{
	page_table_entry *entry;

	start = ROUNDOWN(start, B_PAGE_SIZE);
	end = ROUNDUP(end, B_PAGE_SIZE);

//	dprintf("vm_translation_map.unmap_tmap: start 0x%lx, end 0x%lx\n", start, end);

	while (start < end) {
		if (remove_page_table_entry(map, start))
			map->map_count--;

		start += B_PAGE_SIZE;
	}

	return B_OK;
}


static status_t
query_tmap_interrupt(vm_translation_map *map, addr_t va, addr_t *_outPhysical)
{
	page_table_entry *entry;

	// default the flags to not present
	*_outPhysical = 0;

	entry = lookup_page_table_entry(map, va);
	if (entry == NULL)
		return B_ERROR;

	*_outPhysical = entry->physical_page_number * B_PAGE_SIZE;
	return B_OK;
}


static status_t
query_tmap(vm_translation_map *map, addr_t va, addr_t *_outPhysical, uint32 *_outFlags)
{
	page_table_entry *entry;

	// default the flags to not present
	*_outFlags = 0;
	*_outPhysical = 0;

	entry = lookup_page_table_entry(map, va);
	if (entry == NULL)
		return B_NO_ERROR;

	// ToDo: check this!
	if (IS_KERNEL_ADDRESS(va))
		*_outFlags |= B_KERNEL_READ_AREA | (entry->page_protection == PTE_READ_ONLY ? 0 : B_KERNEL_WRITE_AREA);
	else
		*_outFlags |= B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_READ_AREA | (entry->page_protection == PTE_READ_ONLY ? 0 : B_WRITE_AREA);

	*_outFlags |= entry->changed ? PAGE_MODIFIED : 0;
	*_outFlags |= entry->referenced ? PAGE_ACCESSED : 0;
	*_outFlags |= entry->valid ? PAGE_PRESENT : 0;

	*_outPhysical = entry->physical_page_number * B_PAGE_SIZE;

	return B_OK;
}


static status_t
map_iospace_chunk(addr_t va, addr_t pa)
{
	pa &= ~(B_PAGE_SIZE - 1); // make sure it's page aligned
	va &= ~(B_PAGE_SIZE - 1); // make sure it's page aligned
	if (va < sIOSpaceBase || va >= (sIOSpaceBase + IOSPACE_SIZE))
		panic("map_iospace_chunk: passed invalid va 0x%lx\n", va);

	// map the pages
	return ppc_map_address_range(va, pa, IOSPACE_CHUNK_SIZE);
}


static addr_t 
get_mapped_size_tmap(vm_translation_map *map)
{
	return map->map_count;
}


static status_t
protect_tmap(vm_translation_map *map, addr_t base, addr_t top, uint32 attributes)
{
	// XXX finish
	return B_ERROR;
}


static status_t
clear_flags_tmap(vm_translation_map *map, addr_t virtualAddress, uint32 flags)
{
	page_table_entry *entry = lookup_page_table_entry(map, virtualAddress);
	if (entry == NULL)
		return B_NO_ERROR;

	bool modified = false;

	// clear the bits
	if (flags & PAGE_MODIFIED && entry->changed) {
		entry->changed = false;
		modified = true;
	}
	if (flags & PAGE_ACCESSED && entry->referenced) {
		entry->referenced = false;
		modified = true;
	}

	// synchronize
	if (modified) {
		tlbie(virtualAddress);
		eieio();
		tlbsync();
		ppc_sync();
	}

	return B_OK;
}


static void 
flush_tmap(vm_translation_map *map)
{
// TODO: arch_cpu_global_TLB_invalidate() is extremely expensive and doesn't
// even cut it here. We are supposed to invalidate all TLB entries for this
// map on all CPUs. We should loop over the virtual pages and invoke tlbie
// instead (which marks the entry invalid on all CPUs).
	arch_cpu_global_TLB_invalidate();
}


static status_t
get_physical_page_tmap(addr_t pa, addr_t *va, uint32 flags)
{
	return generic_get_physical_page(pa, va, flags);
}


static status_t
put_physical_page_tmap(addr_t va)
{
	return generic_put_physical_page(va);
}


static vm_translation_map_ops tmap_ops = {
	destroy_tmap,
	lock_tmap,
	unlock_tmap,
	map_tmap,
	unmap_tmap,
	query_tmap,
	query_tmap_interrupt,
	get_mapped_size_tmap,
	protect_tmap,
	clear_flags_tmap,
	flush_tmap,
	get_physical_page_tmap,
	put_physical_page_tmap
};


//  #pragma mark -
//  VM API


status_t
arch_vm_translation_map_init_map(vm_translation_map *map, bool kernel)
{
	// initialize the new object
	map->ops = &tmap_ops;
	map->map_count = 0;

	if (!kernel) {
		// During the boot process, there are no semaphores available at this
		// point, so we only try to create the translation map lock if we're
		// initialize a user translation map.
		// vm_translation_map_init_kernel_map_post_sem() is used to complete
		// the kernel translation map.
		if (recursive_lock_init(&map->lock, "translation map") < B_OK)
			return map->lock.sem;
	}

	map->arch_data = (vm_translation_map_arch_info *)malloc(sizeof(vm_translation_map_arch_info));
	if (map->arch_data == NULL) {
		if (!kernel)
			recursive_lock_destroy(&map->lock);
		return B_NO_MEMORY;
	}

	cpu_status state = disable_interrupts();
	acquire_spinlock(&sVSIDBaseBitmapLock);

	// allocate a VSID base for this one
	if (kernel) {
		// The boot loader has set up the segment registers for identical
		// mapping. Two VSID bases are reserved for the kernel: 0 and 8. The
		// latter one for mapping the kernel address space (0x80000000...), the
		// former one for the lower addresses required by the Open Firmware
		// services.
		map->arch_data->vsid_base = 0;
		sVSIDBaseBitmap[0] |= 0x3;
	} else {
		int i = 0;

		while (i < MAX_VSID_BASES) {
			if (sVSIDBaseBitmap[i / 32] == 0xffffffff) {
				i += 32;
				continue;
			}
			if ((sVSIDBaseBitmap[i / 32] & (1 << (i % 32))) == 0) {
				// we found it
				sVSIDBaseBitmap[i / 32] |= 1 << (i % 32);
				break;
			}
			i++;
		}
		if (i >= MAX_VSID_BASES)
			panic("vm_translation_map_create: out of VSID bases\n");
		map->arch_data->vsid_base = i << VSID_BASE_SHIFT;
	}

	release_spinlock(&sVSIDBaseBitmapLock);
	restore_interrupts(state);

	return B_OK;
}


status_t
arch_vm_translation_map_init_kernel_map_post_sem(vm_translation_map *map)
{
	if (recursive_lock_init(&map->lock, "translation map") < B_OK)
		return map->lock.sem;

	return B_OK;
}


status_t
arch_vm_translation_map_init(kernel_args *args)
{
	sPageTable = (page_table_entry_group *)args->arch_args.page_table.start;
	sPageTableSize = args->arch_args.page_table.size;
	sPageTableHashMask = sPageTableSize / sizeof(page_table_entry_group) - 1;

	// init physical page mapper
	status_t error = generic_vm_physical_page_mapper_init(args,
		map_iospace_chunk, &sIOSpaceBase, IOSPACE_SIZE, IOSPACE_CHUNK_SIZE);
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
arch_vm_translation_map_init_post_area(kernel_args *args)
{
	// If the page table doesn't lie within the kernel address space, we
	// remap it.
	if (!IS_KERNEL_ADDRESS(sPageTable)) {
		addr_t newAddress = (addr_t)sPageTable;
		status_t error = ppc_remap_address_range(&newAddress, sPageTableSize,
			false);
		if (error != B_OK) {
			panic("arch_vm_translation_map_init_post_area(): Failed to remap "
				"the page table!");
			return error;
		}

		// set the new page table address
		addr_t oldVirtualBase = (addr_t)(sPageTable);
		sPageTable = (page_table_entry_group*)newAddress;

		// unmap the old pages
		ppc_unmap_address_range(oldVirtualBase, sPageTableSize);

// TODO: We should probably map the page table via BAT. It is relatively large,
// and due to being a hash table the access patterns might look sporadic, which
// certainly isn't to the liking of the TLB.
	}

	// create an area to cover the page table
	sPageTableArea = create_area("page_table", (void **)&sPageTable, B_EXACT_ADDRESS, 
		sPageTableSize, B_ALREADY_WIRED, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);

	// init physical page mapper
	status_t error = generic_vm_physical_page_mapper_init_post_area(args);
	if (error != B_OK)
		return error;

	return B_OK;
}


status_t
arch_vm_translation_map_init_post_sem(kernel_args *args)
{
	// init physical page mapper
	return generic_vm_physical_page_mapper_init_post_sem(args);
}


/**	Directly maps a page without having knowledge of any kernel structures.
 *	Used only during VM setup.
 *	It currently ignores the "attributes" parameter and sets all pages
 *	read/write.
 */

status_t
arch_vm_translation_map_early_map(kernel_args *ka, addr_t virtualAddress, addr_t physicalAddress, 
	uint8 attributes, addr_t (*get_free_page)(kernel_args *))
{
	uint32 virtualSegmentID = get_sr((void *)virtualAddress) & 0xffffff;

	uint32 hash = page_table_entry::PrimaryHash(virtualSegmentID, (uint32)virtualAddress);
	page_table_entry_group *group = &sPageTable[hash & sPageTableHashMask];

	for (int32 i = 0; i < 8; i++) {
		// 8 entries in a group
		if (group->entry[i].valid)
			continue;

		fill_page_table_entry(&group->entry[i], virtualSegmentID, virtualAddress, physicalAddress, PTE_READ_WRITE, false);
		return B_OK;
	}

	hash = page_table_entry::SecondaryHash(hash);
	group = &sPageTable[hash & sPageTableHashMask];

	for (int32 i = 0; i < 8; i++) {
		if (group->entry[i].valid)
			continue;

		fill_page_table_entry(&group->entry[i], virtualSegmentID, virtualAddress, physicalAddress, PTE_READ_WRITE, true);
		return B_OK;
	}

	return B_ERROR;
}


// XXX currently assumes this translation map is active

status_t 
arch_vm_translation_map_early_query(addr_t va, addr_t *out_physical)
{
	//PANIC_UNIMPLEMENTED();
	panic("vm_translation_map_quick_query(): not yet implemented\n");
	return B_OK;
}


// #pragma mark -


status_t
ppc_map_address_range(addr_t virtualAddress, addr_t physicalAddress,
	size_t size)
{
	addr_t virtualEnd = ROUNDUP(virtualAddress + size, B_PAGE_SIZE);
	virtualAddress = ROUNDOWN(virtualAddress, B_PAGE_SIZE);
	physicalAddress = ROUNDOWN(physicalAddress, B_PAGE_SIZE);

	vm_address_space *addressSpace = vm_kernel_address_space();

	// map the pages
	for (; virtualAddress < virtualEnd;
		 virtualAddress += B_PAGE_SIZE, physicalAddress += B_PAGE_SIZE) {
		status_t error = map_tmap(&addressSpace->translation_map,
			virtualAddress, physicalAddress,
			B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
		if (error != B_OK)
			return error;
	}

	return B_OK;
}


void
ppc_unmap_address_range(addr_t virtualAddress, size_t size)
{
	addr_t virtualEnd = ROUNDUP(virtualAddress + size, B_PAGE_SIZE);
	virtualAddress = ROUNDOWN(virtualAddress, B_PAGE_SIZE);

	vm_address_space *addressSpace = vm_kernel_address_space();

	for (0; virtualAddress < virtualEnd; virtualAddress += B_PAGE_SIZE)
		remove_page_table_entry(&addressSpace->translation_map, virtualAddress);
}


status_t
ppc_remap_address_range(addr_t *_virtualAddress, size_t size, bool unmap)
{
	addr_t virtualAddress = ROUNDOWN(*_virtualAddress, B_PAGE_SIZE);
	size = ROUNDUP(*_virtualAddress + size - virtualAddress, B_PAGE_SIZE);

	vm_address_space *addressSpace = vm_kernel_address_space();

	// reserve space in the address space
	void *newAddress = NULL;
	status_t error = vm_reserve_address_range(addressSpace->id, &newAddress,
		B_ANY_KERNEL_ADDRESS, size, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA);
	if (error != B_OK)
		return error;

	// get the area's first physical page
	page_table_entry *entry = lookup_page_table_entry(
		&addressSpace->translation_map, virtualAddress);
	if (!entry)
		return B_ERROR;
	addr_t physicalBase = entry->physical_page_number << 12;

	// map the pages
	error = ppc_map_address_range((addr_t)newAddress, physicalBase, size);
	if (error != B_OK)
		return error;

	*_virtualAddress = (addr_t)newAddress;

	// unmap the old pages
	if (unmap)
		ppc_unmap_address_range(virtualAddress, size);

	return B_OK;
}

