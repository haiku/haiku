/*
 * Copyright 2015, Hamish Morrison, hamishm53@gmail.com.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef _KERNEL_EVENT_QUEUE_H
#define _KERNEL_EVENT_QUEUE_H

#include <OS.h>
#include <event_queue_defs.h>


#ifdef __cplusplus
extern "C" {
#endif


extern int		_user_event_queue_create(int openFlags);
extern status_t	_user_event_queue_select(int queue,	event_wait_info* userInfos,
					int numInfos);
extern ssize_t	_user_event_queue_wait(int queue, event_wait_info* infos,
					int numInfos, uint32 flags, bigtime_t timeout);


#ifdef __cplusplus
}
#endif

#endif
