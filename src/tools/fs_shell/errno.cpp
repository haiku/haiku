/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include <BeOSBuildCompatibility.h>

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
