/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef BFS_ENDIAN_H
#define BFS_ENDIAN_H


#include <ByteOrder.h>


#if !defined(BFS_LITTLE_ENDIAN_ONLY) && !defined(BFS_BIG_ENDIAN_ONLY)
//	default setting; BFS is now primarily a little endian file system
#	define BFS_LITTLE_ENDIAN_ONLY
#endif


#if defined(BFS_LITTLE_ENDIAN_ONLY) && B_HOST_IS_LENDIAN \
	|| defined(BFS_BIG_ENDIAN_ONLY) && B_HOST_IS_BENDIAN
		/* host is BFS endian */
#	define BFS_ENDIAN_TO_HOST_INT16(value) value
#	define BFS_ENDIAN_TO_HOST_INT32(value) value
#	define BFS_ENDIAN_TO_HOST_INT64(value) value
#	define HOST_ENDIAN_TO_BFS_INT16(value) value
#	define HOST_ENDIAN_TO_BFS_INT32(value) value
#	define HOST_ENDIAN_TO_BFS_INT64(value) value
#elif defined(BFS_LITTLE_ENDIAN_ONLY) && B_HOST_IS_BENDIAN \
	|| defined(BFS_BIG_ENDIAN_ONLY) && B_HOST_IS_LENDIAN
		/* host is big endian, BFS is little endian or vice versa */
#	define BFS_ENDIAN_TO_HOST_INT16(value) __swap_int16(value)
#	define BFS_ENDIAN_TO_HOST_INT32(value) __swap_int32(value)
#	define BFS_ENDIAN_TO_HOST_INT64(value) __swap_int64(value)
#	define HOST_ENDIAN_TO_BFS_INT16(value) __swap_int16(value)
#	define HOST_ENDIAN_TO_BFS_INT32(value) __swap_int32(value)
#	define HOST_ENDIAN_TO_BFS_INT64(value) __swap_int64(value)
#else
	// ToDo: maybe build a version that supports both, big & little endian?
	//		But since that will need some kind of global data (to
	//		know of what type this file system is), it's probably 
	//		something for the boot loader; anything else would be
	//		a major pain.
#endif

#endif	/* BFS_ENDIAN_H */
