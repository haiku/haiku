/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <pthread.h>

#include "posix_error_mapper.h"


WRAPPER_FUNCTION(int, pthread_spin_init,
		(pthread_spinlock_t* lock, int pshared),
	return B_TO_POSITIVE_ERROR(sReal_pthread_spin_init(lock, pshared));
)


WRAPPER_FUNCTION(int, pthread_spin_destroy, (pthread_spinlock_t* lock),
	return B_TO_POSITIVE_ERROR(sReal_pthread_spin_destroy(lock));
)


WRAPPER_FUNCTION(int, pthread_spin_lock, (pthread_spinlock_t* lock),
	return B_TO_POSITIVE_ERROR(sReal_pthread_spin_lock(lock));
)


WRAPPER_FUNCTION(int, pthread_spin_trylock, (pthread_spinlock_t* lock),
	return B_TO_POSITIVE_ERROR(sReal_pthread_spin_trylock(lock));
)


WRAPPER_FUNCTION(int, pthread_spin_unlock, (pthread_spinlock_t* lock),
	return B_TO_POSITIVE_ERROR(sReal_pthread_spin_unlock(lock));
)
