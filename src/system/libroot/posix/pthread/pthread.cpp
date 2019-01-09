/*
 * Copyright 2008-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2006, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "pthread_private.h"

#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <TLS.h>

#include <syscall_utils.h>

#include <libroot_private.h>
#include <syscalls.h>
#include <thread_defs.h>
#include <tls.h>

#include <user_thread.h>


static pthread_attr pthread_attr_default = {
	PTHREAD_CREATE_JOINABLE,
	B_NORMAL_PRIORITY,
	USER_STACK_SIZE,
	USER_STACK_GUARD_SIZE
};


static pthread_thread sMainThread;
static int sConcurrencyLevel;


static status_t
pthread_thread_entry(void*, void* _thread)
{
	pthread_thread* thread = (pthread_thread*)_thread;

	__heap_thread_init();

	pthread_exit(thread->entry(thread->entry_argument));
	return 0;
}


// #pragma mark - private API


void
__pthread_destroy_thread(void)
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


pthread_thread*
__allocate_pthread(void* (*entry)(void*), void *data)
{
	pthread_thread* thread = (pthread_thread*)malloc(sizeof(pthread_thread));
	if (thread == NULL)
		return NULL;

	__init_pthread(thread, entry, data);

	return thread;
}


void
__init_pthread(pthread_thread* thread, void* (*entry)(void*), void* data)
{
	thread->entry = entry;
	thread->entry_argument = data;
	thread->exit_value = NULL;
	thread->cleanup_handlers = NULL;
	thread->flags = THREAD_CANCEL_ENABLED;
		// thread cancellation enabled, but deferred

	memset(thread->specific, 0, sizeof(thread->specific));
}


status_t
__pthread_init_creation_attributes(const pthread_attr_t* pthreadAttributes,
	pthread_t thread, status_t (*entryFunction)(void*, void*),
	void* argument1, void* argument2, const char* name,
	thread_creation_attributes* attributes)
{
	const pthread_attr* attr = NULL;
	if (pthreadAttributes == NULL) {
		attr = &pthread_attr_default;
	} else {
		attr = *pthreadAttributes;
		if (attr == NULL)
			return EINVAL;
	}

	attributes->entry = entryFunction;
	attributes->name = name;
	attributes->priority = attr->sched_priority;
	attributes->args1 = argument1;
	attributes->args2 = argument2;
	attributes->stack_address = NULL;
	attributes->stack_size = attr->stack_size;
	attributes->guard_size = attr->guard_size;
	attributes->pthread = thread;
	attributes->flags = 0;

	if (thread != NULL && attr->detach_state == PTHREAD_CREATE_DETACHED)
		thread->flags |= THREAD_DETACHED;

	return B_OK;
}


void
__pthread_set_default_priority(int32 priority)
{
	pthread_attr_default.sched_priority = priority;
}


// #pragma mark - public API


int
pthread_create(pthread_t* _thread, const pthread_attr_t* attr,
	void* (*startRoutine)(void*), void* arg)
{
	if (_thread == NULL)
		return EINVAL;

	pthread_thread* thread = __allocate_pthread(startRoutine, arg);
	if (thread == NULL)
		return EAGAIN;

	thread_creation_attributes attributes;
	status_t error = __pthread_init_creation_attributes(attr, thread,
		&pthread_thread_entry, NULL, thread, "pthread func", &attributes);
	if (error != B_OK) {
		free(thread);
		return error;
	}

	thread->id = _kern_spawn_thread(&attributes);
	if (thread->id < 0) {
		// stupid error code but demanded by POSIX
		free(thread);
		return EAGAIN;
	}

	__set_stack_protection();
	*_thread = thread;
	resume_thread(thread->id);

	return 0;
}


pthread_t
pthread_self(void)
{
	pthread_thread* thread = get_user_thread()->pthread;
	if (thread == NULL)
		return &sMainThread;

	return thread;
}


int
pthread_equal(pthread_t t1, pthread_t t2)
{
	return t1 == t2;
}


int
pthread_join(pthread_t thread, void** _value)
{
	status_t dummy;
	status_t error;
	do {
		error = wait_for_thread(thread->id, &dummy);
	} while (error == B_INTERRUPTED);

	if (error == B_BAD_THREAD_ID)
		RETURN_AND_TEST_CANCEL(ESRCH);

	if (_value != NULL)
		*_value = thread->exit_value;

	if ((atomic_or(&thread->flags, THREAD_DETACHED) & THREAD_DEAD) != 0)
		free(thread);

	RETURN_AND_TEST_CANCEL(error);
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

		return status;
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


int
pthread_getschedparam(pthread_t thread, int *policy, struct sched_param *param)
{
	thread_info info;
	status_t status = _kern_get_thread_info(thread->id, &info);
	if (status == B_BAD_THREAD_ID)
		return ESRCH;
	param->sched_priority = info.priority;
	if (policy != NULL)
		*policy = SCHED_RR;
	return 0;
}


int
pthread_setschedparam(pthread_t thread, int policy,
	const struct sched_param *param)
{
	status_t status;
	if (policy != SCHED_RR)
		return ENOTSUP;
	status = _kern_set_thread_priority(thread->id, param->sched_priority);
	if (status == B_BAD_THREAD_ID)
		return ESRCH;
	if (status < B_OK)
		return status;
	return 0;
}


// #pragma mark - Haiku thread API bridge


thread_id
get_pthread_thread_id(pthread_t thread)
{
	return thread->id;
}
