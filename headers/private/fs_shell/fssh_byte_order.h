/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_BYTEORDER_H
#define _FSSH_BYTEORDER_H

#include <endian.h>
	// platform endian.h


#include "fssh_defs.h"


// swap directions
typedef enum {
	FSSH_B_SWAP_HOST_TO_LENDIAN,
	FSSH_B_SWAP_HOST_TO_BENDIAN,
	FSSH_B_SWAP_LENDIAN_TO_HOST,
	FSSH_B_SWAP_BENDIAN_TO_HOST,
	FSSH_B_SWAP_ALWAYS
} fssh_swap_action;


// BSD/networking macros
#ifndef fssh_htonl
#	define fssh_htonl(x) FSSH_B_HOST_TO_BENDIAN_INT32(x)
#	define fssh_ntohl(x) FSSH_B_BENDIAN_TO_HOST_INT32(x)
#	define fssh_htons(x) FSSH_B_HOST_TO_BENDIAN_INT16(x)
#	define fssh_ntohs(x) FSSH_B_BENDIAN_TO_HOST_INT16(x)
#endif

// always swap macros
#define FSSH_B_SWAP_DOUBLE(arg)   __fssh_swap_double(arg)
#define FSSH_B_SWAP_FLOAT(arg)    __fssh_swap_float(arg)
#define FSSH_B_SWAP_INT64(arg)    __fssh_swap_int64(arg)
#define FSSH_B_SWAP_INT32(arg)    __fssh_swap_int32(arg)
#define FSSH_B_SWAP_INT16(arg)    __fssh_swap_int16(arg)

#if BYTE_ORDER == __LITTLE_ENDIAN
// Host is little endian

#define FSSH_B_HOST_IS_LENDIAN 1
#define FSSH_B_HOST_IS_BENDIAN 0

// Host native to little endian
#define FSSH_B_HOST_TO_LENDIAN_DOUBLE(arg)	(double)(arg)
#define FSSH_B_HOST_TO_LENDIAN_FLOAT(arg)	(float)(arg)
#define FSSH_B_HOST_TO_LENDIAN_INT64(arg)	(uint64_t)(arg)
#define FSSH_B_HOST_TO_LENDIAN_INT32(arg)	(uint32_t)(arg)
#define FSSH_B_HOST_TO_LENDIAN_INT16(arg)	(uint16_t)(arg)

// Little endian to host native
#define FSSH_B_LENDIAN_TO_HOST_DOUBLE(arg)	(double)(arg)
#define FSSH_B_LENDIAN_TO_HOST_FLOAT(arg)	(float)(arg)
#define FSSH_B_LENDIAN_TO_HOST_INT64(arg)	(uint64_t)(arg)
#define FSSH_B_LENDIAN_TO_HOST_INT32(arg)	(uint32_t)(arg)
#define FSSH_B_LENDIAN_TO_HOST_INT16(arg)	(uint16_t)(arg)

// Host native to big endian
#define FSSH_B_HOST_TO_BENDIAN_DOUBLE(arg)	__fssh_swap_double(arg)
#define FSSH_B_HOST_TO_BENDIAN_FLOAT(arg)	__fssh_swap_float(arg)
#define FSSH_B_HOST_TO_BENDIAN_INT64(arg)	__fssh_swap_int64(arg)
#define FSSH_B_HOST_TO_BENDIAN_INT32(arg)	__fssh_swap_int32(arg)
#define FSSH_B_HOST_TO_BENDIAN_INT16(arg)	__fssh_swap_int16(arg)

// Big endian to host native
#define FSSH_B_BENDIAN_TO_HOST_DOUBLE(arg)	__fssh_swap_double(arg)
#define FSSH_B_BENDIAN_TO_HOST_FLOAT(arg)	__fssh_swap_float(arg)
#define FSSH_B_BENDIAN_TO_HOST_INT64(arg)	__fssh_swap_int64(arg)
#define FSSH_B_BENDIAN_TO_HOST_INT32(arg)	__fssh_swap_int32(arg)
#define FSSH_B_BENDIAN_TO_HOST_INT16(arg)	__fssh_swap_int16(arg)

#else	// BYTE_ORDER
// Host is big endian

#define FSSH_B_HOST_IS_LENDIAN 0
#define FSSH_B_HOST_IS_BENDIAN 1

// Host native to little endian
#define FSSH_B_HOST_TO_LENDIAN_DOUBLE(arg)	__fssh_swap_double(arg)
#define FSSH_B_HOST_TO_LENDIAN_FLOAT(arg)	__fssh_swap_float(arg)
#define FSSH_B_HOST_TO_LENDIAN_INT64(arg)	__fssh_swap_int64(arg)
#define FSSH_B_HOST_TO_LENDIAN_INT32(arg)	__fssh_swap_int32(arg)
#define FSSH_B_HOST_TO_LENDIAN_INT16(arg)	__fssh_swap_int16(arg)

// Little endian to host native
#define FSSH_B_LENDIAN_TO_HOST_DOUBLE(arg)	__fssh_swap_double(arg)
#define FSSH_B_LENDIAN_TO_HOST_FLOAT(arg)	__fssh_swap_float(arg)
#define FSSH_B_LENDIAN_TO_HOST_INT64(arg)	__fssh_swap_int64(arg)
#define FSSH_B_LENDIAN_TO_HOST_INT32(arg)	__fssh_swap_int32(arg)
#define FSSH_B_LENDIAN_TO_HOST_INT16(arg)	__fssh_swap_int16(arg)

// Host native to big endian
#define FSSH_B_HOST_TO_BENDIAN_DOUBLE(arg)	(double)(arg)
#define FSSH_B_HOST_TO_BENDIAN_FLOAT(arg)	(float)(arg)
#define FSSH_B_HOST_TO_BENDIAN_INT64(arg)	(uint64_t)(arg)
#define FSSH_B_HOST_TO_BENDIAN_INT32(arg)	(uint32_t)(arg)
#define FSSH_B_HOST_TO_BENDIAN_INT16(arg)	(uint16_t)(arg)

// Big endian to host native
#define FSSH_B_BENDIAN_TO_HOST_DOUBLE(arg)	(double)(arg)
#define FSSH_B_BENDIAN_TO_HOST_FLOAT(arg)	(float)(arg)
#define FSSH_B_BENDIAN_TO_HOST_INT64(arg)	(uint64_t)(arg)
#define FSSH_B_BENDIAN_TO_HOST_INT32(arg)	(uint32_t)(arg)
#define FSSH_B_BENDIAN_TO_HOST_INT16(arg)	(uint16_t)(arg)

#endif	// BYTE_ORDER


#ifdef __cplusplus
extern "C" {
#endif 

extern fssh_status_t fssh_swap_data(fssh_type_code type, void *data,
	fssh_size_t length, fssh_swap_action action);
extern bool fssh_is_type_swapped(fssh_type_code type);


// Private implementations
extern double __fssh_swap_double(double arg);
extern float  __fssh_swap_float(float arg);
extern uint64_t __fssh_swap_int64(uint64_t arg);
extern uint32_t __fssh_swap_int32(uint32_t arg);
extern uint16_t __fssh_swap_int16(uint16_t arg);

#ifdef __cplusplus
}
#endif

#endif	// _FSSH_BYTEORDER_H
