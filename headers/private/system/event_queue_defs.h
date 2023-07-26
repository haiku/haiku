/*
 * Copyright 2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYSTEM_EVENT_QUEUE_DEFS_H
#define _SYSTEM_EVENT_QUEUE_DEFS_H


// extends B_EVENT_* constants defined in OS.h
enum {
	B_EVENT_LEVEL_TRIGGERED		= (1 << 26),	/* Event is level-triggered, not edge-triggered */
	B_EVENT_ONE_SHOT			= (1 << 27),	/* Delete event after delivery */

	/* bits 28 through 30 are reserved for the kernel */
};


typedef struct event_wait_info {
	int32		object;
	uint16		type;
	int32		events;		/* select(): > 0 to select, -1 to get selection, 0 to deselect */
	void*		user_data;
} event_wait_info;


#endif	/* _SYSTEM_EVENT_QUEUE_DEFS_H */
