/*
 * Copyright 2007, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_POSIX_COMPATIBILITY_H
#define _FSSH_POSIX_COMPATIBILITY_H

// BeOS doesn't define [U]LLONG_{MIN,MAX}, but [U]LONGLONG_{MIN,MAX}
#include <limits.h>
#ifndef LLONG_MIN
#	define LLONG_MIN	LONGLONG_MIN
#	define LLONG_MAX	LONGLONG_MAX
#	define ULLONG_MAX	ULONGLONG_MAX
#endif

#endif	// _FSSH_POSIX_COMPATIBILITY_H

