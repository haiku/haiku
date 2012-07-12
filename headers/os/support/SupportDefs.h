/*
 * Copyright 2004-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author:
 *		Erik Jaesler (erik@cgsoftware.com)
 */
#ifndef _SUPPORT_DEFS_H
#define _SUPPORT_DEFS_H


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
typedef int64					nanotime_t;
typedef uint32					type_code;
typedef uint32					perform_code;

typedef __haiku_phys_addr_t		phys_addr_t;
typedef phys_addr_t				phys_size_t;

typedef	__haiku_generic_addr_t	generic_addr_t;
typedef	generic_addr_t			generic_size_t;


/* printf()/scanf() format strings for [u]int* types */
#define B_PRId8			"d"
#define B_PRIi8			"i"
#define B_PRId16		"d"
#define B_PRIi16		"i"
#define B_PRId32		__HAIKU_PRI_PREFIX_32 "d"
#define B_PRIi32		__HAIKU_PRI_PREFIX_32 "i"
#define B_PRId64		__HAIKU_PRI_PREFIX_64 "d"
#define B_PRIi64		__HAIKU_PRI_PREFIX_64 "i"
#define B_PRIu8			"u"
#define B_PRIo8			"o"
#define B_PRIx8			"x"
#define B_PRIX8			"X"
#define B_PRIu16		"u"
#define B_PRIo16		"o"
#define B_PRIx16		"x"
#define B_PRIX16		"X"
#define B_PRIu32		__HAIKU_PRI_PREFIX_32 "u"
#define B_PRIo32		__HAIKU_PRI_PREFIX_32 "o"
#define B_PRIx32		__HAIKU_PRI_PREFIX_32 "x"
#define B_PRIX32		__HAIKU_PRI_PREFIX_32 "X"
#define B_PRIu64		__HAIKU_PRI_PREFIX_64 "u"
#define B_PRIo64		__HAIKU_PRI_PREFIX_64 "o"
#define B_PRIx64		__HAIKU_PRI_PREFIX_64 "x"
#define B_PRIX64		__HAIKU_PRI_PREFIX_64 "X"

#define B_SCNd8 		"hhd"
#define B_SCNi8 		"hhi"
#define B_SCNd16		"hd"
#define B_SCNi16	 	"hi"
#define B_SCNd32 		__HAIKU_PRI_PREFIX_32 "d"
#define B_SCNi32	 	__HAIKU_PRI_PREFIX_32 "i"
#define B_SCNd64		__HAIKU_PRI_PREFIX_64 "d"
#define B_SCNi64 		__HAIKU_PRI_PREFIX_64 "i"
#define B_SCNu8 		"hhu"
#define B_SCNo8 		"hho"
#define B_SCNx8 		"hhx"
#define B_SCNu16		"hu"
#define B_SCNo16		"ho"
#define B_SCNx16		"hx"
#define B_SCNu32 		__HAIKU_PRI_PREFIX_32 "u"
#define B_SCNo32 		__HAIKU_PRI_PREFIX_32 "o"
#define B_SCNx32 		__HAIKU_PRI_PREFIX_32 "x"
#define B_SCNu64		__HAIKU_PRI_PREFIX_64 "u"
#define B_SCNo64		__HAIKU_PRI_PREFIX_64 "o"
#define B_SCNx64		__HAIKU_PRI_PREFIX_64 "x"

/* printf() format strings for some standard types */
/* size_t */
#define B_PRIuSIZE		__HAIKU_PRI_PREFIX_ADDR "u"
#define B_PRIoSIZE		__HAIKU_PRI_PREFIX_ADDR "o"
#define B_PRIxSIZE		__HAIKU_PRI_PREFIX_ADDR "x"
#define B_PRIXSIZE		__HAIKU_PRI_PREFIX_ADDR "X"
/* ssize_t */
#define B_PRIdSSIZE		__HAIKU_PRI_PREFIX_ADDR "d"
#define B_PRIiSSIZE		__HAIKU_PRI_PREFIX_ADDR "i"
/* addr_t */
#define B_PRIuADDR		__HAIKU_PRI_PREFIX_ADDR "u"
#define B_PRIoADDR		__HAIKU_PRI_PREFIX_ADDR "o"
#define B_PRIxADDR		__HAIKU_PRI_PREFIX_ADDR "x"
#define B_PRIXADDR		__HAIKU_PRI_PREFIX_ADDR "X"
/* phys_addr_t */
#define B_PRIuPHYSADDR	__HAIKU_PRI_PREFIX_PHYS_ADDR "u"
#define B_PRIoPHYSADDR	__HAIKU_PRI_PREFIX_PHYS_ADDR "o"
#define B_PRIxPHYSADDR	__HAIKU_PRI_PREFIX_PHYS_ADDR "x"
#define B_PRIXPHYSADDR	__HAIKU_PRI_PREFIX_PHYS_ADDR "X"
/* generic_addr_t */
#define B_PRIuGENADDR	__HAIKU_PRI_PREFIX_GENERIC_ADDR "u"
#define B_PRIoGENADDR	__HAIKU_PRI_PREFIX_GENERIC_ADDR "o"
#define B_PRIxGENADDR	__HAIKU_PRI_PREFIX_GENERIC_ADDR "x"
#define B_PRIXGENADDR	__HAIKU_PRI_PREFIX_GENERIC_ADDR "X"
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


/* Printed width of a pointer with the %p format (minus 0x prefix). */
#ifdef B_HAIKU_64_BIT
#	define B_PRINTF_POINTER_WIDTH	16
#else
#	define B_PRINTF_POINTER_WIDTH	8
#endif


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


/* Use the built-in atomic functions, if requested and available. */

#if defined(B_USE_BUILTIN_ATOMIC_FUNCTIONS) && __GNUC__ >= 4

#define atomic_test_and_set(valuePointer, newValue, testAgainst)	\
	__sync_val_compare_and_swap(valuePointer, testAgainst, newValue)
#define atomic_add(valuePointer, addValue)	\
	__sync_fetch_and_add(valuePointer, addValue)
#define atomic_and(valuePointer, andValue)	\
	__sync_fetch_and_and(valuePointer, andValue)
#define atomic_or(valuePointer, orValue)	\
	__sync_fetch_and_or(valuePointer, orValue)
#define atomic_get(valuePointer)	\
	__sync_fetch_and_or(valuePointer, 0)
	// No equivalent to atomic_get(). We simulate it via atomic or. On most
	// (all?) 32+ bit architectures aligned 32 bit reads will be atomic anyway,
	// though.

// Note: No equivalent for atomic_set(). It could be simulated by a
// get + atomic test and set loop, but calling the atomic_set() implementation
// might be faster.

#endif	// B_USE_BUILTIN_ATOMIC_FUNCTIONS && __GNUC__ >= 4


#endif	/* _SUPPORT_DEFS_H */
