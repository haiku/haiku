/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel/kernel.h>
#include <kernel/arch/vm_translation_map.h>
#include <boot/stage2.h>

static int lock_tmap(vm_translation_map *map)
{
	return 0;
}

static int unlock_tmap(vm_translation_map *map)
{
	return 0;
}

static void destroy_tmap(vm_translation_map *map)
{
}

static int map_tmap(vm_translation_map *map, addr va, addr pa, unsigned int attributes)
{
	return 0;
}

static int unmap_tmap(vm_translation_map *map, addr start, addr end)
{
	return 0;
}

static int query_tmap(vm_translation_map *map, addr va, addr *out_physical, unsigned int *out_flags)
{
	return 0;
}

static addr get_mapped_size_tmap(vm_translation_map *map)
{
	return 0;
}

static int protect_tmap(vm_translation_map *map, addr base, addr top, unsigned int attributes)
{
	// XXX finish
	return -1;
}

static int get_physical_page_tmap(addr pa, addr *va, int flags)
{
	return 0;
}

static int put_physical_page_tmap(addr va)
{
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
	get_physical_page_tmap,
	put_physical_page_tmap
};

int vm_translation_map_create(vm_translation_map *new_map, bool kernel)
{
	return 0;
}

int vm_translation_map_module_init(kernel_args *ka)
{
	return 0;
}

void vm_translation_map_module_init_post_sem(kernel_args *ka)
{
	return 0;
}

int vm_translation_map_module_init2(kernel_args *ka)
{
	return 0;
}

// XXX horrible back door to map a page quickly regardless of translation map object, etc.
// used only during VM setup.
// uses a 'page hole' set up in the stage 2 bootloader. The page hole is created by pointing one of
// the pgdir entries back at itself, effectively mapping the contents of all of the 4MB of pagetables
// into a 4 MB region. It's only used here, and is later unmapped.
int vm_translation_map_quick_map(kernel_args *ka, addr va, addr pa, unsigned int attributes, addr (*get_free_page)(kernel_args *))
{
	return 0;
}

// XXX currently assumes this translation map is active
static int vm_translation_map_quick_query(addr va, addr *out_physical)
{
	return 0;
}

