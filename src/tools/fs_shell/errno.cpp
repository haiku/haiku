/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include "compatibility.h"

#include "fssh_errno.h"

#include <errno.h>


int *
_fssh_errnop(void)
{
	return &errno;
}


int
fssh_get_errno(void)
{
	return errno;
}


void
fssh_set_errno(int error)
{
	errno = error;
}

int
fssh_to_host_error(int error)
{
	#if (defined(__BEOS__) || defined(__HAIKU__))
		return error;
	#else
		return _haiku_to_host_error(error);
	#endif
}
