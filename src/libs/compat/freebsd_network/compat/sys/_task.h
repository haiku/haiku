#ifndef _FBSD_COMPAT_SYS__TASK_H_
#define _FBSD_COMPAT_SYS__TASK_H_

/* Haiku's list management */
#include <util/list.h>

typedef void (*task_handler_t)(void *context, int pending);

struct task {
	int ta_priority;
	task_handler_t ta_handler;
	void *ta_argument;
	int ta_pending;

	struct list_link ta_link;
};

#endif
