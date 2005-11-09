#ifndef _LIBC_LIMITS_H_
#define _LIBC_LIMITS_H_
	// Note: The header guard is checked in gcc's limits.h.

#include <float.h>		/* for DBL_DIG, FLT_DIG, etc */

/* _GCC_LIMITS_H_ is defined by GCC's internal limits.h to avoid
 * collisions with any defines in this file.
 */
#if __INTEL__
# ifndef _GCC_LIMITS_H_
#  include_next <limits.h>
# endif
#else

#define CHAR_BIT		(8)

#define SCHAR_MIN		(-127-1)
#define SCHAR_MAX		(127)

#define	UCHAR_MAX		(255U)

#define CHAR_MIN		SCHAR_MIN
#define CHAR_MAX		SCHAR_MAX

#define MB_LEN_MAX		(1)

#define SHRT_MIN		(-32767-1)
#define SHRT_MAX		(32767)

#define USHRT_MAX		(65535U)

#define LONG_MIN		(-2147483647L-1)
#define LONG_MAX	  	(2147483647L)

#define ULONG_MAX		(4294967295U)

#define	INT_MIN			LONG_MIN
#define INT_MAX			LONG_MAX
#define UINT_MAX		ULONG_MAX

#endif /* else not INTEL */

#define LONGLONG_MIN    (-9223372036854775807LL - 1)  /* these are Be specific */
#define LONGLONG_MAX    (9223372036854775807LL)
#define ULONGLONG_MAX   (0xffffffffffffffffULL)                            

#define	ULLONG_MAX		ULONGLONG_MAX
#define	LLONG_MAX		LONGLONG_MAX
#define	LLONG_MIN		LONGLONG_MIN

#define OFF_MAX			LLONG_MAX
#define OFF_MIN			LLONG_MIN

/* These are various BeOS implementation limits */

#define ARG_MAX			 		(32768)
#define ATEXIT_MAX			 	(32)	 /* XXXdbg */
#define CHILD_MAX				(1024)
#define IOV_MAX					(256)	/* really there is no limit */
#define FILESIZEBITS			(64)
#define LINK_MAX				(1)
#define LOGIN_NAME_MAX			(32)	 /* XXXdbg */
#define MAX_CANON		   		(255)
#define MAX_INPUT				(255)
#define NAME_MAX				(256)
#define NGROUPS_MAX		 		(32)
#define OPEN_MAX				(128)
#define PATH_MAX				(1024)
#define PIPE_MAX				(512)
#define SSIZE_MAX		  		(2147483647L)
#define TTY_NAME_MAX			(256)
#define TZNAME_MAX		  		(32)
#define	SYMLINKS_MAX			(16)

#define _POSIX_ARG_MAX	  		(32768)
#define _POSIX_CHILD_MAX		(1024)
#define _POSIX_LINK_MAX	 		(1)
#define _POSIX_LOGIN_NAME_MAX	(9)		/* XXXdbg */
#define _POSIX_MAX_CANON		(255)
#define _POSIX_MAX_INPUT		(255)
#define _POSIX_NAME_MAX	 		(255) 
#define _POSIX_NGROUPS_MAX  	(0)  
#define _POSIX_OPEN_MAX	 		(128)
#define _POSIX_PATH_MAX	 		(1024)
#define _POSIX_PIPE_BUF	 		(512) 
#define _POSIX_SSIZE_MAX		(2147483647L)
#define _POSIX_STREAM_MAX   	(8)
#define _POSIX_TTY_NAME_MAX		(256)
#define _POSIX_TZNAME_MAX   	(3)

#endif /* _LIBC_LIMITS_H_ */
