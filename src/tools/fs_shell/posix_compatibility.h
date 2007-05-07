/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_POSIX_COMPATIBILITY_H
#define _FSSH_POSIX_COMPATIBILITY_H

#include <limits.h>

// BeOS doesn't define [U]LLONG_{MIN,MAX}, but [U]LONGLONG_{MIN,MAX}.
// And under some stupid Linux platforms we don't get the macros when
// compiling in C++ mode.
#ifndef LLONG_MAX
#	ifdef LONGLONG_MAX
		// define to the BeOS macro
#		define LLONG_MAX	LONGLONG_MAX
#	else
		// minimum acceptable value as per standard
#		define LLONG_MAX	(9223372036854775807LL)
#	endif
#endif

#endif	// _FSSH_POSIX_COMPATIBILITY_H
