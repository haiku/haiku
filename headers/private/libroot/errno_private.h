/*
 * Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ERRNO_PRIVATE_H
#define _ERRNO_PRIVATE_H


#include <errno.h>

//#define TRACE_ERRNO

#if defined(TRACE_ERRNO) && !defined(_KERNEL_MODE)
#	include <OS.h>
#	define __set_errno(x) 													  \
		do { 																  \
			int error = (x);												  \
			debug_printf("%s:%d - setting errno to %x\n", __FILE__, __LINE__, \
				error);														  \
			errno = error;													  \
		} while (0)
#else
#	define __set_errno(x) \
		do { errno = (x); } while (0)
#endif


#endif	// _ERRNO_PRIVATE_H
