/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <pthread.h>

#include "posix_error_mapper.h"


WRAPPER_FUNCTION(int, pthread_mutex_destroy, (pthread_mutex_t *mutex),
	return B_TO_POSITIVE_ERROR(sReal_pthread_mutex_destroy(mutex));
)


WRAPPER_FUNCTION(int, pthread_mutex_getprioceiling,
		(const pthread_mutex_t *mutex, int *_priorityCeiling),
	return B_TO_POSITIVE_ERROR(sReal_pthread_mutex_getprioceiling(mutex,
		_priorityCeiling));
)


WRAPPER_FUNCTION(int, pthread_mutex_init,
		(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr),
	return B_TO_POSITIVE_ERROR(sReal_pthread_mutex_init(mutex, attr));
)


WRAPPER_FUNCTION(int, pthread_mutex_lock, (pthread_mutex_t *mutex),
	return B_TO_POSITIVE_ERROR(sReal_pthread_mutex_lock(mutex));
)


WRAPPER_FUNCTION(int, pthread_mutex_setprioceiling,
		(pthread_mutex_t *mutex, int newPriorityCeiling,
		int *_oldPriorityCeiling),
	return B_TO_POSITIVE_ERROR(sReal_pthread_mutex_setprioceiling(mutex,
		newPriorityCeiling, _oldPriorityCeiling));
)


WRAPPER_FUNCTION(int, pthread_mutex_timedlock,
		(pthread_mutex_t *mutex, const struct timespec *spec),
	return B_TO_POSITIVE_ERROR(sReal_pthread_mutex_timedlock(mutex, spec));
)


WRAPPER_FUNCTION(int, pthread_mutex_trylock, (pthread_mutex_t *mutex),
	return B_TO_POSITIVE_ERROR(sReal_pthread_mutex_trylock(mutex));
)


WRAPPER_FUNCTION(int, pthread_mutex_unlock, (pthread_mutex_t *mutex),
	return B_TO_POSITIVE_ERROR(sReal_pthread_mutex_unlock(mutex));
)
