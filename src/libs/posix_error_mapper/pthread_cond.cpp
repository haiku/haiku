/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <pthread.h>

#include "posix_error_mapper.h"


WRAPPER_FUNCTION(int, pthread_cond_destroy, (pthread_cond_t *cond),
	return B_TO_POSITIVE_ERROR(sReal_pthread_cond_destroy(cond));
)


WRAPPER_FUNCTION(int, pthread_cond_init, (pthread_cond_t *cond,
		const pthread_condattr_t *attr),
	return B_TO_POSITIVE_ERROR(sReal_pthread_cond_init(cond, attr));
)


WRAPPER_FUNCTION(int, pthread_cond_broadcast, (pthread_cond_t *cond),
	return B_TO_POSITIVE_ERROR(sReal_pthread_cond_broadcast(cond));
)


WRAPPER_FUNCTION(int, pthread_cond_signal, (pthread_cond_t *cond),
	return B_TO_POSITIVE_ERROR(sReal_pthread_cond_signal(cond));
)


WRAPPER_FUNCTION(int, pthread_cond_timedwait, (pthread_cond_t *cond,
		pthread_mutex_t *mutex, const struct timespec *abstime),
	return B_TO_POSITIVE_ERROR(sReal_pthread_cond_timedwait(cond, mutex,
		abstime));
)


WRAPPER_FUNCTION(int, pthread_cond_wait, (pthread_cond_t *cond,
		pthread_mutex_t *mutex),
	return B_TO_POSITIVE_ERROR(sReal_pthread_cond_wait(cond, mutex));
)

