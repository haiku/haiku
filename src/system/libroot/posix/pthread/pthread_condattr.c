/*
 * Copyright 2007, Ryan Leavengood, leavengood@gmail.com.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <pthread.h>
#include "pthread_private.h"

#include <stdlib.h>


int 
pthread_condattr_init(pthread_condattr_t *_condAttr)
{
	pthread_condattr *attr;

	if (_condAttr == NULL)
		return B_BAD_VALUE;

	attr = (pthread_condattr *)malloc(sizeof(pthread_condattr));
	if (attr == NULL)
		return B_NO_MEMORY;

	attr->process_shared = false;

	*_condAttr = attr;
	return B_OK;
}


int 
pthread_condattr_destroy(pthread_condattr_t *_condAttr)
{
	pthread_condattr *attr;

	if (_condAttr == NULL || (attr = *_condAttr) == NULL)
		return B_BAD_VALUE;

	*_condAttr = NULL;
	free(attr);

	return B_OK;
}


int 
pthread_condattr_getpshared(const pthread_condattr_t *_condAttr, int *_processShared)
{
	pthread_condattr *attr;

	if (_condAttr == NULL || (attr = *_condAttr) == NULL || _processShared == NULL)
		return B_BAD_VALUE;

	*_processShared = attr->process_shared ? PTHREAD_PROCESS_SHARED : PTHREAD_PROCESS_PRIVATE;
	return B_OK;
}


int 
pthread_condattr_setpshared(pthread_condattr_t *_condAttr, int processShared)
{
	pthread_condattr *attr;

	if (_condAttr == NULL || (attr = *_condAttr) == NULL
		|| processShared < PTHREAD_PROCESS_PRIVATE
		|| processShared > PTHREAD_PROCESS_SHARED)
		return B_BAD_VALUE;

	attr->process_shared = processShared == PTHREAD_PROCESS_SHARED ? true : false;
	return B_OK;
}

