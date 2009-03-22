/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <pthread.h>

#include "posix_error_mapper.h"


WRAPPER_FUNCTION(int, pthread_rwlock_init,
		(pthread_rwlock_t *lock, const pthread_rwlockattr_t *attr),
	return B_TO_POSITIVE_ERROR(sReal_pthread_rwlock_init(lock, attr));
)


WRAPPER_FUNCTION(int, pthread_rwlock_destroy, (pthread_rwlock_t *lock),
	return B_TO_POSITIVE_ERROR(sReal_pthread_rwlock_destroy(lock));
)


WRAPPER_FUNCTION(int, pthread_rwlock_rdlock, (pthread_rwlock_t *lock),
	return B_TO_POSITIVE_ERROR(sReal_pthread_rwlock_rdlock(lock));
)


WRAPPER_FUNCTION(int, pthread_rwlock_tryrdlock, (pthread_rwlock_t *lock),
	return B_TO_POSITIVE_ERROR(sReal_pthread_rwlock_tryrdlock(lock));
)


WRAPPER_FUNCTION(int, pthread_rwlock_timedrdlock,
		(pthread_rwlock_t *lock, const struct timespec *timeout),
	return B_TO_POSITIVE_ERROR(sReal_pthread_rwlock_timedrdlock(lock, timeout));
)


WRAPPER_FUNCTION(int, pthread_rwlock_wrlock, (pthread_rwlock_t *lock),
	return B_TO_POSITIVE_ERROR(sReal_pthread_rwlock_wrlock(lock));
)


WRAPPER_FUNCTION(int, pthread_rwlock_trywrlock, (pthread_rwlock_t *lock),
	return B_TO_POSITIVE_ERROR(sReal_pthread_rwlock_trywrlock(lock));
)


WRAPPER_FUNCTION(int, pthread_rwlock_timedwrlock,
		(pthread_rwlock_t *lock, const struct timespec *timeout),
	return B_TO_POSITIVE_ERROR(sReal_pthread_rwlock_timedwrlock(lock, timeout));
)


WRAPPER_FUNCTION(int, pthread_rwlock_unlock, (pthread_rwlock_t *lock),
	return B_TO_POSITIVE_ERROR(sReal_pthread_rwlock_unlock(lock));
)
