/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_VM_H
#define _KERNEL_VM_H


#include <kernel.h>
#include <defines.h>
#include <vfs.h>
#include <vm_types.h>
#include <arch/vm_translation_map.h>

struct kernel_args;


#ifdef __cplusplus
extern "C" {
#endif

//void vm_dump_areas(vm_address_space *aspace);
status_t vm_init(kernel_args *ka);
status_t vm_init_postsem(struct kernel_args *ka);
status_t vm_init_postthread(struct kernel_args *ka);
status_t vm_aspace_init(void);
status_t vm_aspace_init_post_sem(void);

status_t vm_create_aspace(const char *name, addr base, addr size, bool kernel, vm_address_space **_aspace);
status_t vm_delete_areas(struct vm_address_space *aspace);
vm_address_space *vm_get_kernel_aspace(void);
aspace_id vm_get_kernel_aspace_id(void);
vm_address_space *vm_get_current_user_aspace(void);
aspace_id vm_get_current_user_aspace_id(void);
vm_address_space *vm_get_aspace_by_id(aspace_id aid);
void vm_put_aspace(vm_address_space *aspace);
vm_region *vm_get_region_by_id(region_id rid);
void vm_put_region(vm_region *region);
#define vm_aspace_swap(aspace) arch_vm_aspace_swap(aspace)

// private kernel only extension (should be moved somewhere else):
struct team;
area_id create_area_etc(struct team *team, const char *name, void **address, uint32 addressSpec,
	uint32 size, uint32 lock, uint32 protection);
status_t delete_area_etc(struct team *team, area_id area);

status_t vm_unreserve_address_range(aspace_id aid, void *address, addr_t size);
status_t vm_reserve_address_range(aspace_id aid, void **_address, uint32 addressSpec, addr_t size);
region_id vm_create_anonymous_region(aspace_id aid, const char *name, void **address, int addr_type,
	addr size, int wiring, int lock);
region_id vm_map_physical_memory(aspace_id aid, const char *name, void **address, int addr_type,
	addr size, int lock, addr phys_addr);
region_id vm_map_file(aspace_id aid, char *name, void **address, int addr_type,
	addr size, int lock, int mapping, const char *path, off_t offset);
region_id vm_create_null_region(aspace_id aid, char *name, void **address, int addr_type, addr size);
area_id vm_copy_area(aspace_id addressSpaceID, const char *name, void **_address, uint32 addressSpec,
			uint32 protection, area_id sourceID);
region_id vm_clone_region(aspace_id aid, char *name, void **address, int addr_type,
	region_id source_region, int mapping, int lock);
status_t vm_delete_region(aspace_id aid, region_id id);
region_id vm_find_region_by_name(aspace_id aid, const char *name);
status_t vm_create_vnode_cache(void *vnode, void **_cache);

status_t vm_get_page_mapping(aspace_id aid, addr vaddr, addr *paddr);
status_t vm_get_physical_page(addr paddr, addr *vaddr, int flags);
status_t vm_put_physical_page(addr vaddr);

area_id _user_create_area(const char *name, void **address, uint32 addressSpec,
			size_t size, uint32 lock, uint32 protection);
status_t _user_delete_area(area_id area);
region_id _user_vm_map_file(const char *uname, void **uaddress, int addr_type,
			addr size, int lock, int mapping, const char *upath, off_t offset);
area_id _user_area_for(void *address);
area_id _user_find_area(const char *name);
status_t _user_get_area_info(area_id area, area_info *info);
status_t _user_get_next_area_info(team_id team, int32 *cookie, area_info *info);
status_t _user_resize_area(area_id area, size_t newSize);
status_t _user_set_area_protection(area_id area, uint32 newProtection);
area_id _user_clone_area(const char *name, void **_address, uint32 addressSpec, 
			uint32 protection, area_id sourceArea);

region_id find_region_by_name(const char *);
region_id find_region_by_address (addr);
int vm_resize_region (aspace_id, region_id, size_t);

// to protect code regions with interrupts turned on
void permit_page_faults(void);
void forbid_page_faults(void);

// XXX remove later
void vm_test(void);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_VM_H */
