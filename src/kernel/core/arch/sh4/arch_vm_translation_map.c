/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel/kernel.h>
#include <kernel/arch/vm_translation_map.h>
#include <kernel/heap.h>
#include <kernel/int.h>
#include <kernel/smp.h>
#include <kernel/vm.h>
#include <kernel/vm_page.h>
#include <kernel/vm_priv.h>
#include <kernel/arch/cpu.h>
#include <kernel/debug.h>
#include <nulibc/string.h>
#include <boot/stage2.h>

#include <arch/sh4/sh4.h>
#include <arch/sh4/vcpu.h>

typedef struct vm_translation_map_arch_info_struct {
	addr pgdir_virt;
	addr pgdir_phys;
	bool is_user;
} vm_translation_map_arch_info;

static vcpu_struct *vcpu;

#define CHATTY_TMAP 0

#define PHYS_TO_P1(x) ((x) + P1_AREA)

static void destroy_tmap(vm_translation_map *map)
{
	int state;
	vm_translation_map *entry;
	vm_translation_map *last = NULL;

	if(map == NULL)
		return;

	if(map->arch_data->pgdir_virt != NULL)
		kfree((void *)map->arch_data->pgdir_virt);

	kfree(map->arch_data);
}

static int lock_tmap(vm_translation_map *map)
{
//	dprintf("lock_tmap: map 0x%x\n", map);
	if(recursive_lock_lock(&map->lock) == true) {
		// we were the first one to grab the lock
//		dprintf("clearing invalidated page count\n");
//		map->arch_data->num_invalidate_pages = 0;
	}

	return 0;}

static int unlock_tmap(vm_translation_map *map)
{
	if(recursive_lock_get_recursion(&map->lock) == 1) {
		// XXX flush any TLB ents we wanted to
	}
	recursive_lock_unlock(&map->lock);
	return -1;
}

static int map_tmap(vm_translation_map *map, addr va, addr pa, unsigned int lock)
{
	struct pdent *pd = NULL;
	struct ptent *pt;
	unsigned int index;

#if CHATTY_TMAP
	dprintf("map_tmap: va 0x%x pa 0x%x lock 0x%x\n", va, pa, lock);
#endif

	if(map->arch_data->is_user) {
		if(va >= P1_AREA) {
			// invalid
			panic("map_tmap: asked to map va 0x%x in user space aspace\n", va);
		}
		pd = (struct pdent *)map->arch_data->pgdir_phys;
		index = va >> 22;
	} else {
		if(va < P3_AREA && va >= P4_AREA) {
			panic("map_tmap: asked to map va 0x%x in kernel space aspace\n", va);
		}
		pd = (struct pdent *)vcpu->kernel_pgdir;
		index = (va & 0x7fffffff) >> 22;
	}

	if(pd[index].v == 0) {
		vm_page *page;
		unsigned int pgtable;

		page = vm_page_allocate_page(PAGE_STATE_CLEAR);
		pgtable = page->ppn * PAGE_SIZE;

		// XXX remove when real clear pages support is there
		memset((void *)PHYS_TO_P1(pgtable), 0, PAGE_SIZE);

		pd[index].ppn = pgtable >> 12;
		pd[index].v = 1;
	}

	// get the pagetable
	pt = (struct ptent *)PHYS_TO_P1(pd[index].ppn << 12);
	index = (va >> 12) & 0x000003ff;

	if(pt[index].v)
		panic("map_tmap: va 0x%x already mapped to pa 0x%x\n", va, pt[index].ppn << 12);

	// insert the mapping
	pt[index].wt = 0;
	pt[index].pr = (lock & LOCK_KERNEL) ? (lock & 0x1) : (lock | 0x2);
	pt[index].ppn = pa >> 12;
	pt[index].tlb_ent = 0;
	pt[index].c = 1;
	pt[index].sz = 0x1; // 4k page
	pt[index].sh = 0;
	pt[index].d = 0;
	pt[index].v = 1;

	sh4_invl_page(va);

	return 0;
}

static int unmap_tmap(vm_translation_map *map, addr start, addr end)
{
	struct pdent *pd;
	struct ptent *pt;
	unsigned int index;

	start = ROUNDOWN(start, PAGE_SIZE);
	end = ROUNDUP(end, PAGE_SIZE);

#if CHATTY_TMAP
		dprintf("unmap_tmap: asked to free pages 0x%x to 0x%x\n", start, end);
#endif

	for(; start < end; start += PAGE_SIZE) {
		if(map->arch_data->is_user) {
			if(start >= P1_AREA) {
				// invalid
				panic("unmap_tmap: asked to unmap va 0x%x in user space aspace\n", start);
			}
			pd = (struct pdent *)map->arch_data->pgdir_phys;
			index = start >> 22;
		} else {
			if(start < P3_AREA && start >= P4_AREA) {
				panic("unmap_tmap: asked to unmap va 0x%x in kernel space aspace\n", start);
			}
			pd = (struct pdent *)vcpu->kernel_pgdir;
			index = (start & 0x7fffffff) >> 22;
		}

		if(pd[index].v == 0)
			continue;

		// get the pagetable
		pt = (struct ptent *)PHYS_TO_P1(pd[index].ppn << 12);
		index = (start >> 12) & 0x000003ff;

		if(pt[index].v == 0)
			continue;

		pt[index].v = 0;

		sh4_invl_page(start);
	}
	return 0;
}

static int query_tmap(vm_translation_map *map, addr va, addr *out_physical, unsigned int *out_flags)
{
	struct pdent *pd;
	struct ptent *pt;
	unsigned int index;

	// default the flags to not present
	*out_flags = 0;
	*out_physical = 0;

	if(map->arch_data->is_user) {
		if(va >= P1_AREA) {
			// invalid
			return ERR_VM_GENERAL;
		}
		pd = (struct pdent *)map->arch_data->pgdir_phys;
		index = va >> 22;
	} else {
		if(va < P3_AREA && va >= P4_AREA) {
			return ERR_VM_GENERAL;
		}
		pd = (struct pdent *)vcpu->kernel_pgdir;
		index = (va & 0x7fffffff) >> 22;
	}

	if(pd[index].v == 0) {
		return NO_ERROR;
	}

	// get the pagetable
	pt = (struct ptent *)PHYS_TO_P1(pd[index].ppn << 12);
	index = (va >> 12) & 0x000003ff;

	*out_physical = pt[index].ppn  << 12;

	// read in the page state flags, clearing the modified and accessed flags in the process
	*out_flags = 0;
	*out_flags |= (pt[index].pr & 0x1) ? LOCK_RW : LOCK_RO;
	*out_flags |= (pt[index].pr & 0x2) ? 0 : LOCK_KERNEL;
	*out_flags |= pt[index].d ? PAGE_MODIFIED : 0;
//	*out_flags |= pt[index].accessed ? PAGE_ACCESSED : 0; // not emulating the accessed bit yet
	*out_flags |= pt[index].v ? PAGE_PRESENT : 0;

	return 0;
}

static addr get_mapped_size_tmap(vm_translation_map *map)
{
	return map->map_count;
}

static int protect_tmap(vm_translation_map *map, addr base, addr top, unsigned int attributes)
{
	// XXX finish
	return -1;
}

static int clear_flags_tmap(vm_translation_map *map, addr va, unsigned int flags)
{
	struct pdent *pd;
	struct ptent *pt;
	unsigned int index;
	int tlb_flush = false;

	if(map->arch_data->is_user) {
		if(va >= P1_AREA) {
			// invalid
			return ERR_VM_GENERAL;
		}
		pd = (struct pdent *)map->arch_data->pgdir_phys;
		index = va >> 22;
	} else {
		if(va < P3_AREA && va >= P4_AREA) {
			return ERR_VM_GENERAL;
		}
		pd = (struct pdent *)vcpu->kernel_pgdir;
		index = (va & 0x7fffffff) >> 22;
	}

	if(pd[index].v == 0) {
		return NO_ERROR;
	}

	// get the pagetable
	pt = (struct ptent *)PHYS_TO_P1(pd[index].ppn << 12);
	index = (va >> 12) & 0x000003ff;

	// clear out the flags we've been requested to clear
	if(flags & PAGE_MODIFIED) {
		pt[index].d = 0;
		tlb_flush = true;
	}
	if(flags & PAGE_ACCESSED) {
//		pt[index].accessed = 0;
//		tlb_flush = true;
	}

	if(tlb_flush)
		sh4_invl_page(va);

	return 0;
}

static void flush_tmap(vm_translation_map *map)
{
	// no-op, we aren't caching any tlb invalidations
}

static int get_physical_page_tmap(addr pa, addr *va, int flags)
{
	if(pa >= PHYS_ADDR_SIZE)
		panic("get_physical_page_tmap: passed invalid address 0x%x\n", pa);
	*va = PHYS_ADDR_TO_P1(pa);
	return NO_ERROR;
}

static int put_physical_page_tmap(addr va)
{
	if(va < P1_AREA && va >= P2_AREA)
		panic("put_physical_page_tmap: bad address passed 0x%x\n", va);
	return NO_ERROR;
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

int vm_translation_map_create(vm_translation_map *new_map, bool kernel)
{
	if(new_map == NULL)
		return -1;

	// initialize the new object
	new_map->ops = &tmap_ops;
	new_map->map_count = 0;
	if(recursive_lock_create(&new_map->lock) < 0)
		return ERR_NO_MEMORY;

	new_map->arch_data = kmalloc(sizeof(vm_translation_map_arch_info));
	if(new_map == NULL)
		panic("error allocating translation map object!\n");

	if(!kernel) {
		// user
		// allocate a pgdir
		new_map->arch_data->pgdir_virt = (addr)kmalloc(PAGE_SIZE);
		if(new_map->arch_data->pgdir_virt == NULL) {
			kfree(new_map->arch_data);
			return -1;
		}
		if(((addr)new_map->arch_data->pgdir_virt % PAGE_SIZE) != 0)
			panic("vm_translation_map_create: malloced pgdir and found it wasn't aligned!\n");
		vm_get_page_mapping(vm_get_kernel_aspace_id(), (addr)new_map->arch_data->pgdir_virt, (addr *)&new_map->arch_data->pgdir_phys);
		new_map->arch_data->pgdir_phys = PHYS_TO_P1(new_map->arch_data->pgdir_phys);
		// zero out the new pgdir
		memset((void *)new_map->arch_data->pgdir_virt, 0, PAGE_SIZE);
		new_map->arch_data->is_user = true;
	} else {
		// kernel
		// we already know the kernel pgdir mapping
		(addr)new_map->arch_data->pgdir_virt = NULL;
		(addr)new_map->arch_data->pgdir_phys = vcpu->kernel_pgdir;
		new_map->arch_data->is_user = false;
	}

	return 0;
}

int vm_translation_map_module_init(kernel_args *ka)
{
	vcpu = ka->arch_args.vcpu;

	return 0;
}

int vm_translation_map_module_init2(kernel_args *ka)
{
	return 0;
}

void vm_translation_map_module_init_post_sem(kernel_args *ka)
{
}

// XXX horrible back door to map a page quickly regardless of translation map object, etc.
// used only during VM setup
int vm_translation_map_quick_map(kernel_args *ka, addr va, addr pa, unsigned int lock, addr (*get_free_page)(kernel_args *))
{
	struct pdent *pd = NULL;
	struct ptent *pt;
	unsigned int index;

#if CHATTY_TMAP
	dprintf("quick_tmap: va 0x%x pa 0x%x lock 0x%x\n", va, pa, lock);
#endif

	if(va < P3_AREA && va >= P4_AREA) {
		panic("quick_tmap: asked to map invalid va 0x%x\n", va);
	}
	pd = (struct pdent *)vcpu->kernel_pgdir;
	index = (va & 0x7fffffff) >> 22;

	if(pd[index].v == 0) {
		vm_page *page;
		unsigned int pgtable;

		pgtable = (*get_free_page)(ka) * PAGE_SIZE;

		memset((void *)PHYS_TO_P1(pgtable), 0, PAGE_SIZE);

		pd[index].ppn = pgtable >> 12;
		pd[index].v = 1;
	}

	// get the pagetable
	pt = (struct ptent *)PHYS_TO_P1(pd[index].ppn << 12);
	index = (va >> 12) & 0x000003ff;

	// insert the mapping
	pt[index].wt = 0;
	pt[index].pr = (lock & LOCK_KERNEL) ? (lock & 0x1) : (lock | 0x2);
	pt[index].ppn = pa >> 12;
	pt[index].tlb_ent = 0;
	pt[index].c = 1;
	pt[index].sz = 0x1; // 4k page
	pt[index].sh = 0;
	pt[index].d = 0;
	pt[index].v = 1;

	sh4_invl_page(va);

	return 0;
}

addr vm_translation_map_get_pgdir(vm_translation_map *map)
{
	return (addr)map->arch_data->pgdir_phys;
}
