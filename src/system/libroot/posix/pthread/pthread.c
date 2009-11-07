/*
 * Copyright 2008-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2006, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "pthread_private.h"

#include <signal.h>
#include <stdlib.h>

#include <TLS.h>

#include <syscalls.h>
#include <thread_defs.h>


#define THREAD_DETACHED	0x01
#define THREAD_DEAD		0x02


static const pthread_attr pthread_attr_default = {
	PTHREAD_CREATE_JOINABLE,
	B_NORMAL_PRIORITY,
	USER_STACK_SIZE
};


static pthread_thread sMainThread;
static int32 sPthreadSlot = -1;
static int sConcurrencyLevel;


static void
pthread_destroy_thread(void* data)
{
	pthread_thread* thread = pthread_self();

	// call cleanup handlers
	while (true) {
		struct __pthread_cleanup_handler* handler
			= __pthread_cleanup_pop_handler();
		if (handler == NULL)
			break;

		handler->function(handler->argument);
	}

	__pthread_key_call_destructors(thread);

	if ((atomic_or(&thread->flags, THREAD_DEAD) & THREAD_DETACHED) != 0)
		free(thread);
}


static int32
pthread_thread_entry(thread_func _unused, void* _thread)
{
	pthread_thread* thread = (pthread_thread*)_thread;

	// store thread data in TLS
	*tls_address(sPthreadSlot) = thread;

	on_exit_thread(pthread_destroy_thread, NULL);

	pthread_exit(thread->entry(thread->entry_argument));
	return 0;
}


// #pragma mark - public API


int
pthread_create(pthread_t* _thread, const pthread_attr_t* _attr,
	void* (*startRoutine)(void*), void* arg)
{
	const pthread_attr* attr = NULL;
	pthread_thread* thread;
	struct thread_creation_attributes attributes;

	if (_thread == NULL)
		return EINVAL;

	if (_attr == NULL)
		attr = &pthread_attr_default;
	else {
		attr = *_attr;
		if (attr == NULL)
			return EINVAL;
	}

	thread = (pthread_thread*)malloc(sizeof(pthread_thread));
	if (thread == NULL)
		return EAGAIN;

	thread->entry = startRoutine;
	thread->entry_argument = arg;
	thread->exit_value = NULL;
	thread->cancel_state = PTHREAD_CANCEL_ENABLE;
	thread->cancel_type = PTHREAD_CANCEL_DEFERRED;
	thread->cancelled = false;
	thread->cleanup_handlers = NULL;
	thread->flags = 0;

	if (attr->detach_state == PTHREAD_CREATE_DETACHED)
		thread->flags |= THREAD_DETACHED;

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

	thread->id = _kern_spawn_thread(&attributes);
	if (thread->id < 0) {
		// stupid error code but demanded by POSIX
		free(thread);
		return EAGAIN;
	}

	resume_thread(thread->id);
	*_thread = thread;

	return 0;
}


pthread_t
pthread_self(void)
{
	pthread_thread* thread;

	if (sPthreadSlot == -1)
		return &sMainThread;

	thread = (pthread_thread*)tls_get(sPthreadSlot);
	if (thread == NULL)
		return &sMainThread;

	return thread;
}


int
pthread_equal(pthread_t t1, pthread_t t2)
{
	return t1 != NULL && t2 != NULL && t1 == t2;
}


int
pthread_join(pthread_t thread, void** _value)
{
	status_t dummy;
	status_t error = wait_for_thread(thread->id, &dummy);
	if (error == B_BAD_THREAD_ID)
		return ESRCH;

	if (_value != NULL)
		*_value = thread->exit_value;

	if ((atomic_or(&thread->flags, THREAD_DETACHED) & THREAD_DEAD) != 0)
		free(thread);

	return B_TO_POSIX_ERROR(error);
}


void
pthread_exit(void* value)
{
	pthread_self()->exit_value = value;
	exit_thread(B_OK);
}


int
pthread_kill(pthread_t thread, int sig)
{
	status_t status = send_signal(thread->id, (uint)sig);
	if (status != B_OK) {
		if (status == B_BAD_THREAD_ID)
			return ESRCH;

		return B_TO_POSIX_ERROR(status);
	}

	return 0;
}


int
pthread_detach(pthread_t thread)
{
	int32 flags;

	if (thread == NULL)
		return EINVAL;

	flags = atomic_or(&thread->flags, THREAD_DETACHED);
	if ((flags & THREAD_DETACHED) != 0)
		return 0;

	if ((flags & THREAD_DEAD) != 0)
		free(thread);

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


// #pragma mark - Haiku thread API bridge


thread_id
get_pthread_thread_id(pthread_t thread)
{
	return thread->id;
}
