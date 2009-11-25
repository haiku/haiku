/*
 * Copyright 2007-2009, Ingo Weinhold, bonefish@cs.tu-berlin.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FSSH_DEFS_H
#define _FSSH_DEFS_H


#include "fssh_types.h"


// 32/64 bitness
#ifdef HAIKU_HOST_PLATFORM_64_BIT
#	define FSSH_B_HAIKU_64_BIT			1
#else
#	define FSSH_B_HAIKU_32_BIT			1
#endif

// Limits
#define FSSH_B_DEV_NAME_LENGTH		128
#define FSSH_B_FILE_NAME_LENGTH		256
#define FSSH_B_PATH_NAME_LENGTH 	1024
#define FSSH_B_ATTR_NAME_LENGTH		(FSSH_B_FILE_NAME_LENGTH-1)
#define FSSH_B_MIME_TYPE_LENGTH		(FSSH_B_ATTR_NAME_LENGTH - 15)
#define FSSH_B_MAX_SYMLINKS			16

// Open Modes
#define FSSH_B_READ_ONLY 		FSSH_O_RDONLY	// read only
#define FSSH_B_WRITE_ONLY 		FSSH_O_WRONLY 	// write only
#define FSSH_B_READ_WRITE		FSSH_O_RDWR   	// read and write

#define	FSSH_B_FAIL_IF_EXISTS	FSSH_O_EXCL		// exclusive create
#define FSSH_B_CREATE_FILE		FSSH_O_CREAT	// create the file
#define FSSH_B_ERASE_FILE		FSSH_O_TRUNC	// erase the file's data
#define FSSH_B_OPEN_AT_END	   	FSSH_O_APPEND	// point to the end of the data

// Node Flavors
enum fssh_node_flavor {
  FSSH_B_FILE_NODE		= 0x01,
  FSSH_B_SYMLINK_NODE	= 0x02,
  FSSH_B_DIRECTORY_NODE	= 0x04,
  FSSH_B_ANY_NODE		= 0x07
};


#if defined(__GNUC__) && __GNUC__ > 3
#define fssh_offsetof(type,member)   __builtin_offsetof(type, member)
#else
#define fssh_offsetof(type,member) ((size_t)&((type*)0)->member)
#endif

#define fssh_min_c(a,b) ((a)>(b)?(b):(a))
#define fssh_max_c(a,b) ((a)>(b)?(a):(b))

#define _FSSH_PACKED	__attribute__((packed))


#endif	// _FSSH_DEFS_H
