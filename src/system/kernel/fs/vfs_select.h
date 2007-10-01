/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef VFS_SELECT_H
#define VFS_SELECT_H


#include <OS.h>

#include <Select.h>
#include <util/DoublyLinkedList.h>

#include <lock.h>


struct select_sync;

typedef struct select_info {
	select_info*	next;				// next in the IO context's list
	select_sync*	sync;
	uint16			selected_events;
	uint16			events;
} select_info;

typedef struct select_sync {
	vint32			ref_count;
	benaphore		lock;
	sem_id			sem;
	uint32			count;
	select_info*	set;
} select_sync;

#define SELECT_FLAG(type) (1L << (type - 1))

struct select_sync_pool_entry
	: DoublyLinkedListLinkImpl<select_sync_pool_entry> {
	selectsync			*sync;
	uint32				ref;
	uint16				events;
};

typedef DoublyLinkedList<select_sync_pool_entry> SelectSyncPoolEntryList;

struct select_sync_pool {
	SelectSyncPoolEntryList	entries;
};

void put_select_sync(select_sync* sync);

status_t notify_select_events(select_info* info, uint16 events);

#endif	/* VFS_SELECT_H */
