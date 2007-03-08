/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_H
#define _KERNEL_VM_H


#include <vm_types.h>
#include <arch/vm.h>
#include <arch/vm_translation_map.h>

struct kernel_args;
struct team;


#ifdef __cplusplus
extern "C" {
#endif

// startup only
status_t vm_init(kernel_args *args);
status_t vm_init_post_sem(struct kernel_args *args);
status_t vm_init_post_thread(struct kernel_args *args);
status_t vm_init_post_modules(struct kernel_args *args);
void vm_free_kernel_args(kernel_args *args);
void vm_free_unused_boot_loader_range(addr_t start, addr_t end);
addr_t vm_allocate_early(kernel_args *args, size_t virtualSize,
			size_t physicalSize, uint32 attributes);

// to protect code regions with interrupts turned on
void permit_page_faults(void);
void forbid_page_faults(void);

// private kernel only extension (should be moved somewhere else):
area_id create_area_etc(struct team *team, const char *name, void **address, 
			uint32 addressSpec, uint32 size, uint32 lock, uint32 protection);
status_t delete_area_etc(struct team *team, area_id area);

status_t vm_unreserve_address_range(team_id team, void *address, addr_t size);
status_t vm_reserve_address_range(team_id team, void **_address, 
			uint32 addressSpec, addr_t size, uint32 flags);
area_id vm_create_anonymous_area(team_id team, const char *name, void **address, 
			uint32 addressSpec, addr_t size, uint32 wiring, uint32 protection);
area_id vm_map_physical_memory(team_id team, const char *name, void **address, 
			uint32 addressSpec, addr_t size, uint32 protection, addr_t phys_addr);
area_id vm_map_file(team_id aid, const char *name, void **address, 
			uint32 addressSpec, addr_t size, uint32 protection, uint32 mapping, 
			const char *path, off_t offset);
area_id vm_create_null_area(team_id team, const char *name, void **address, 
			uint32 addressSpec, addr_t size);
area_id vm_copy_area(team_id team, const char *name, void **_address, 
			uint32 addressSpec, uint32 protection, area_id sourceID);
area_id vm_clone_area(team_id team, const char *name, void **address, 
			uint32 addressSpec, uint32 protection, uint32 mapping, 
			area_id sourceArea);
status_t vm_delete_area(team_id aid, area_id id);
status_t vm_create_vnode_cache(void *vnode, vm_cache_ref **_cacheRef);
vm_area *vm_area_lookup(vm_address_space *addressSpace, addr_t address);
status_t vm_set_area_memory_type(area_id id, addr_t physicalBase, uint32 type);
status_t vm_get_page_mapping(team_id team, addr_t vaddr, addr_t *paddr);
void vm_remove_all_page_mappings(vm_page *page);
status_t vm_unmap_pages(vm_area *area, addr_t base, size_t length);
status_t vm_map_page(vm_area *area, vm_page *page, addr_t address,
			uint32 protection);
status_t vm_get_physical_page(addr_t paddr, addr_t *vaddr, uint32 flags);
status_t vm_put_physical_page(addr_t vaddr);

// user syscalls
area_id _user_create_area(const char *name, void **address, uint32 addressSpec,
			size_t size, uint32 lock, uint32 protection);
status_t _user_delete_area(area_id area);
area_id _user_vm_map_file(const char *uname, void **uaddress, int addr_type,
			addr_t size, int lock, int mapping, const char *upath, off_t offset);
area_id _user_area_for(void *address);
area_id _user_find_area(const char *name);
status_t _user_get_area_info(area_id area, area_info *info);
status_t _user_get_next_area_info(team_id team, int32 *cookie, area_info *info);
status_t _user_resize_area(area_id area, size_t newSize);
status_t _user_transfer_area(area_id area, void **_address, uint32 addressSpec, 
			team_id target);
status_t _user_set_area_protection(area_id area, uint32 newProtection);
area_id _user_clone_area(const char *name, void **_address, uint32 addressSpec, 
			uint32 protection, area_id sourceArea);
status_t _user_reserve_heap_address_range(addr_t* userAddress, uint32 addressSpec,
			addr_t size);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_VM_H */
