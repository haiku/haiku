/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <pthread.h>
#include <signal.h>

#include "posix_error_mapper.h"


WRAPPER_FUNCTION(int, pthread_create,
		(pthread_t *thread, const pthread_attr_t *attr,
			void *(*start_routine)(void*), void *arg),
	return B_TO_POSITIVE_ERROR(sReal_pthread_create(thread, attr, start_routine,
		arg));
)


WRAPPER_FUNCTION(int, pthread_detach, (pthread_t thread),
	return B_TO_POSITIVE_ERROR(sReal_pthread_detach(thread));
)


WRAPPER_FUNCTION(int, pthread_join, (pthread_t thread, void **_value),
	return B_TO_POSITIVE_ERROR(sReal_pthread_join(thread, _value));
)


WRAPPER_FUNCTION(int, pthread_kill, (pthread_t thread, int sig),
	return B_TO_POSITIVE_ERROR(sReal_pthread_kill(thread, sig));
)


WRAPPER_FUNCTION(int, pthread_setconcurrency, (int newLevel),
	return B_TO_POSITIVE_ERROR(sReal_pthread_setconcurrency(newLevel));
)


WRAPPER_FUNCTION(int, pthread_cancel, (pthread_t thread),
	return B_TO_POSITIVE_ERROR(sReal_pthread_cancel(thread));
)


WRAPPER_FUNCTION(int, pthread_setcancelstate,
		(int state, int *_oldState),
	return B_TO_POSITIVE_ERROR(sReal_pthread_setcancelstate(state, _oldState));
)


WRAPPER_FUNCTION(int, pthread_setcanceltype, (int type, int *_oldType),
	return B_TO_POSITIVE_ERROR(sReal_pthread_setcanceltype(type, _oldType));
)


WRAPPER_FUNCTION(int, pthread_getschedparam,
		(pthread_t thread, int *policy, struct sched_param *param),
	return B_TO_POSITIVE_ERROR(sReal_pthread_getschedparam(thread, policy,
		param));
)


WRAPPER_FUNCTION(int, pthread_setschedparam,
		(pthread_t thread, int policy, const struct sched_param *param),
	return B_TO_POSITIVE_ERROR(sReal_pthread_setschedparam(thread, policy,
		param));
)
