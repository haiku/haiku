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


WRAPPER_FUNCTION(int, pthread_atfork,
		(void (*prepare)(void), void (*parent)(void), void (*child)(void)),
	return B_TO_POSITIVE_ERROR(sReal_pthread_atfork(prepare, parent, child));
)


WRAPPER_FUNCTION(int, pthread_once,
		(pthread_once_t *once_control, void (*init_routine)(void)),
	return B_TO_POSITIVE_ERROR(sReal_pthread_once(once_control, init_routine));
)


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


WRAPPER_FUNCTION(int, pthread_key_create,
		(pthread_key_t *key, void (*destructorFunc)(void*)),
	return B_TO_POSITIVE_ERROR(sReal_pthread_key_create(key, destructorFunc));
)


WRAPPER_FUNCTION(int, pthread_key_delete, (pthread_key_t key),
	return B_TO_POSITIVE_ERROR(sReal_pthread_key_delete(key));
)


WRAPPER_FUNCTION(int, pthread_setspecific,
		(pthread_key_t key, const void *value),
	return B_TO_POSITIVE_ERROR(sReal_pthread_setspecific(key, value));
)
