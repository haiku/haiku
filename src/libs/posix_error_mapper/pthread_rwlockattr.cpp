/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <pthread.h>

#include "posix_error_mapper.h"


WRAPPER_FUNCTION(int, pthread_rwlockattr_init,
		(pthread_rwlockattr_t *attr),
	return B_TO_POSITIVE_ERROR(sReal_pthread_rwlockattr_init(attr));
)


WRAPPER_FUNCTION(int, pthread_rwlockattr_destroy,
		(pthread_rwlockattr_t *attr),
	return B_TO_POSITIVE_ERROR(sReal_pthread_rwlockattr_destroy(attr));
)


WRAPPER_FUNCTION(int, pthread_rwlockattr_getpshared,
		(const pthread_rwlockattr_t *attr, int *shared),
	return B_TO_POSITIVE_ERROR(sReal_pthread_rwlockattr_getpshared(attr,
		shared));
)


WRAPPER_FUNCTION(int, pthread_rwlockattr_setpshared,
		(pthread_rwlockattr_t *attr, int shared),
	return B_TO_POSITIVE_ERROR(sReal_pthread_rwlockattr_setpshared(attr,
		shared));
)
