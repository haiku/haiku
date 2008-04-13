/*
 * Copyright 2002-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_H
#define _KERNEL_VM_H


#include <arch/vm.h>

#include <OS.h>


struct kernel_args;
struct team;
struct vm_page;
struct vm_cache;
struct vm_area;
struct vm_address_space;
struct vnode;


// additional protection flags
// Note: the VM probably won't support all combinations - it will try
// its best, but create_area() will fail if it has to.
// Of course, the exact behaviour will be documented somewhere...
#define B_EXECUTE_AREA			0x04
#define B_STACK_AREA			0x08
	// "stack" protection is not available on most platforms - it's used
	// to only commit memory as needed, and have guard pages at the
	// bottom of the stack.
	// "execute" protection is currently ignored, but nevertheless, you
	// should use it if you require to execute code in that area.

#define B_KERNEL_EXECUTE_AREA	0x40
#define B_KERNEL_STACK_AREA		0x80

#define B_USER_PROTECTION \
	(B_READ_AREA | B_WRITE_AREA | B_EXECUTE_AREA | B_STACK_AREA)
#define B_KERNEL_PROTECTION \
	(B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA | B_KERNEL_EXECUTE_AREA \
	| B_KERNEL_STACK_AREA)

#define B_OVERCOMMITTING_AREA	0x1000
#define B_SHARED_AREA			0x2000
	// TODO: These aren't really a protection flags, but since the "protection"
	//	field is the only flag field, we currently use it for this.
	//	A cleaner approach would be appreciated - maybe just an official generic
	//	flags region in the protection field.


#define B_USER_AREA_FLAGS		(B_USER_PROTECTION)
#define B_KERNEL_AREA_FLAGS \
	(B_KERNEL_PROTECTION | B_USER_CLONEABLE_AREA | B_OVERCOMMITTING_AREA \
		| B_SHARED_AREA)

// flags for vm_get_physical_page()
enum {
	PHYSICAL_PAGE_NO_WAIT = 0,
	PHYSICAL_PAGE_CAN_WAIT,
};

// mapping argument for several internal VM functions
enum {
	REGION_NO_PRIVATE_MAP = 0,
	REGION_PRIVATE_MAP
};

enum {
	// ToDo: these are here only temporarily - it's a private
	//	addition to the BeOS create_area() lock flags
	B_ALREADY_WIRED	= 6,
};

#define MEMORY_TYPE_SHIFT		28


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
addr_t vm_allocate_early(struct kernel_args *args, size_t virtualSize,
			size_t physicalSize, uint32 attributes);

void slab_init(struct kernel_args *args, addr_t initialBase,
	size_t initialSize);
void slab_init_post_sem();

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
			uint32 addressSpec, addr_t size, uint32 wiring, uint32 protection,
			bool unmapAddressRange);
area_id vm_map_physical_memory(team_id team, const char *name, void **address,
			uint32 addressSpec, addr_t size, uint32 protection, addr_t phys_addr);
area_id vm_map_file(team_id aid, const char *name, void **address,
			uint32 addressSpec, addr_t size, uint32 protection, uint32 mapping,
			int fd, off_t offset);
struct vm_cache *vm_area_get_locked_cache(struct vm_area *area);
void vm_area_put_locked_cache(struct vm_cache *cache);
area_id vm_create_null_area(team_id team, const char *name, void **address,
			uint32 addressSpec, addr_t size);
area_id vm_copy_area(team_id team, const char *name, void **_address,
			uint32 addressSpec, uint32 protection, area_id sourceID);
area_id vm_clone_area(team_id team, const char *name, void **address,
			uint32 addressSpec, uint32 protection, uint32 mapping,
			area_id sourceArea);
status_t vm_delete_area(team_id teamID, area_id areaID);
status_t vm_create_vnode_cache(struct vnode *vnode, struct vm_cache **_cache);
struct vm_area *vm_area_lookup(struct vm_address_space *addressSpace,
			addr_t address);
status_t vm_set_area_memory_type(area_id id, addr_t physicalBase, uint32 type);
status_t vm_get_page_mapping(team_id team, addr_t vaddr, addr_t *paddr);
bool vm_test_map_modification(struct vm_page *page);
int32 vm_test_map_activation(struct vm_page *page, bool *_modified);
void vm_clear_map_flags(struct vm_page *page, uint32 flags);
void vm_remove_all_page_mappings(struct vm_page *page, uint32 *_flags);
status_t vm_unmap_pages(struct vm_area *area, addr_t base, size_t length,
			bool preserveModified);
status_t vm_map_page(struct vm_area *area, struct vm_page *page, addr_t address,
			uint32 protection);
status_t vm_get_physical_page(addr_t paddr, addr_t *vaddr, uint32 flags);
status_t vm_put_physical_page(addr_t vaddr);

off_t vm_available_memory(void);

// user syscalls
area_id _user_create_area(const char *name, void **address, uint32 addressSpec,
			size_t size, uint32 lock, uint32 protection);
status_t _user_delete_area(area_id area);
area_id _user_map_file(const char *uname, void **uaddress, int addressSpec,
			addr_t size, int protection, int mapping, int fd, off_t offset);
status_t _user_unmap_memory(void *address, addr_t size);
area_id _user_area_for(void *address);
area_id _user_find_area(const char *name);
status_t _user_get_area_info(area_id area, area_info *info);
status_t _user_get_next_area_info(team_id team, int32 *cookie, area_info *info);
status_t _user_resize_area(area_id area, size_t newSize);
area_id _user_transfer_area(area_id area, void **_address, uint32 addressSpec,
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
