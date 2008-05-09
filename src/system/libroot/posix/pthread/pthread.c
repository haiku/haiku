/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2006, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "pthread_private.h"

#include <signal.h>
#include <stdlib.h>

#include <TLS.h>

#include <kernel.h>
#include <syscalls.h>
#include <thread.h>


static const pthread_attr pthread_attr_default = {
	PTHREAD_CREATE_JOINABLE,
	B_NORMAL_PRIORITY,
	USER_STACK_SIZE
};


static struct pthread_thread sMainThread;
static int32 sPthreadSlot = -1;
static int sConcurrencyLevel;


struct pthread_thread *
__get_pthread(void)
{
	struct pthread_thread *thread;

	if (sPthreadSlot == -1)
		return &sMainThread;

	thread = (struct pthread_thread *)tls_get(sPthreadSlot);
	if (thread == NULL)
		return &sMainThread;

	return thread;
}


static void
pthread_destroy_thread(void *data)
{
	struct pthread_thread *thread = __get_pthread();

	// call cleanup handlers
	while (true) {
		struct __pthread_cleanup_handler *handler
			= __pthread_cleanup_pop_handler();
		if (handler == NULL)
			break;

		handler->function(handler->argument);
	}

	__pthread_key_call_destructors(thread);
	free(thread);
}


static int32
pthread_thread_entry(thread_func _unused, void *_thread)
{
	struct pthread_thread *thread = (struct pthread_thread *)_thread;

	// store thread data in TLS
	*tls_address(sPthreadSlot) = thread;

	on_exit_thread(pthread_destroy_thread, NULL);

	pthread_exit(thread->entry(thread->entry_argument));
	return B_OK;
}


//	#pragma mark - public API


int
pthread_create(pthread_t *_thread, const pthread_attr_t *_attr,
		void *(*startRoutine)(void*), void *arg)
{
	const pthread_attr *attr = NULL;
	struct pthread_thread *thread;
	thread_id threadID;
	struct thread_creation_attributes attributes;

	if (_thread == NULL)
		return B_BAD_VALUE;

	if (_attr == NULL)
		attr = &pthread_attr_default;
	else
		attr = *_attr;

	thread = (struct pthread_thread *)malloc(sizeof(struct pthread_thread));
	if (thread == NULL)
		return B_WOULD_BLOCK;

	thread->entry = startRoutine;
	thread->entry_argument = arg;
	thread->cancel_state = PTHREAD_CANCEL_ENABLE;
	thread->cancel_type = PTHREAD_CANCEL_DEFERRED;
	thread->cancelled = false;
	thread->cleanup_handlers = NULL;

	if (sPthreadSlot == -1) {
		// In a clean pthread environment, this is even thread-safe!
		sPthreadSlot = tls_allocate();
	}

	attributes.entry = pthread_thread_entry;
	attributes.name = "pthread func";
	attributes.priority = attr->sched_priority;
	attributes.args1 = NULL;
	attributes.args2 = thread;
	attributes.stack_address = NULL;
	attributes.stack_size = attr->stack_size;

	threadID = _kern_spawn_thread(&attributes);
	if (threadID < B_OK) {
		// stupid error code (EAGAIN) but demanded by POSIX
		return B_WOULD_BLOCK;
	}

	resume_thread(threadID);
	*_thread = threadID;

	return B_OK;
}


pthread_t
pthread_self(void)
{
	return find_thread(NULL);
}


int
pthread_equal(pthread_t t1, pthread_t t2)
{
	return t1 > 0 && t2 > 0 && t1 == t2;
}

int
pthread_join(pthread_t thread, void **value_ptr)
{
	status_t error = wait_for_thread(thread, (status_t *)value_ptr);
	if (error == B_BAD_THREAD_ID)
		return ESRCH;
	return error;
}


void
pthread_exit(void *value)
{
	exit_thread((status_t)value);
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
	// TODO: currently, all threads are detached in our implementation...
	return 0;
}


int
pthread_getconcurrency(void)
{
	return sConcurrencyLevel;
}


int
pthread_setconcurrency(int newLevel)
{
	if (newLevel < 0)
		return EINVAL;

	sConcurrencyLevel = newLevel;
	return 0;
}
