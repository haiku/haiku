/* 
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/
#ifndef VFS_SELECT_H
#define VFS_SELECT_H


#include <Select.h>


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

#endif	/* VFS_SELECT_H */
