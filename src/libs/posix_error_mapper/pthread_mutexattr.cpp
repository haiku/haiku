/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <pthread.h>

#include "posix_error_mapper.h"


WRAPPER_FUNCTION(int, pthread_mutexattr_destroy,
		(pthread_mutexattr_t *mutexAttr),
	return B_TO_POSITIVE_ERROR(sReal_pthread_mutexattr_destroy(mutexAttr));
)


WRAPPER_FUNCTION(int, pthread_mutexattr_getprioceiling,
		(const pthread_mutexattr_t *mutexAttr, int *_priorityCeiling),
	return B_TO_POSITIVE_ERROR(sReal_pthread_mutexattr_getprioceiling(mutexAttr,
		_priorityCeiling));
)


WRAPPER_FUNCTION(int, pthread_mutexattr_getprotocol,
		(const pthread_mutexattr_t *mutexAttr, int *_protocol),
	return B_TO_POSITIVE_ERROR(sReal_pthread_mutexattr_getprotocol(mutexAttr,
		_protocol));
)


WRAPPER_FUNCTION(int, pthread_mutexattr_getpshared,
		(const pthread_mutexattr_t *mutexAttr, int *_processShared),
	return B_TO_POSITIVE_ERROR(sReal_pthread_mutexattr_getpshared(mutexAttr,
		_processShared));
)


WRAPPER_FUNCTION(int, pthread_mutexattr_gettype,
		(const pthread_mutexattr_t *mutexAttr, int *_type),
	return B_TO_POSITIVE_ERROR(sReal_pthread_mutexattr_gettype(mutexAttr,
		_type));
)


WRAPPER_FUNCTION(int, pthread_mutexattr_init,
		(pthread_mutexattr_t *mutexAttr),
	return B_TO_POSITIVE_ERROR(sReal_pthread_mutexattr_init(mutexAttr));
)


WRAPPER_FUNCTION(int, pthread_mutexattr_setprioceiling,
		(pthread_mutexattr_t *mutexAttr, int priorityCeiling),
	return B_TO_POSITIVE_ERROR(sReal_pthread_mutexattr_setprioceiling(mutexAttr,
		priorityCeiling));
)


WRAPPER_FUNCTION(int, pthread_mutexattr_setprotocol,
		(pthread_mutexattr_t *mutexAttr, int protocol),
	return B_TO_POSITIVE_ERROR(sReal_pthread_mutexattr_setprotocol(mutexAttr,
		protocol));
)


WRAPPER_FUNCTION(int, pthread_mutexattr_setpshared,
		(pthread_mutexattr_t *mutexAttr, int processShared),
	return B_TO_POSITIVE_ERROR(sReal_pthread_mutexattr_setpshared(mutexAttr,
		processShared));
)


WRAPPER_FUNCTION(int, pthread_mutexattr_settype,
		(pthread_mutexattr_t *mutexAttr, int type),
	return B_TO_POSITIVE_ERROR(sReal_pthread_mutexattr_settype(mutexAttr,
		type));
)
