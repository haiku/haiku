/* 
** Copyright 2002-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/


#include <unistd.h>
#include <sys/resource.h>
#include <stdio.h>
#include <errno.h>


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
	// This is about what BeOS does, better POSIX conformance would be nice, though

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

