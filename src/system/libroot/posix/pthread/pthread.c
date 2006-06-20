/*
 * Copyright 2006, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <pthread.h>
#include <signal.h>
#include "pthread_private.h"

static const pthread_attr pthread_attr_default = {
	PTHREAD_CREATE_JOINABLE,
	B_NORMAL_PRIORITY
};


int 
pthread_create(pthread_t *_thread, const pthread_attr_t *_attr,
		void *(*start_routine)(void*), void *arg)
{
	thread_id thread;
	const pthread_attr *attr = NULL;

	if (_thread == NULL)
		return B_BAD_VALUE;

	if (_attr == NULL)
		attr = &pthread_attr_default;
	else
		attr = *_attr;
	
	thread = spawn_thread((thread_entry)start_routine, "pthread func", attr->sched_priority, arg);
	if (thread < B_OK)
		return thread;

	resume_thread(thread);

	*_thread = thread;

	return B_OK;
}


pthread_t
pthread_self()
{
	return find_thread(NULL);
}


int 
pthread_equal(pthread_t t1, pthread_t t2)
{
	return (t1 > 0 && t2 > 0 && t1 == t2);
}

int 
pthread_join(pthread_t thread, void **value_ptr)
{
	return wait_for_thread(thread, (status_t *)value_ptr);
}


void 
pthread_exit(void *value_ptr)
{
	exit_thread((status_t)value_ptr);
}


int 
pthread_kill(pthread_t thread, int sig)
{
	return kill(thread, sig);
}

