/* 
** Copyright 2002, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef VFS_SELECT_H
#define VFS_SELECT_H

typedef struct select_info {
	uint8	events;
} select_info;

typedef struct select_sync {
	sem_id		sem;
	uint32		count;
	select_info *set;
} select_sync;

enum select_events {
	B_SELECT_READ = 1,
	B_SELECT_WRITE,
	B_SELECT_ERROR,

	B_SELECT_PRI_READ,
	B_SELECT_PRI_WRITE,

	B_SELECT_HIGH_PRI_READ,
	B_SELECT_HIGH_PRI_WRITE,

	B_SELECT_DISCONNECTED
};

#define SELECT_FLAG(type) (1L << (type - 1))
#define MAKE_SELECT_REF(index, type) ((SELECT_FLAG(type) << 16L) | ((index) & 0xffffUL))

#define INDEX_FROM_REF(ref) ((ref) & 0xffffUL)
#define SELECT_FLAG_FROM_REF(ref) ((ref) >> 16L)

#endif	/* VFS_SELECT_H */
