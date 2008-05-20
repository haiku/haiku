/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "pthread_private.h"


void
__pthread_cleanup_push_handler(__pthread_cleanup_handler* handler)
{
	pthread_thread* thread = pthread_self();
	if (thread == NULL)
		return;

	handler->previous = thread->cleanup_handlers;
	thread->cleanup_handlers = handler;
}


__pthread_cleanup_handler*
__pthread_cleanup_pop_handler(void)
{
	pthread_thread* thread = pthread_self();
	if (thread == NULL)
		return NULL;

	__pthread_cleanup_handler* handler = thread->cleanup_handlers;
	if (handler == NULL)
		return NULL;

	thread->cleanup_handlers = handler->previous;
	return handler;
}

