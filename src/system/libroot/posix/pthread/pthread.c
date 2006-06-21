/*
 * Copyright 2006, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include "pthread_private.h"

static const pthread_attr pthread_attr_default = {
	PTHREAD_CREATE_JOINABLE,
	B_NORMAL_PRIORITY
};

struct pthread_data {
	thread_entry entry;
	void *data;
};

static int32
_pthread_thread_entry(void *entry_data)
{
	struct pthread_data *pdata = (struct pthread_data *)entry_data;
	thread_func entry = pdata->entry;
	void *data = pdata->data;

	free(pdata);
	on_exit_thread(_pthread_key_call_destructors, NULL);
	return entry(data);
}


int 
pthread_create(pthread_t *_thread, const pthread_attr_t *_attr,
		void *(*start_routine)(void*), void *arg)
{
	thread_id thread;
	const pthread_attr *attr = NULL;
	struct pthread_data *pdata;

	if (_thread == NULL)
		return B_BAD_VALUE;

	if (_attr == NULL)
		attr = &pthread_attr_default;
	else
		attr = *_attr;
	
	pdata = malloc(sizeof(struct pthread_data));
	if (!pdata)
		return B_WOULD_BLOCK;
	pdata->entry = (thread_entry)start_routine;
	pdata->data = arg;

	thread = spawn_thread(_pthread_thread_entry, "pthread func", attr->sched_priority, pdata);
	if (thread < B_OK)
		return B_WOULD_BLOCK;
		// stupid error code (EAGAIN) but demanded by POSIX

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
	wait_for_thread(thread, (status_t *)value_ptr);
	/* the thread could be joinable and gone */
	return B_OK;
}


void 
pthread_exit(void *value_ptr)
{
	exit_thread((status_t)value_ptr);
}


int 
pthread_kill(pthread_t thread, int sig)
{
	status_t err =  kill(thread, sig);
	if (err == B_BAD_THREAD_ID)
		return ESRCH;
	return err;
}


int 
pthread_detach(pthread_t thread)
{
	return B_NOT_ALLOWED;
}

