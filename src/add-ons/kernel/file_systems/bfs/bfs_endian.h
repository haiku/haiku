/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef BFS_ENDIAN_H
#define BFS_ENDIAN_H


#include <ByteOrder.h>


#if !defined(BFS_LITTLE_ENDIAN_ONLY) && !defined(BFS_BIG_ENDIAN_ONLY)
//	default setting; BFS is now primarily a little endian file system
#	define BFS_LITTLE_ENDIAN_ONLY
#endif


#ifdef BFS_LITTLE_ENDIAN_ONLY
#	if B_HOST_IS_LENDIAN
#		define BFS_ENDIAN_TO_HOST_INT16(value) value
#		define BFS_ENDIAN_TO_HOST_INT32(value) value
#		define BFS_ENDIAN_TO_HOST_INT64(value) value
#	else
#		define BFS_ENDIAN_TO_HOST_INT16(value) __swap_int16(value)
#		define BFS_ENDIAN_TO_HOST_INT16(value) __swap_int32(value)
#		define BFS_ENDIAN_TO_HOST_INT16(value) __swap_int64(value)
#	endif
#elif BFS_BIG_ENDIAN_ONLY
#	if B_HOST_IS_BENDIAN
#		define BFS_ENDIAN_TO_HOST_INT16(value) value
#		define BFS_ENDIAN_TO_HOST_INT32(value) value
#		define BFS_ENDIAN_TO_HOST_INT64(value) value
#	else
#		define BFS_ENDIAN_TO_HOST_INT16(value) __swap_int16(value)
#		define BFS_ENDIAN_TO_HOST_INT16(value) __swap_int32(value)
#		define BFS_ENDIAN_TO_HOST_INT16(value) __swap_int64(value)
#	endif
#else
	// ToDo: maybe build a version that supports both, big & little endian?
	//		But since that will need some kind of global data (to
	//		know of what type this file system is), it's probably 
	//		something for the boot loader; anything else would be
	//		a major pain.
#endif

#endif	/* BFS_ENDIAN_H */
