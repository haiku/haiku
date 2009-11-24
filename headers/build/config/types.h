/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CONFIG_TYPES_H
#define _CONFIG_TYPES_H


#include <config/HaikuConfig.h>

#include <inttypes.h>


/* fixed-width types -- the __haiku_std_[u]int* types correspond to the POSIX
   [u]int*_t types, the _haiku_[u]int* types to the BeOS [u]int* types. If
   __HAIKU_BEOS_COMPATIBLE_TYPES is not defined both sets are identical. Once
   we drop compatibility for good, we can consolidate the types.
*/
typedef int8_t				__haiku_std_int8;
typedef uint8_t				__haiku_std_uint8;
typedef int16_t				__haiku_std_int16;
typedef uint16_t			__haiku_std_uint16;
typedef int32_t				__haiku_std_int32;
typedef uint32_t			__haiku_std_uint32;
typedef int64_t				__haiku_std_int64;
typedef uint64_t			__haiku_std_uint64;

typedef __haiku_std_int8	__haiku_int8;
typedef __haiku_std_uint8	__haiku_uint8;
typedef __haiku_std_int16	__haiku_int16;
typedef __haiku_std_uint16	__haiku_uint16;
#ifdef __HAIKU_BEOS_COMPATIBLE_TYPES
typedef signed long int		__haiku_int32;
typedef unsigned long int	__haiku_uint32;
#else
typedef __haiku_std_int32	__haiku_int32;
typedef __haiku_std_uint32	__haiku_uint32;
#endif
typedef __haiku_std_int64	__haiku_int64;
typedef __haiku_std_uint64	__haiku_uint64;

/* address types */
#ifdef __HAIKU_ARCH_64_BIT
	typedef	__haiku_int64	__haiku_saddr_t;
	typedef	__haiku_uint64	__haiku_addr_t;
#else
	typedef	__haiku_int32	__haiku_saddr_t;
	typedef	__haiku_uint32	__haiku_addr_t;
#endif

/* address type limits */
#ifdef __HAIKU_ARCH_64_BIT
#	define __HAIKU_SADDR_MAX	(9223372036854775807LL)
#	define __HAIKU_ADDR_MAX		(18446744073709551615ULL)
#else
#	define __HAIKU_SADDR_MAX	(2147483647)
#	define __HAIKU_ADDR_MAX		(4294967295U)
#endif
#define __HAIKU_SADDR_MIN		(-__HAIKU_SADDR_MAX-1)


#endif	/* _CONFIG_TYPES_H */
