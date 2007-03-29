/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef VFS_SELECT_H
#define VFS_SELECT_H


#include <Select.h>
#include <util/DoublyLinkedList.h>

#include <OS.h>


typedef struct select_info {
	uint16	selected_events;
	uint16	events;
} select_info;

typedef struct select_sync {
	sem_id		sem;
	uint32		count;
	select_info *set;
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

#endif	/* VFS_SELECT_H */
