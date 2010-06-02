/*
 * Copyright 2004-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Erik Jaesler (erik@cgsoftware.com)
 */
#ifndef _SUPPORT_DEFS_H
#define _SUPPORT_DEFS_H


#include <config_build/types.h>

#include <BeBuild.h>
#include <Errors.h>

#include <inttypes.h>
#include <sys/types.h>


/* fixed-size integer types */
typedef	__haiku_int8			int8;
typedef __haiku_uint8			uint8;
typedef	__haiku_int16			int16;
typedef __haiku_uint16			uint16;
typedef	__haiku_int32			int32;
typedef __haiku_uint32			uint32;
typedef	__haiku_int64			int64;
typedef __haiku_uint64			uint64;

/* shorthand types */
typedef volatile int8   		vint8;
typedef volatile uint8			vuint8;
typedef volatile int16			vint16;
typedef volatile uint16			vuint16;
typedef volatile int32			vint32;
typedef volatile uint32			vuint32;
typedef volatile int64			vint64;
typedef volatile uint64			vuint64;

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

/* descriptive types */
typedef int32					status_t;
typedef int64					bigtime_t;
typedef uint32					type_code;
typedef uint32					perform_code;

#ifdef HAIKU_HOST_PLATFORM_64_BIT
typedef uint64					phys_addr_t;
#else
typedef uint32					phys_addr_t;
#endif
typedef phys_addr_t				phys_size_t;

typedef	addr_t					generic_addr_t;
typedef	size_t					generic_size_t;


/* printf()/scanf() format strings for [u]int* types */
#define B_PRId8			PRId8
#define B_PRIi8			PRIi8
#define B_PRId16		PRId16
#define B_PRIi16		PRIi16
#define B_PRId32		PRId32
#define B_PRIi32		PRIi32
#define B_PRId64		PRId64
#define B_PRIi64		PRIi64
#define B_PRIu8			PRIu8
#define B_PRIo8			PRIo8
#define B_PRIx8			PRIx8
#define B_PRIX8			PRIX8
#define B_PRIu16		PRIu16
#define B_PRIo16		PRIo16
#define B_PRIx16		PRIx16
#define B_PRIX16		PRIX16
#define B_PRIu32		PRIu32
#define B_PRIo32		PRIo32
#define B_PRIx32		PRIx32
#define B_PRIX32		PRIX32
#define B_PRIu64		PRIu64
#define B_PRIo64		PRIo64
#define B_PRIx64		PRIx64
#define B_PRIX64		PRIX64

#define B_SCNd8 		SCNd8
#define B_SCNi8 		SCNi8
#define B_SCNd16		SCNd16
#define B_SCNi16 		SCNi16
#define B_SCNd32 		SCNd32
#define B_SCNi32 		SCNi32
#define B_SCNd64		SCNd64
#define B_SCNi64 		SCNi64
#define B_SCNu8 		SCNu8
#define B_SCNo8 		SCNo8
#define B_SCNx8 		SCNx8
#define B_SCNu16		SCNu16
#define B_SCNu16		SCNu16
#define B_SCNx16		SCNx16
#define B_SCNu32 		SCNu32
#define B_SCNo32 		SCNo32
#define B_SCNx32 		SCNx32
#define B_SCNu64		SCNu64
#define B_SCNo64		SCNo64
#define B_SCNx64		SCNx64


/* printf() format strings for some standard types */
/* addr_t, size_t, ssize_t */
#ifdef HAIKU_HOST_PLATFORM_64_BIT
#	define B_PRIuADDR	B_PRIu64
#	define B_PRIoADDR	B_PRIo64
#	define B_PRIxADDR	B_PRIx64
#	define B_PRIXADDR	B_PRIX64
#	define B_PRIuSIZE	B_PRIu64
#	define B_PRIoSIZE	B_PRIo64
#	define B_PRIxSIZE	B_PRIx64
#	define B_PRIXSIZE	B_PRIX64
#	define B_PRIdSSIZE	B_PRId64
#	define B_PRIiSSIZE	B_PRIi64
#else
#	define B_PRIuADDR	B_PRIu32
#	define B_PRIoADDR	B_PRIo32
#	define B_PRIxADDR	B_PRIx32
#	define B_PRIXADDR	B_PRIX32
#	define B_PRIuSIZE	B_PRIu32
#	define B_PRIoSIZE	B_PRIo32
#	define B_PRIxSIZE	B_PRIx32
#	define B_PRIXSIZE	B_PRIX32
#	define B_PRIdSSIZE	B_PRId32
#	define B_PRIiSSIZE	B_PRIi32
#endif
/* phys_addr_t */
#ifdef HAIKU_HOST_PLATFORM_64_BIT
#	define B_PRIuPHYSADDR	B_PRIu64
#	define B_PRIoPHYSADDR	B_PRIo64
#	define B_PRIxPHYSADDR	B_PRIx64
#	define B_PRIXPHYSADDR	B_PRIX64
#else
#	define B_PRIuPHYSADDR	B_PRIu32
#	define B_PRIoPHYSADDR	B_PRIo32
#	define B_PRIxPHYSADDR	B_PRIx32
#	define B_PRIXPHYSADDR	B_PRIX32
#endif
/* generic_addr_t */
#define B_PRIuGENADDR	B_PRIuADDR
#define B_PRIoGENADDR	B_PRIoADDR
#define B_PRIxGENADDR	B_PRIxADDR
#define B_PRIXGENADDR	B_PRIXADDR
/* off_t */
#define B_PRIdOFF		B_PRId64
#define B_PRIiOFF		B_PRIi64
/* dev_t */
#define B_PRIdDEV		B_PRId32
#define B_PRIiDEV		B_PRIi32
/* ino_t */
#define B_PRIdINO		B_PRId64
#define B_PRIiINO		B_PRIi64
/* time_t */
#define B_PRIdTIME		B_PRId32
#define B_PRIiTIME		B_PRIi32


/* Empty string ("") */
#ifdef __cplusplus
extern const char *B_EMPTY_STRING;
#endif


/* min and max comparisons */
#ifndef __cplusplus
#	ifndef min
#		define min(a,b) ((a)>(b)?(b):(a))
#	endif
#	ifndef max
#		define max(a,b) ((a)>(b)?(a):(b))
#	endif
#endif

/* min() and max() are functions in C++ */
#define min_c(a,b) ((a)>(b)?(b):(a))
#define max_c(a,b) ((a)>(b)?(a):(b))


/* Grandfathering */
#ifndef __cplusplus
#	include <stdbool.h>
#endif

#ifndef NULL
#	define NULL (0)
#endif


#ifdef __cplusplus
extern "C" {
#endif

/* Atomic functions; previous value is returned */
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

/* Other stuff */
extern void*	get_stack_frame(void);

#ifdef __cplusplus
}
#endif

/* Obsolete or discouraged API */

/* use 'true' and 'false' */
#ifndef FALSE
#	define FALSE	0
#endif
#ifndef TRUE
#	define TRUE		1
#endif

#endif	/* _SUPPORT_DEFS_H */
