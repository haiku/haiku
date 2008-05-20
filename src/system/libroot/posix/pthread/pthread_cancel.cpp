/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "pthread_private.h"

#include <stdio.h>


int
pthread_cancel(pthread_t thread)
{
	// TODO: notify thread of being cancelled.
	fprintf(stderr, "pthread_cancel() is not yet implemented!\n");
	return EINVAL;
}


int
pthread_setcancelstate(int state, int *_oldState)
{
	if (state != PTHREAD_CANCEL_ENABLE && state != PTHREAD_CANCEL_DISABLE)
		return EINVAL;

	pthread_thread* thread = pthread_self();
	if (thread == NULL)
		return EINVAL;

	if (_oldState != NULL)
		*_oldState = thread->cancel_state;

	thread->cancel_state = state;
	return 0;
}


int
pthread_setcanceltype(int type, int *_oldType)
{
	if (type != PTHREAD_CANCEL_DEFERRED && type != PTHREAD_CANCEL_ASYNCHRONOUS)
		return EINVAL;

	pthread_thread* thread = pthread_self();
	if (thread == NULL)
		return EINVAL;

	if (_oldType != NULL)
		*_oldType = thread->cancel_type;

	thread->cancel_type = type;
	return 0;
}


void
pthread_testcancel(void)
{
	pthread_thread* thread = pthread_self();
	if (thread == NULL)
		return;

	if (thread->cancelled && thread->cancel_state == PTHREAD_CANCEL_ENABLE)
		pthread_exit(NULL);
}

