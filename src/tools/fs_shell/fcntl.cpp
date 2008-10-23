/*
 * Copyright 2007-2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "fssh_fcntl.h"

#include <fcntl.h>
#include <stdarg.h>

#include "fssh_errno.h"
#include "partition_support.h"
#include "stat_util.h"


using namespace FSShell;


#if (!defined(__BEOS__) && !defined(__HAIKU__))
	// The _kern_open() defined in libroot_build.so.
	extern "C"  int _kern_open(int fd, const char *path, int openMode,
		int perms);
#endif


int
fssh_open(const char *pathname, int oflags, ...)
{
	va_list args;
	va_start(args, oflags);

	// get the mode, if O_CREAT was specified
	fssh_mode_t mode = 0;
	if (oflags & FSSH_O_CREAT)
		mode = va_arg(args, fssh_mode_t);

	va_end(args);

	// Use the _kern_open() defined in libroot on BeOS incompatible systems.
	// Required for proper attribute emulation support.
	int fd;
	#if (defined(__BEOS__) || defined(__HAIKU__))
		fd = open(pathname, to_platform_open_mode(oflags),
			to_platform_mode(mode));
	#else
		fd = _kern_open(-1, pathname, to_platform_open_mode(oflags),
			to_platform_mode(mode));
		if (fd < 0) {
			fssh_set_errno(fd);
			fd = -1;
		}

	#endif

	restricted_file_opened(fd);

	return fd;
}


int
fssh_creat(const char *path, fssh_mode_t mode)
{
	return fssh_open(path, FSSH_O_WRONLY | FSSH_O_CREAT | FSSH_O_TRUNC, mode);
}
