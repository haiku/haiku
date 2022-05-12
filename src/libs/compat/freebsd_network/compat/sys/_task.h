/*
 * Copyright 2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS__TASK_H_
#define _FBSD_COMPAT_SYS__TASK_H_


/* Haiku's list management */
#include <util/list.h>


typedef void (*task_fn_t)(void *context, int pending);

struct task {
	int ta_pending;
	int ta_priority;
	int ta_flags;
	task_fn_t ta_handler;
	void *ta_argument;

	struct list_link ta_link;
};


#define TASK_NEEDSGIANT (1 << 0) /* Haiku extension, OpenBSD compatibility */


#endif
