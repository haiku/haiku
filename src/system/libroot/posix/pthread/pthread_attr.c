/* 
** Copyright 2006, Jérôme Duval. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include <pthread.h>
#include "pthread_private.h"

#include <stdlib.h>


int 
pthread_attr_init(pthread_attr_t *_attr)
{
	pthread_attr *attr;

	if (_attr == NULL)
		return B_BAD_VALUE;

	attr = (pthread_attr *)malloc(sizeof(pthread_attr));
	if (attr == NULL)
		return B_NO_MEMORY;

	attr->detach_state = PTHREAD_CREATE_JOINABLE;
	attr->sched_priority = B_NORMAL_PRIORITY;

	*_attr = attr;
	return B_OK;
}


int 
pthread_attr_destroy(pthread_attr_t *_attr)
{
	pthread_attr *attr;

	if (_attr == NULL || (attr = *_attr) == NULL)
		return B_BAD_VALUE;

	*_attr = NULL;
	free(attr);

	return B_OK;
}


int
pthread_attr_getdetachstate(const pthread_attr_t *_attr, int *state)
{
	pthread_attr *attr;
	
	if (_attr == NULL || (attr = *_attr) == NULL || state == NULL)
		return B_BAD_VALUE;

	*state = attr->detach_state;

	return B_OK;
}


int
pthread_attr_setdetachstate(pthread_attr_t *_attr, int state)
{
	pthread_attr *attr;

	if (_attr == NULL || (attr = *_attr) == NULL)
		return B_BAD_VALUE;

	if (state != PTHREAD_CREATE_JOINABLE && state != PTHREAD_CREATE_DETACHED)
		return B_BAD_VALUE;

	attr->detach_state = state;

	return B_OK;
}

