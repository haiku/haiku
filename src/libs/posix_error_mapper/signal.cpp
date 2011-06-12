/*
 * Copyright 2010-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <signal.h>

#include "posix_error_mapper.h"


WRAPPER_FUNCTION(int, pthread_sigmask,
		(int how, const sigset_t *set, sigset_t *oldSet),
	return B_TO_POSITIVE_ERROR(sReal_pthread_sigmask(how, set, oldSet));
)


WRAPPER_FUNCTION(int, sigwait, (const sigset_t *set, int *signal),
	return B_TO_POSITIVE_ERROR(sReal_sigwait(set, signal));
)
