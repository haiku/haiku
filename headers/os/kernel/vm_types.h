#ifndef _PUBLIC_KERNEL_VM_TYPES_H
#define _PUBLIC_KERNEL_VM_TYPES_H

#include <kernel.h>
#include <stage2.h>
#include <defines.h>
#include <vfs.h>
#include <arch/vm_translation_map.h>

// vm page
typedef struct vm_page {
	struct vm_page *queue_prev;
	struct vm_page *queue_next;

	struct vm_page *hash_next;

	addr ppn; // physical page number
	off_t offset;

	struct vm_cache_ref *cache_ref;

	struct vm_page *cache_prev;
	struct vm_page *cache_next;

	unsigned int ref_count;

	unsigned int type : 2;
	unsigned int state : 3;
} vm_page;

enum {
	PAGE_TYPE_PHYSICAL = 0,
	PAGE_TYPE_DUMMY,
	PAGE_TYPE_GUARD
};

enum {
	PAGE_STATE_ACTIVE = 0,
	PAGE_STATE_INACTIVE,
	PAGE_STATE_BUSY,
	PAGE_STATE_MODIFIED,
	PAGE_STATE_FREE,
	PAGE_STATE_CLEAR,
	PAGE_STATE_WIRED,
	PAGE_STATE_UNUSED
};

// vm_cache_ref
typedef struct vm_cache_ref {
	struct vm_cache *cache;
	mutex lock;

	struct vm_region *region_list;

	int ref_count;
} vm_cache_ref;

// vm_cache
typedef struct vm_cache {
	vm_page *page_list;
	vm_cache_ref *ref;
	struct vm_cache *source;
	struct vm_store *store;
	off_t virtual_size;
	unsigned int temporary : 1;
	unsigned int scan_skip : 1;
} vm_cache;

// info about a region that external entities may want to know
// used in vm_get_region_info()
typedef struct vm_region_info {
	region_id id;
	addr base;
	addr size;
	int lock;
	int wiring;
	char name[SYS_MAX_OS_NAME_LEN];
} vm_region_info;

// vm region
typedef struct vm_region {
	char *name;
	region_id id;
	addr base;
	addr size;
	int lock;
	int wiring;
	int ref_count;

	struct vm_cache_ref *cache_ref;
	off_t cache_offset;

	struct vm_address_space *aspace;
	struct vm_region *aspace_next;
	struct vm_virtual_map *map;
	struct vm_region *cache_next;
	struct vm_region *cache_prev;
	struct vm_region *hash_next;
} vm_region;

// virtual map (1 per address space)
typedef struct vm_virtual_map {
	vm_region *region_list;
	vm_region *region_hint;
	int change_count;
	sem_id sem;
	struct vm_address_space *aspace;
	addr base;
	addr size;
} vm_virtual_map;

enum {
	VM_ASPACE_STATE_NORMAL = 0,
	VM_ASPACE_STATE_DELETION
};

// address space
typedef struct vm_address_space {
	vm_virtual_map virtual_map;
	vm_translation_map translation_map;
	char *name;
	aspace_id id;
	int ref_count;
	int fault_count;
	int state;
	addr scan_va;
	addr working_set_size;
	addr max_working_set;
	addr min_working_set;
	bigtime_t last_working_set_adjust;
	struct vm_address_space *hash_next;
} vm_address_space;

// vm_store
typedef struct vm_store {
	struct vm_store_ops *ops;
	struct vm_cache *cache;
	void *data;
	off_t committed_size;
} vm_store;

// vm_store_ops
typedef struct vm_store_ops {
	void (*destroy)(struct vm_store *backing_store);
	off_t (*commit)(struct vm_store *backing_store, off_t size);
	int (*has_page)(struct vm_store *backing_store, off_t offset);
	ssize_t (*read)(struct vm_store *backing_store, off_t offset, iovecs *vecs);
	ssize_t (*write)(struct vm_store *backing_store, off_t offset, iovecs *vecs);
	int (*fault)(struct vm_store *backing_store, struct vm_address_space *aspace, off_t offset);
	void (*acquire_ref)(struct vm_store *backing_store);
	void (*release_ref)(struct vm_store *backing_store);
} vm_store_ops;

// args for the create_area funcs
enum {
	REGION_ADDR_ANY_ADDRESS = 0,
	REGION_ADDR_EXACT_ADDRESS
};

enum {
	REGION_NO_PRIVATE_MAP = 0,
	REGION_PRIVATE_MAP
};

enum {
	REGION_WIRING_LAZY = 0,
	REGION_WIRING_WIRED,
	REGION_WIRING_WIRED_ALREADY,
	REGION_WIRING_WIRED_CONTIG
 };

enum {
	PHYSICAL_PAGE_NO_WAIT = 0,
	PHYSICAL_PAGE_CAN_WAIT,
};

#define LOCK_RO        0x0
#define LOCK_RW        0x1
#define LOCK_KERNEL    0x2
#define LOCK_MASK      0x3

#endif /* _PUBLIC_KERNEL_VM_TYPES_H */

