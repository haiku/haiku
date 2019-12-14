/*
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_VM_H
#define _KERNEL_VM_VM_H

#include <OS.h>

#include <arch/vm.h>
#include <vm_defs.h>
#include <vm/vm_types.h>


struct generic_io_vec;
struct kernel_args;
struct ObjectCache;
struct system_memory_info;
struct VMAddressSpace;
struct VMArea;
struct VMCache;
struct vm_page;
struct vnode;
struct VMPageWiringInfo;


// area creation flags
#define CREATE_AREA_DONT_WAIT			0x01
#define CREATE_AREA_UNMAP_ADDRESS_RANGE	0x02
#define CREATE_AREA_DONT_CLEAR			0x04
#define CREATE_AREA_PRIORITY_VIP		0x08
#define CREATE_AREA_DONT_COMMIT_MEMORY	0x10

// memory/page allocation priorities
#define VM_PRIORITY_USER	0
#define VM_PRIORITY_SYSTEM	1
#define VM_PRIORITY_VIP		2

// page reserves
#define VM_PAGE_RESERVE_USER	512
#define VM_PAGE_RESERVE_SYSTEM	128

// memory reserves
#define VM_MEMORY_RESERVE_USER		(VM_PAGE_RESERVE_USER * B_PAGE_SIZE)
#define VM_MEMORY_RESERVE_SYSTEM	(VM_PAGE_RESERVE_SYSTEM * B_PAGE_SIZE)


extern struct ObjectCache* gPageMappingsObjectCache;


#ifdef __cplusplus
extern "C" {
#endif

// startup only
status_t vm_init(struct kernel_args *args);
status_t vm_init_post_sem(struct kernel_args *args);
status_t vm_init_post_thread(struct kernel_args *args);
status_t vm_init_post_modules(struct kernel_args *args);
void vm_free_kernel_args(struct kernel_args *args);
void vm_free_unused_boot_loader_range(addr_t start, addr_t end);
page_num_t vm_allocate_early_physical_page(kernel_args *args);
addr_t vm_allocate_early(struct kernel_args *args, size_t virtualSize,
			size_t physicalSize, uint32 attributes, addr_t alignment);

void slab_init(struct kernel_args *args);
void slab_init_post_area();
void slab_init_post_sem();
void slab_init_post_thread();

// to protect code regions with interrupts turned on
void permit_page_faults(void);
void forbid_page_faults(void);

// private kernel only extension (should be moved somewhere else):
area_id create_area_etc(team_id team, const char *name, size_t size,
			uint32 lock, uint32 protection, uint32 flags, uint32 guardSize,
			const virtual_address_restrictions* virtualAddressRestrictions,
			const physical_address_restrictions* physicalAddressRestrictions,
			void **_address);
area_id transfer_area(area_id id, void** _address, uint32 addressSpec,
			team_id target, bool kernel);

const char* vm_cache_type_to_string(int32 type);

status_t vm_prepare_kernel_area_debug_protection(area_id id, void** cookie);
status_t vm_set_kernel_area_debug_protection(void* cookie, void* _address,
			size_t size, uint32 protection);

status_t vm_block_address_range(const char* name, void* address, addr_t size);
status_t vm_unreserve_address_range(team_id team, void *address, addr_t size);
status_t vm_reserve_address_range(team_id team, void **_address,
			uint32 addressSpec, addr_t size, uint32 flags);
area_id vm_create_anonymous_area(team_id team, const char* name, addr_t size,
			uint32 wiring, uint32 protection, uint32 flags, addr_t guardSize,
			const virtual_address_restrictions* virtualAddressRestrictions,
			const physical_address_restrictions* physicalAddressRestrictions,
			bool kernel, void** _address);
area_id vm_map_physical_memory(team_id team, const char *name, void **address,
			uint32 addressSpec, addr_t size, uint32 protection,
			phys_addr_t physicalAddress, bool alreadyWired);
area_id vm_map_physical_memory_vecs(team_id team, const char* name,
	void** _address, uint32 addressSpec, addr_t* _size, uint32 protection,
	struct generic_io_vec* vecs, uint32 vecCount);
area_id vm_map_file(team_id aid, const char *name, void **address,
			uint32 addressSpec, addr_t size, uint32 protection, uint32 mapping,
			bool unmapAddressRange, int fd, off_t offset);
struct VMCache *vm_area_get_locked_cache(struct VMArea *area);
void vm_area_put_locked_cache(struct VMCache *cache);
area_id vm_create_null_area(team_id team, const char *name, void **address,
			uint32 addressSpec, addr_t size, uint32 flags);
area_id vm_copy_area(team_id team, const char *name, void **_address,
			uint32 addressSpec, area_id sourceID);
area_id vm_clone_area(team_id team, const char *name, void **address,
			uint32 addressSpec, uint32 protection, uint32 mapping,
			area_id sourceArea, bool kernel);
status_t vm_delete_area(team_id teamID, area_id areaID, bool kernel);
status_t vm_create_vnode_cache(struct vnode *vnode, struct VMCache **_cache);
status_t vm_set_area_memory_type(area_id id, phys_addr_t physicalBase,
			uint32 type);
status_t vm_set_area_protection(team_id team, area_id areaID,
			uint32 newProtection, bool kernel);
status_t vm_get_page_mapping(team_id team, addr_t vaddr, phys_addr_t *paddr);
bool vm_test_map_modification(struct vm_page *page);
void vm_clear_map_flags(struct vm_page *page, uint32 flags);
void vm_remove_all_page_mappings(struct vm_page *page);
int32 vm_clear_page_mapping_accessed_flags(struct vm_page *page);
int32 vm_remove_all_page_mappings_if_unaccessed(struct vm_page *page);
status_t vm_wire_page(team_id team, addr_t address, bool writable,
			struct VMPageWiringInfo* info);
void vm_unwire_page(struct VMPageWiringInfo* info);

status_t vm_get_physical_page(phys_addr_t paddr, addr_t* vaddr, void** _handle);
status_t vm_put_physical_page(addr_t vaddr, void* handle);
status_t vm_get_physical_page_current_cpu(phys_addr_t paddr, addr_t* vaddr,
			void** _handle);
status_t vm_put_physical_page_current_cpu(addr_t vaddr, void* handle);
status_t vm_get_physical_page_debug(phys_addr_t paddr, addr_t* vaddr,
			void** _handle);
status_t vm_put_physical_page_debug(addr_t vaddr, void* handle);

void vm_get_info(system_info *info);
uint32 vm_num_page_faults(void);
off_t vm_available_memory(void);
off_t vm_available_not_needed_memory(void);
off_t vm_available_not_needed_memory_debug(void);
size_t vm_kernel_address_space_left(void);

status_t vm_memset_physical(phys_addr_t address, int value, phys_size_t length);
status_t vm_memcpy_from_physical(void* to, phys_addr_t from, size_t length,
			bool user);
status_t vm_memcpy_to_physical(phys_addr_t to, const void* from, size_t length,
			bool user);
void vm_memcpy_physical_page(phys_addr_t to, phys_addr_t from);

status_t vm_debug_copy_page_memory(team_id teamID, void* unsafeMemory,
			void* buffer, size_t size, bool copyToUnsafe);

// user syscalls
area_id _user_create_area(const char *name, void **address, uint32 addressSpec,
			size_t size, uint32 lock, uint32 protection);
status_t _user_delete_area(area_id area);

area_id _user_map_file(const char *uname, void **uaddress, uint32 addressSpec,
			size_t size, uint32 protection, uint32 mapping,
			bool unmapAddressRange, int fd, off_t offset);
status_t _user_unmap_memory(void *address, size_t size);
status_t _user_set_memory_protection(void* address, size_t size,
			uint32 protection);
status_t _user_sync_memory(void *address, size_t size, uint32 flags);
status_t _user_memory_advice(void* address, size_t size, uint32 advice);
status_t _user_get_memory_properties(team_id teamID, const void *address,
			uint32 *_protected, uint32 *_lock);

status_t _user_mlock(const void* address, size_t size);
status_t _user_munlock(const void* address, size_t size);

area_id _user_area_for(void *address);
area_id _user_find_area(const char *name);
status_t _user_get_area_info(area_id area, area_info *info);
status_t _user_get_next_area_info(team_id team, ssize_t *cookie, area_info *info);
status_t _user_resize_area(area_id area, size_t newSize);
area_id _user_transfer_area(area_id area, void **_address, uint32 addressSpec,
			team_id target);
status_t _user_set_area_protection(area_id area, uint32 newProtection);
area_id _user_clone_area(const char *name, void **_address, uint32 addressSpec,
			uint32 protection, area_id sourceArea);
status_t _user_reserve_address_range(addr_t* userAddress, uint32 addressSpec,
			addr_t size);
status_t _user_unreserve_address_range(addr_t address, addr_t size);

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_VM_VM_H */
