/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Erik Jaesler (erik@cgsoftware.com)
 */
#ifndef _SUPPORT_DEFS_H
#define _SUPPORT_DEFS_H


/* this must be located before the include of sys/types.h */
#if !defined(_SYS_TYPES_H) && !defined(_SYS_TYPES_H_)
typedef unsigned long			ulong;
typedef unsigned int			uint;
typedef unsigned short			ushort;
#endif

#include <BeBuild.h>
#include <Errors.h>

#include <sys/types.h>


// Shorthand type formats
typedef	signed char				int8;
typedef unsigned char			uint8;
typedef volatile signed char   	vint8;
typedef volatile unsigned char	vuint8;

typedef	short					int16;
typedef unsigned short			uint16;
typedef volatile short			vint16;
typedef volatile unsigned short	vuint16;

typedef	long					int32;
typedef unsigned long			uint32;
typedef volatile long			vint32;
typedef volatile unsigned long	vuint32;

typedef	long long				int64;
typedef unsigned long long		uint64;
typedef volatile long long		vint64;
typedef volatile unsigned long long	vuint64;

typedef volatile long			vlong;
typedef volatile int			vint;
typedef volatile short			vshort;
typedef volatile char			vchar;

typedef volatile unsigned long	vulong;
typedef volatile unsigned int	vuint;
typedef volatile unsigned short	vushort;
typedef volatile unsigned char	vuchar;

typedef unsigned char			uchar;
typedef unsigned short          unichar;


// Descriptive formats
typedef int32					status_t;
typedef int64					bigtime_t;
typedef uint32					type_code;
typedef uint32					perform_code;


// Empty string ("")
#ifdef __cplusplus
extern const char *B_EMPTY_STRING;
#endif


// min and max comparisons
#ifndef __cplusplus
#	ifndef min
#		define min(a,b) ((a)>(b)?(b):(a))
#	endif
#	ifndef max
#		define max(a,b) ((a)>(b)?(a):(b))
#	endif
#endif 

// min() and max() won't work in C++
#define min_c(a,b) ((a)>(b)?(b):(a))
#define max_c(a,b) ((a)>(b)?(a):(b))


// Grandfathering
#ifndef __cplusplus
#	include <stdbool.h>
#endif 

#ifndef NULL
#	define NULL (0)
#endif


#ifdef __cplusplus
extern "C" {
#endif

// Atomic functions; previous value is returned
extern int32	atomic_set(vint32 *value, int32 newValue);
extern int32	atomic_test_and_set(vint32 *value, int32 newValue, int32 testAgainst);
extern int32	atomic_add(vint32 *value, int32 addValue);
extern int32	atomic_and(vint32 *value, int32 andValue);
extern int32	atomic_or(vint32 *value, int32 orValue);	
extern int32	atomic_get(vint32 *value);

extern int64	atomic_set64(vint64 *value, int64 newValue);
extern int64	atomic_test_and_set64(vint64 *value, int64 newValue, int64 testAgainst);
extern int64	atomic_add64(vint64 *value, int64 addValue);
extern int64	atomic_and64(vint64 *value, int64 andValue);
extern int64	atomic_or64(vint64 *value, int64 orValue);	
extern int64	atomic_get64(vint64 *value);

// Other stuff
extern void*	get_stack_frame(void);

#ifdef __cplusplus
}
#endif

// Obsolete or discouraged API

// use 'true' and 'false'
#ifndef FALSE
#	define FALSE	0
#endif
#ifndef TRUE
#	define TRUE		1
#endif

#endif	// _SUPPORT_DEFS_H
