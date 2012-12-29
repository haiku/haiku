/*
 * Copyright 2009-2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <pthread.h>

#include "posix_error_mapper.h"


WRAPPER_FUNCTION(int, pthread_attr_destroy, (pthread_attr_t *attr),
	return B_TO_POSITIVE_ERROR(sReal_pthread_attr_destroy(attr));
)


WRAPPER_FUNCTION(int, pthread_attr_init, (pthread_attr_t *attr),
	return B_TO_POSITIVE_ERROR(sReal_pthread_attr_init(attr));
)


WRAPPER_FUNCTION(int, pthread_attr_getdetachstate,
		(const pthread_attr_t *attr, int *detachstate),
	return B_TO_POSITIVE_ERROR(sReal_pthread_attr_getdetachstate(attr,
		detachstate));
)


WRAPPER_FUNCTION(int, pthread_attr_setdetachstate,
		(pthread_attr_t *attr, int detachstate),
	return B_TO_POSITIVE_ERROR(sReal_pthread_attr_setdetachstate(attr,
		detachstate));
)


WRAPPER_FUNCTION(int, pthread_attr_getstacksize,
		(const pthread_attr_t *attr, size_t *stacksize),
	return B_TO_POSITIVE_ERROR(sReal_pthread_attr_getstacksize(attr,
		stacksize));
)


WRAPPER_FUNCTION(int, pthread_attr_setstacksize,
		(pthread_attr_t *attr, size_t stacksize),
	return B_TO_POSITIVE_ERROR(sReal_pthread_attr_setstacksize(attr,
		stacksize));
)


WRAPPER_FUNCTION(int, pthread_attr_getscope,
		(const pthread_attr_t *attr, int *contentionScope),
	return B_TO_POSITIVE_ERROR(sReal_pthread_attr_getscope(attr,
		contentionScope));
)


WRAPPER_FUNCTION(int, pthread_attr_setscope,
		(pthread_attr_t *attr, int contentionScope),
	return B_TO_POSITIVE_ERROR(sReal_pthread_attr_setscope(attr,
		contentionScope));
)


WRAPPER_FUNCTION(int, pthread_attr_setschedparam,
		(pthread_attr_t *attr, const struct sched_param *param),
	return B_TO_POSITIVE_ERROR(sReal_pthread_attr_setschedparam(attr,
		param));
)


WRAPPER_FUNCTION(int, pthread_attr_getschedparam,
		(const pthread_attr_t *attr, struct sched_param *param),
	return B_TO_POSITIVE_ERROR(sReal_pthread_attr_getschedparam(attr,
		param));
)


WRAPPER_FUNCTION(int, pthread_attr_getguardsize,
		(const pthread_attr_t *attr, size_t *guardsize),
	return B_TO_POSITIVE_ERROR(sReal_pthread_attr_getguardsize(attr,
		guardsize));
)


WRAPPER_FUNCTION(int, pthread_attr_setguardsize,
		(pthread_attr_t *attr, size_t guardsize),
	return B_TO_POSITIVE_ERROR(sReal_pthread_attr_setguardsize(attr,
		guardsize));
)
