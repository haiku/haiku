/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <pthread.h>

#include "posix_error_mapper.h"


WRAPPER_FUNCTION(int, pthread_condattr_destroy,
		(pthread_condattr_t *condAttr),
	return B_TO_POSITIVE_ERROR(sReal_pthread_condattr_destroy(condAttr));
)


WRAPPER_FUNCTION(int, pthread_condattr_init,
		(pthread_condattr_t *condAttr),
 	return B_TO_POSITIVE_ERROR(sReal_pthread_condattr_init(condAttr));
)


WRAPPER_FUNCTION(int, pthread_condattr_getpshared,
		(const pthread_condattr_t *condAttr, int *processShared),
	return B_TO_POSITIVE_ERROR(sReal_pthread_condattr_getpshared(condAttr,
		processShared));
)


WRAPPER_FUNCTION(int, pthread_condattr_setpshared,
		(pthread_condattr_t *condAttr, int processShared),
	return B_TO_POSITIVE_ERROR(sReal_pthread_condattr_setpshared(condAttr,
		processShared));
)
