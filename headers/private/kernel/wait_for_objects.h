/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _KERNEL_WAIT_FOR_OBJECTS_H
#define _KERNEL_WAIT_FOR_OBJECTS_H

#include <OS.h>

#include <lock.h>


struct select_sync;


typedef struct select_info {
	struct select_info*			next;
	struct select_sync*			sync;
	int32						events;
	uint16						selected_events;
} select_info;


#define SELECT_FLAG(type) (1L << (type - 1))

#define SELECT_OUTPUT_ONLY_FLAGS \
	(B_EVENT_ERROR | B_EVENT_DISCONNECTED | B_EVENT_INVALID)

#define SELECT_TYPE_IS_OUTPUT_ONLY(type) \
	((SELECT_FLAG(type) & SELECT_OUTPUT_ONLY_FLAGS) != 0)


#ifdef __cplusplus
extern "C" {
#endif


extern void		acquire_select_sync(select_sync* sync);
extern void		put_select_sync(select_sync* sync);
extern status_t	notify_select_events(select_info* info, uint16 events);
extern void		notify_select_events_list(select_info* list, uint16 events);

extern ssize_t	_user_wait_for_objects(object_wait_info* userInfos,
					int numInfos, uint32 flags, bigtime_t timeout);


#ifdef __cplusplus
}
#endif 

#endif	// _KERNEL_WAIT_FOR_OBJECTS_H
