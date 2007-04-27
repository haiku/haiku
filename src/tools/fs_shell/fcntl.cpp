/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */

#include "fssh_fcntl.h"

#include <fcntl.h>
#include <stdarg.h>

#include "stat_util.h"


using namespace FSShell;


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

// TODO: That's not perfect yet: We should use open() on BeOS compatible
// platforms and _kern_open() otherwise.
	return open(pathname, to_platform_open_mode(oflags),
		to_platform_mode(mode));
}
