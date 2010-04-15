/*
 * Copyright (c) 2002, Intel Corporation. All rights reserved.
 * Created by:  rolla.n.selbak REMOVE-THIS AT intel DOT com
 * This file is licensed under the GPL license.  For the full content
 * of this license, see the COPYING file at the top level of this
 * source tree.

 * Test that pthread_detach()
 *
 * If 'thread' has not terminated, pthread_detach() shall not cause it to
 * terminate.  The effect of multiple pthread_detach() calls on the same
 *
 * STEPS:
 * 1.Create a joinable thread
 * 2.Detach that thread
 * 3.Verify that the thread did not terminate because of this
 *
 */

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "posixtest.h"


void *a_thread_func()
{

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	/* If the thread wasn't canceled in 10 seconds, time out */
	sleep(10);

	printf("Thread couldn't be canceled (at cleanup time), timing out\n");
	pthread_exit(0);
	return NULL;
}

int main()
{
	pthread_attr_t new_attr;
	pthread_t new_th;
	int ret;

	/* Initialize attribute */
	if(pthread_attr_init(&new_attr) != 0)
	{
		printf("Cannot initialize attribute object\n");
		return PTS_UNRESOLVED;
	}

	/* Set the attribute object to be joinable */
	if(pthread_attr_setdetachstate(&new_attr, PTHREAD_CREATE_JOINABLE) != 0)
	{
		printf("Error in pthread_attr_setdetachstate()\n");
		return PTS_UNRESOLVED;
	}

	/* Create the thread */
	if(pthread_create(&new_th, &new_attr, a_thread_func, NULL) != 0)
	{
		printf("Error creating thread\n");
		return PTS_UNRESOLVED;
	}

	/* Detach the thread. */
	if(pthread_detach(new_th) != 0)
	{
		printf("Error detaching thread\n");
		return PTS_FAIL;
	}

	/* Verify that it hasn't terminated the thread */
	ret=pthread_cancel(new_th);

	if(ret != 0)
	{
		if(ret==ESRCH)
		{
			printf("Test FAILED\n");
			return PTS_FAIL;
		}
		printf("Error canceling thread: %s\n", strerror(ret));
		return PTS_UNRESOLVED;
	}

	printf("Test PASSED\n");
	return PTS_PASS;
}


