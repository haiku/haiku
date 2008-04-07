/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <unistd.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

#include <SupportDefs.h>

#include <libroot_private.h>


int
getdtablesize(void)
{
	struct rlimit rlimit;
	if (getrlimit(RLIMIT_NOFILE, &rlimit) < 0)
		return 0;

	return rlimit.rlim_cur;
}


long
sysconf(int name)
{
	// TODO: This is about what BeOS does, better POSIX conformance would be nice, though

	switch (name) {
		case _SC_ARG_MAX:
			return ARG_MAX;
		case _SC_CHILD_MAX:
			return CHILD_MAX;
		case _SC_CLK_TCK:
			return CLK_TCK;
		case _SC_JOB_CONTROL:
			return 1;
		case _SC_NGROUPS_MAX:
			return NGROUPS_MAX;
		case _SC_OPEN_MAX:
			return OPEN_MAX;
		case _SC_SAVED_IDS:
			return 1;
		case _SC_STREAM_MAX:
			return STREAM_MAX;
		case _SC_TZNAME_MAX:
			return TZNAME_MAX;
		case _SC_VERSION:
			return _POSIX_VERSION;
		case _SC_GETGR_R_SIZE_MAX:
			return MAX_GROUP_BUFFER_SIZE;
		case _SC_GETPW_R_SIZE_MAX:
			return MAX_PASSWD_BUFFER_SIZE;
		case _SC_PAGE_SIZE:
			return B_PAGE_SIZE;
	}

	return -1;
}


long
fpathconf(int fd, int name)
{
	switch (name) {
		// ToDo: out of what stupidity have those been defined differently?
		case _PC_CHOWN_RESTRICTED:
		case _POSIX_CHOWN_RESTRICTED:
			return 1;

		case _PC_MAX_CANON:
			return MAX_CANON;

		case _PC_MAX_INPUT:
			return MAX_INPUT;

		case _PC_NAME_MAX:
			return NAME_MAX;

		case _PC_NO_TRUNC:
		case _POSIX_NO_TRUNC:
			return 0;

		case _PC_PATH_MAX:
			return PATH_MAX;

		case _PC_PIPE_BUF:
			return 4096;

		case _PC_LINK_MAX:
			return LINK_MAX;

		// _PC_VDISABLE
		// _POSIX_VDISABLE
		// _POSIX_JOB_CONTROL
		// _POSIX_SAVED_IDS
	}

	return -1;
}


long
pathconf(const char *path, int name)
{
	return fpathconf(-1, name);
}


size_t
confstr(int name, char *buffer, size_t length)
{
	size_t stringLength = 1;
	char *string = "";

	switch (name) {
		case 0:
			break;
		default:
			errno = EINVAL;
			return 0;
	}

	if (buffer != NULL)
		strlcpy(buffer, string, min_c(length, stringLength));

	return stringLength;
}

