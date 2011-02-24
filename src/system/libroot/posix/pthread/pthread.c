/*
 * Copyright 2008-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2006, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <libroot_private.h>
#include <pthread_private.h>

#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <TLS.h>

#include <syscalls.h>
#include <thread_defs.h>
#include <tls.h>


#define THREAD_DETACHED	0x01
#define THREAD_DEAD		0x02


static const pthread_attr pthread_attr_default = {
	PTHREAD_CREATE_JOINABLE,
	B_NORMAL_PRIORITY,
	USER_STACK_SIZE
};


static pthread_thread sMainThread;
static int sConcurrencyLevel;


static status_t
pthread_thread_entry(thread_func _unused, void* _thread)
{
	pthread_thread* thread = (pthread_thread*)_thread;

	// store thread data in TLS
	*tls_address(TLS_PTHREAD_SLOT) = thread;

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
__allocate_pthread(void *data)
{
	pthread_thread* thread = (pthread_thread*)malloc(sizeof(pthread_thread));
	if (thread == NULL)
		return NULL;

	thread->entry = NULL;
	thread->entry_argument = data;
	thread->exit_value = NULL;
	thread->cancel_state = PTHREAD_CANCEL_ENABLE;
	thread->cancel_type = PTHREAD_CANCEL_DEFERRED;
	thread->cancelled = false;
	thread->cleanup_handlers = NULL;
	thread->flags = 0;

	memset(thread->specific, 0, sizeof(thread->specific));

	return thread;
}


void
__init_pthread(void)
{
	sMainThread.id = find_thread(NULL);
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

	thread = __allocate_pthread(arg);
	if (thread == NULL)
		return EAGAIN;

	thread->entry = startRoutine;

	if (attr->detach_state == PTHREAD_CREATE_DETACHED)
		thread->flags |= THREAD_DETACHED;

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

	thread = (pthread_thread*)tls_get(TLS_PTHREAD_SLOT);
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
	return status;
}


// #pragma mark - Haiku thread API bridge


thread_id
get_pthread_thread_id(pthread_t thread)
{
	return thread->id;
}
