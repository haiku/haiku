/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/


#include <KernelExport.h>
#include <kernel.h>
#include <vm.h>
#include <vm_priv.h>
#include <int.h>
#include <boot/kernel_args.h>
#include <arch/vm_translation_map.h>
#include <arch/cpu.h>
#include <arch_mmu.h>
#include <stdlib.h>


static struct page_table_entry_group *sPageTable;
static size_t sPageTableSize;
static uint32 sPageTableHashMask;
static area_id sPageTableRegion;


// 512 MB of iospace
#define IOSPACE_SIZE (512*1024*1024)
// put it 512 MB into kernel space
#define IOSPACE_BASE (KERNEL_BASE + IOSPACE_SIZE)

#define MAX_ASIDS (PAGE_SIZE * 8)
static uint32 asid_bitmap[MAX_ASIDS / (sizeof(uint32) * 8)];
spinlock asid_bitmap_lock;
#define ASID_SHIFT 4
#define VADDR_TO_ASID(map, vaddr) \
	(((map)->arch_data->asid_base << ASID_SHIFT) + ((vaddr) / 0x10000000))

// vm_translation object stuff
typedef struct vm_translation_map_arch_info_struct {
	int asid_base; // shift left by ASID_SHIFT to get the base asid to use
} vm_translation_map_arch_info;


static int 
lock_tmap(vm_translation_map *map)
{
	recursive_lock_lock(&map->lock);
	return 0;
}


static int 
unlock_tmap(vm_translation_map *map)
{
	recursive_lock_unlock(&map->lock);
	return 0;
}


static void 
destroy_tmap(vm_translation_map *map)
{
	if (map->map_count > 0) {
		panic("vm_translation_map.destroy_tmap: map %p has positive map count %d\n", map, map->map_count);
	}

	// mark the asid not in use
	atomic_and((vint32 *)&asid_bitmap[map->arch_data->asid_base / 32], 
			~(1 << (map->arch_data->asid_base % 32)));

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

	// ToDo: this is probably a bit too much sledge hammer
	arch_cpu_global_TLB_invalidate();
}


static int 
map_tmap(vm_translation_map *map, addr_t virtualAddress, addr_t physicalAddress, unsigned int attributes)
{
	// lookup the vsid based off the va
	uint32 virtualSegmentID = VADDR_TO_ASID(map, virtualAddress);

	if (attributes & LOCK_KERNEL)
		attributes = 0; // all kernel mappings are R/W to supervisor code
	else
		attributes = (attributes & LOCK_RW) ? PTE_READ_WRITE : PTE_READ_ONLY;

	//dprintf("vm_translation_map.map_tmap: vsid %d, pa 0x%lx, va 0x%lx\n", vsid, pa, va);

	// Search for a free page table slot using the primary hash value

	uint32 hash = page_table_entry::PrimaryHash(virtualSegmentID, virtualAddress);
	page_table_entry_group *group = &sPageTable[hash & sPageTableHashMask];

	for (int i = 0; i < 8; i++) {
		page_table_entry *entry = &group->entry[i];

		if (entry->valid)
			continue;

		fill_page_table_entry(entry, virtualSegmentID, virtualAddress, physicalAddress, 
			attributes, false);
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
			attributes, false);
		map->map_count++;
		return B_OK;
	}

	panic("vm_translation_map.map_tmap: hash table full\n");
	return B_ERROR;
}


static page_table_entry *
lookup_pagetable_entry(vm_translation_map *map, addr_t virtualAddress)
{
	// lookup the vsid based off the va
	uint32 virtualSegmentID = VADDR_TO_ASID(map, virtualAddress);

//	dprintf("vm_translation_map.lookup_pagetable_entry: vsid %d, va 0x%lx\n", vsid, va);


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


static int 
unmap_tmap(vm_translation_map *map, addr_t start, addr_t end)
{
	page_table_entry *entry;

	start = ROUNDOWN(start, PAGE_SIZE);
	end = ROUNDUP(end, PAGE_SIZE);

	dprintf("vm_translation_map.unmap_tmap: start 0x%lx, end 0x%lx\n", start, end);

	while (start < end) {
		entry = lookup_pagetable_entry(map, start);
		if (entry) {
			// unmap this page
			entry->valid = 0;
			arch_cpu_global_TLB_invalidate();
			map->map_count--;
		}

		start += B_PAGE_SIZE;
	}

	return 0;
}


static int
query_tmap(vm_translation_map *map, addr_t va, addr_t *out_physical, unsigned int *out_flags)
{
	page_table_entry *entry;

	// default the flags to not present
	*out_flags = 0;
	*out_physical = 0;

	entry = lookup_pagetable_entry(map, va);
	if (entry == NULL)
		return B_NO_ERROR;

	*out_flags |= (entry->page_protection == PTE_READ_ONLY) ? LOCK_RO : LOCK_RW;
	if (va >= KERNEL_BASE)
		*out_flags |= LOCK_KERNEL; // XXX is this enough?
	*out_flags |= entry->changed ? PAGE_MODIFIED : 0;
	*out_flags |= entry->referenced ? PAGE_ACCESSED : 0;
	*out_flags |= entry->valid ? PAGE_PRESENT : 0;

	*out_physical = entry->physical_page_number * B_PAGE_SIZE;

	return B_OK;
}


static addr_t 
get_mapped_size_tmap(vm_translation_map *map)
{
	return map->map_count;
}


static int 
protect_tmap(vm_translation_map *map, addr_t base, addr_t top, unsigned int attributes)
{
	// XXX finish
	return -1;
}


static int 
clear_flags_tmap(vm_translation_map *map, addr_t virtualAddress, unsigned int flags)
{
	page_table_entry *entry = lookup_pagetable_entry(map, virtualAddress);
	if (entry == NULL)
		return B_NO_ERROR;

	if (flags & PAGE_MODIFIED)
		entry->changed = false;
	if (flags & PAGE_ACCESSED)
		entry->referenced = false;

	arch_cpu_global_TLB_invalidate();

	return B_OK;
}


static void 
flush_tmap(vm_translation_map *map)
{
	arch_cpu_global_TLB_invalidate();
}


static int 
get_physical_page_tmap(addr_t pa, addr_t *va, int flags)
{
	pa = ROUNDOWN(pa, PAGE_SIZE);

	if(pa > IOSPACE_SIZE)
		panic("get_physical_page_tmap: asked for pa 0x%lx, cannot provide\n", pa);

	*va = (IOSPACE_BASE + pa);
	return 0;
}


static int 
put_physical_page_tmap(addr_t va)
{
	if (va < IOSPACE_BASE || va >= IOSPACE_BASE + IOSPACE_SIZE)
		panic("put_physical_page_tmap: va 0x%lx out of iospace region\n", va);

	return 0;
}


static vm_translation_map_ops tmap_ops = {
	destroy_tmap,
	lock_tmap,
	unlock_tmap,
	map_tmap,
	unmap_tmap,
	query_tmap,
	get_mapped_size_tmap,
	protect_tmap,
	clear_flags_tmap,
	flush_tmap,
	get_physical_page_tmap,
	put_physical_page_tmap
};


int 
vm_translation_map_create(vm_translation_map *new_map, bool kernel)
{
	// initialize the new object
	new_map->ops = &tmap_ops;
	new_map->map_count = 0;
	if (recursive_lock_init(&new_map->lock, "map lock") < B_OK)
		return B_NO_MEMORY;

	new_map->arch_data = (vm_translation_map_arch_info *)malloc(sizeof(vm_translation_map_arch_info));
	if (new_map->arch_data == NULL)
		return B_NO_MEMORY;

	cpu_status state = disable_interrupts();
	acquire_spinlock(&asid_bitmap_lock);

	// allocate a ASID base for this one
	if (kernel) {
		new_map->arch_data->asid_base = 0; // set up by the bootloader
		asid_bitmap[0] |= 0x1;
	} else {
		int i = 0;

		while (i < MAX_ASIDS) {
			if (asid_bitmap[i / 32] == 0xffffffff) {
				i += 32;
				continue;
			}
			if ((asid_bitmap[i / 32] & (1 << (i % 32))) == 0) {
				// we found it
				asid_bitmap[i / 32] |= 1 << (i % 32);
				break;
			}
			i++;
		}
		if (i >= MAX_ASIDS)
			panic("vm_translation_map_create: out of ASIDs\n");
		new_map->arch_data->asid_base = i;
	}

	release_spinlock(&asid_bitmap_lock);
	restore_interrupts(state);

	return 0;
}


int 
vm_translation_map_module_init(kernel_args *args)
{
	sPageTable = (page_table_entry_group *)args->arch_args.page_table.start;
	sPageTableSize = args->arch_args.page_table.size;
	sPageTableHashMask = sPageTableSize / sizeof(page_table_entry_group) - 1;

	return B_OK;
}


void 
vm_translation_map_module_init_post_sem(kernel_args *ka)
{
}


int 
vm_translation_map_module_init2(kernel_args *ka)
{
	// create a region to cover the page table
	sPageTableRegion = vm_create_anonymous_region(vm_get_kernel_aspace_id(), 
		"page_table", (void **)&sPageTable, REGION_ADDR_EXACT_ADDRESS, 
		sPageTableSize, REGION_WIRING_WIRED_ALREADY, LOCK_KERNEL | LOCK_RW);

	// ToDo: for now just map 0 - 512MB of physical memory to the iospace region
	block_address_translation bat;

	bat.Clear();
	bat.SetVirtualAddress((void *)IOSPACE_BASE);
	bat.page_index = IOSPACE_BASE;
	bat.length = BAT_LENGTH_256MB;
	bat.kernel_valid = true;
	bat.SetPhysicalAddress(NULL);
	bat.memory_coherent = true;
	bat.protection = BAT_READ_WRITE;

	set_ibat2(&bat);
	set_dbat2(&bat);
	isync();

	bat.SetVirtualAddress((void *)(IOSPACE_BASE + 256 * 1024 * 1024));
	bat.SetPhysicalAddress((void *)(256 * 1024 * 1024));

	set_ibat3(&bat);
	set_dbat3(&bat);
	isync();
/*
		int ibats[8], dbats[8];

		getibats(ibats);
		getdbats(dbats);

		// use bat 2 & 3 to do this
		ibats[4] = dbats[4] = IOSPACE_BASE | BATU_LEN_256M | BATU_VS;
		ibats[5] = dbats[5] = 0x0 | BATL_MC | BATL_PP_RW;
		ibats[6] = dbats[6] = (IOSPACE_BASE + 256*1024*1024) | BATU_LEN_256M | BATU_VS;
		ibats[7] = dbats[7] = (256*1024*1024) | BATL_MC | BATL_PP_RW;

		setibats(ibats);
		setdbats(dbats);
*/

	// create a region to cover the iospace
	void *temp = (void *)IOSPACE_BASE;
	vm_create_null_region(vm_get_kernel_aspace_id(), "iospace", &temp,
		REGION_ADDR_EXACT_ADDRESS, IOSPACE_SIZE);

	return 0;
}


/**	Directly maps a page without having knowledge of any kernel structures.
 *	Used only during VM setup.
 *	It currently ignores the "attributes" parameter and sets all pages
 *	read/write.
 */

int
vm_translation_map_quick_map(kernel_args *ka, addr_t virtualAddress, addr_t physicalAddress, uint8 attributes, addr_t (*get_free_page)(kernel_args *))
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

int 
vm_translation_map_quick_query(addr_t va, addr_t *out_physical)
{
	//PANIC_UNIMPLEMENTED();
	panic("vm_translation_map_quick_query(): not yet implemented\n");
	return 0;
}


void 
ppc_translation_map_change_asid(vm_translation_map *map)
{
// this code depends on the kernel being at 0x80000000, fix if we change that
#if KERNEL_BASE != 0x80000000
#error fix me
#endif
	int asid_base = map->arch_data->asid_base;

	asm("mtsr	0,%0" :: "g"(asid_base));
	asm("mtsr	1,%0" :: "g"(asid_base+1));
	asm("mtsr	2,%0" :: "g"(asid_base+2));
	asm("mtsr	3,%0" :: "g"(asid_base+3));
	asm("mtsr	4,%0" :: "g"(asid_base+4));
	asm("mtsr	5,%0" :: "g"(asid_base+5));
	asm("mtsr	6,%0" :: "g"(asid_base+6));
	asm("mtsr	7,%0" :: "g"(asid_base+7));
}

