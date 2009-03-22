/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <pthread.h>

#include "posix_error_mapper.h"


WRAPPER_FUNCTION(int, pthread_atfork,
		(void (*prepare)(void), void (*parent)(void), void (*child)(void)),
	return B_TO_POSITIVE_ERROR(sReal_pthread_atfork(prepare, parent, child));
)


WRAPPER_FUNCTION(int, pthread_once,
		(pthread_once_t *once_control, void (*init_routine)(void)),
	return B_TO_POSITIVE_ERROR(sReal_pthread_once(once_control, init_routine));
)
