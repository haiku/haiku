/*
 * Copyright 2001-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _LIBC_LIMITS_H_
#define _LIBC_LIMITS_H_
	/* Note: The header guard is checked in gcc's limits.h. */


#include <config/types.h>

#include <float.h>		/* for DBL_DIG, FLT_DIG, etc */

/* _GCC_LIMITS_H_ is defined by GCC's internal limits.h to avoid
 * collisions with any defines in this file.
 */
#ifndef _GCC_LIMITS_H_
#	include_next <limits.h>
#endif

#define LONGLONG_MIN    (-9223372036854775807LL - 1)  /* these are Be specific */
#define LONGLONG_MAX    (9223372036854775807LL)
#define ULONGLONG_MAX   (0xffffffffffffffffULL)

#define	ULLONG_MAX		ULONGLONG_MAX
#define	LLONG_MAX		LONGLONG_MAX
#define	LLONG_MIN		LONGLONG_MIN

#define OFF_MAX			LLONG_MAX
#define OFF_MIN			LLONG_MIN

#define ARG_MAX			 		(32768)
#define ATEXIT_MAX			 	(32)
#define CHILD_MAX				(1024)
#define IOV_MAX					(1024)
#define FILESIZEBITS			(64)
#define HOST_NAME_MAX			(255)
#define LINE_MAX				(2048)
#define LINK_MAX				(1)
#define LOGIN_NAME_MAX			(32)
#define MAX_CANON		   		(255)
#define MAX_INPUT				(255)
#define NAME_MAX				(256)
#define NGROUPS_MAX		 		(32)
#define OPEN_MAX				(128)
#define PAGE_SIZE				(4096)
#define PAGESIZE				(4096)
#define PATH_MAX				(1024)
#define PIPE_MAX				(512)
#define PTHREAD_KEYS_MAX		256
#define PTHREAD_STACK_MIN		4096
#define SSIZE_MAX		  		__HAIKU_SADDR_MAX
#define TTY_NAME_MAX			(256)
#define TZNAME_MAX		  		(32)
#define	SYMLINK_MAX				(1024)
#define	SYMLOOP_MAX				(16)

#define _POSIX_ARG_MAX	  		(32768)
#define _POSIX_CHILD_MAX		(1024)
#define _POSIX_HOST_NAME_MAX	(255)
#define _POSIX_LINK_MAX	 		(1)
#define _POSIX_LOGIN_NAME_MAX	(9)
#define _POSIX_MAX_CANON		(255)
#define _POSIX_MAX_INPUT		(255)
#define _POSIX_NAME_MAX	 		(255)
#define _POSIX_NGROUPS_MAX  	(8)
#define _POSIX_OPEN_MAX	 		(128)
#define _POSIX_PATH_MAX	 		(1024)
#define _POSIX_PIPE_BUF	 		(512)
#define _POSIX_SSIZE_MAX		__HAIKU_SADDR_MAX
#define _POSIX_STREAM_MAX   	(8)
#define _POSIX_TTY_NAME_MAX		(256)
#define _POSIX_TZNAME_MAX   	(3)
#define _POSIX_SEM_VALUE_MAX	INT_MAX
#define	_POSIX_SIGQUEUE_MAX		32
#define _POSIX_RTSIG_MAX		8
#define _POSIX_CLOCKRES_MIN		20000000
#define _POSIX_TIMER_MAX		32
#define _POSIX_DELAYTIMER_MAX	32

#define _POSIX2_LINE_MAX		(2048)

#endif /* _LIBC_LIMITS_H_ */
