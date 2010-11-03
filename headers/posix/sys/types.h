/*
 * Copyright 2002-2010 Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H


#include <config/types.h>

#include <BeBuild.h>


/* BSD compatibility */
typedef unsigned long 		u_long;
typedef unsigned int 		u_int;
typedef unsigned short 		u_short;
typedef unsigned char 		u_char;


/* sysV compatibility */
typedef unsigned long 		ulong;
typedef unsigned short 		ushort;
typedef unsigned int 		uint;
typedef unsigned char		unchar;


typedef __haiku_int64 		blkcnt_t;
typedef __haiku_std_int32	blksize_t;
typedef __haiku_int64		fsblkcnt_t;
typedef __haiku_int64		fsfilcnt_t;
typedef __haiku_int64		off_t;
typedef __haiku_int64		ino_t;
typedef __haiku_std_int32	cnt_t;
typedef __haiku_int32		dev_t;
typedef __haiku_int32		pid_t;
typedef __haiku_int32		id_t;

typedef __haiku_std_uint32	uid_t;
typedef __haiku_std_uint32	gid_t;
typedef __haiku_std_uint32  mode_t;
typedef __haiku_std_uint32	umode_t;
typedef __haiku_std_int32	nlink_t;

#ifdef __HAIKU_BEOS_COMPATIBLE_TYPES
	typedef int		daddr_t;	/* disk address */
#else
	typedef off_t	daddr_t;	/* disk address */
#endif
typedef char*				caddr_t;

typedef __haiku_addr_t		addr_t;
typedef __haiku_int32 		key_t;

#include <null.h>
#include <size_t.h>
#include <time.h>

#endif
