/* 
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_TYPES_H
#define _KERNEL_VM_TYPES_H


#include <arch/vm_types.h>
#include <arch/vm_translation_map.h>
#include <condition_variable.h>
#include <kernel.h>
#include <util/DoublyLinkedQueue.h>

#include <sys/uio.h>


// Enables the vm_page::queue, i.e. it is tracked which queue the page should
// be in.
//#define DEBUG_PAGE_QUEUE	1

// Enables extra debug fields in the vm_page used to track page transitions
// between caches.
//#define DEBUG_PAGE_CACHE_TRANSITIONS	1

// Enables a global list of all vm_cache structures.
//#define DEBUG_CACHE_LIST 1

#ifdef __cplusplus
struct vm_page_mapping;
typedef DoublyLinkedListLink<vm_page_mapping> vm_page_mapping_link;

typedef struct vm_page_mapping {
	vm_page_mapping_link page_link;
	vm_page_mapping_link area_link;
	struct vm_page *page;
	struct vm_area *area;
} vm_page_mapping;

class DoublyLinkedPageLink {
	public:
		inline vm_page_mapping_link *operator()(vm_page_mapping *element) const
		{
			return &element->page_link;
		}

		inline const vm_page_mapping_link *operator()(const vm_page_mapping *element) const
		{
			return &element->page_link;
		}
};

class DoublyLinkedAreaLink {
	public:
		inline vm_page_mapping_link *operator()(vm_page_mapping *element) const
		{
			return &element->area_link;
		}

		inline const vm_page_mapping_link *operator()(const vm_page_mapping *element) const
		{
			return &element->area_link;
		}
};

typedef class DoublyLinkedQueue<vm_page_mapping, DoublyLinkedPageLink> vm_page_mappings;
typedef class DoublyLinkedQueue<vm_page_mapping, DoublyLinkedAreaLink> vm_area_mappings;

struct vm_page {
	struct vm_page		*queue_prev;
	struct vm_page		*queue_next;

	struct vm_page		*hash_next;

	addr_t				physical_page_number;

	struct vm_cache		*cache;
	uint32				cache_offset;
							// in page size units

	struct vm_page		*cache_prev;
	struct vm_page		*cache_next;

	vm_page_mappings	mappings;

#ifdef DEBUG_PAGE_QUEUE
	void*				queue;
#endif

#ifdef DEBUG_PAGE_CACHE_TRANSITIONS
	uint32	debug_flags;
	struct vm_page *collided_page;
#endif

	uint8				type : 2;
	uint8				state : 3;

	uint8				is_cleared : 1;
		// is currently only used in vm_page_allocate_page_run()

	uint16				wired_count;
	int8				usage_count;
};

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

enum {
	CACHE_TYPE_RAM = 0,
	CACHE_TYPE_VNODE,
	CACHE_TYPE_DEVICE,
	CACHE_TYPE_NULL
};

struct vm_dummy_page : vm_page {
	ConditionVariable<vm_page>	busy_condition;
};

struct vm_cache {
	mutex				lock;
	struct vm_area		*areas;
	vint32				ref_count;
	struct list_link	consumer_link;
	struct list			consumers;
		// list of caches that use this cache as a source
	vm_page				*page_list;
	struct vm_cache		*source;
	struct vm_store		*store;
	off_t				virtual_base;
	off_t				virtual_size;
		// the size is absolute, and independent from virtual_base
	uint32				page_count;
	uint32				temporary : 1;
	uint32				scan_skip : 1;
	uint32				busy : 1;
	uint32				type : 5;

#if DEBUG_CACHE_LIST
	struct vm_cache*	debug_previous;
	struct vm_cache*	debug_next;
#endif
};


#if DEBUG_CACHE_LIST
extern vm_cache* gDebugCacheList;
#endif

struct vm_area {
	char				*name;
	area_id				id;
	addr_t				base;
	addr_t				size;
	uint32				protection;
	uint16				wiring;
	uint16				memory_type;

	struct vm_cache		*cache;
	vint32				no_cache_change;
	off_t				cache_offset;
	uint32				cache_type;
	vm_area_mappings	mappings;

	struct vm_address_space *address_space;
	struct vm_area		*address_space_next;
	struct vm_area		*cache_next;
	struct vm_area		*cache_prev;
	struct vm_area		*hash_next;
};

#endif	// __cplusplus

enum {
	VM_ASPACE_STATE_NORMAL = 0,
	VM_ASPACE_STATE_DELETION
};

struct vm_address_space {
	struct vm_area		*areas;
	struct vm_area		*area_hint;
	sem_id				sem;
	addr_t				base;
	addr_t				size;
	int32				change_count;
	vm_translation_map	translation_map;
	team_id				id;
	int32				ref_count;
	int32				fault_count;
	int32				state;
	addr_t				scan_va;
	addr_t				working_set_size;
	addr_t				max_working_set;
	addr_t				min_working_set;
	bigtime_t			last_working_set_adjust;
	struct vm_address_space *hash_next;
};

struct vm_store {
	struct vm_store_ops	*ops;
	struct vm_cache		*cache;
	off_t				committed_size;
};

typedef struct vm_store_ops {
	void (*destroy)(struct vm_store *backingStore);
	status_t (*commit)(struct vm_store *backingStore, off_t size);
	bool (*has_page)(struct vm_store *backingStore, off_t offset);
	status_t (*read)(struct vm_store *backingStore, off_t offset,
		const iovec *vecs, size_t count, size_t *_numBytes, bool fsReenter);
	status_t (*write)(struct vm_store *backingStore, off_t offset,
		const iovec *vecs, size_t count, size_t *_numBytes, bool fsReenter);
	status_t (*fault)(struct vm_store *backingStore,
		struct vm_address_space *aspace, off_t offset);
	status_t (*acquire_unreferenced_ref)(struct vm_store *backingStore);
	void (*acquire_ref)(struct vm_store *backingStore);
	void (*release_ref)(struct vm_store *backingStore);
} vm_store_ops;

#endif	/* _KERNEL_VM_TYPES_H */
