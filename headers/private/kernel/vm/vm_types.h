/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2002-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */
#ifndef _KERNEL_VM_VM_TYPES_H
#define _KERNEL_VM_VM_TYPES_H


#include <arch/vm_types.h>
#include <arch/vm_translation_map.h>
#include <condition_variable.h>
#include <kernel.h>
#include <lock.h>
#include <util/DoublyLinkedQueue.h>
#include <util/SplayTree.h>

#include <sys/uio.h>

#include "kernel_debug_config.h"


class AsyncIOCallback;
struct vm_page_mapping;
struct VMCache;
typedef DoublyLinkedListLink<vm_page_mapping> vm_page_mapping_link;


typedef struct vm_page_mapping {
	vm_page_mapping_link page_link;
	vm_page_mapping_link area_link;
	struct vm_page *page;
	struct VMArea *area;
} vm_page_mapping;

class DoublyLinkedPageLink {
	public:
		inline vm_page_mapping_link *operator()(vm_page_mapping *element) const
		{
			return &element->page_link;
		}

		inline const vm_page_mapping_link *operator()(
			const vm_page_mapping *element) const
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

		inline const vm_page_mapping_link *operator()(
			const vm_page_mapping *element) const
		{
			return &element->area_link;
		}
};

typedef class DoublyLinkedQueue<vm_page_mapping, DoublyLinkedPageLink>
	vm_page_mappings;
typedef class DoublyLinkedQueue<vm_page_mapping, DoublyLinkedAreaLink>
	VMAreaMappings;

typedef uint32 page_num_t;

struct vm_page {
	struct vm_page*			queue_prev;
	struct vm_page*			queue_next;

	addr_t					physical_page_number;

	VMCache*				cache;
	page_num_t				cache_offset;
								// in page size units

	SplayTreeLink<vm_page>	cache_link;
	vm_page*				cache_next;

	vm_page_mappings		mappings;

#if DEBUG_PAGE_QUEUE
	void*					queue;
#endif

	uint8					type : 2;
	uint8					state : 3;

	uint8					is_cleared : 1;
		// is currently only used in vm_page_allocate_page_run()
	uint8					busy_writing : 1;
	uint8					unused : 1;
		// used in VMAnonymousCache::Merge()

	int8					usage_count;
	uint16					wired_count;
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


#endif	// _KERNEL_VM_VM_TYPES_H
