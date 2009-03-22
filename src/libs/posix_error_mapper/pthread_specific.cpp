/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <pthread.h>

#include "posix_error_mapper.h"


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
