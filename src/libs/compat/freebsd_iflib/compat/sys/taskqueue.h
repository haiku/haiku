/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_IFLIB_SYS_TASKQUEUE_H_
#define _FBSD_IFLIB_SYS_TASKQUEUE_H_

/* include the real sys/taskqueue.h */
#include_next <sys/taskqueue.h>

typedef void (*taskqueue_callback_fn)(void *context);


enum taskqueue_callback_type {
	TASKQUEUE_CALLBACK_TYPE_INIT,
	TASKQUEUE_CALLBACK_TYPE_SHUTDOWN,
};
#define	TASKQUEUE_CALLBACK_TYPE_MIN	TASKQUEUE_CALLBACK_TYPE_INIT
#define	TASKQUEUE_CALLBACK_TYPE_MAX	TASKQUEUE_CALLBACK_TYPE_SHUTDOWN
#define	TASKQUEUE_NUM_CALLBACKS		TASKQUEUE_CALLBACK_TYPE_MAX + 1


#endif /* _FBSD_IFLIB_SYS_TASKQUEUE_H_ */
