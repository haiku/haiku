/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_KTYPES_H
#define _KERNEL_KTYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <types.h>

typedef uint16 mode_t;
typedef int    pid_t;
typedef int    thread_id;
typedef int    region_id;
typedef int    aspace_id;
typedef int    proc_id;
typedef int    sem_id;
typedef int    port_id;
typedef int    image_id;
typedef uint64 ino_t;
typedef uint64 vnode_id;
typedef uint32 fs_id;
typedef uint16 nlink_t;
typedef uint32 uid_t;
typedef uint32 gid_t;

/* compat with beos (was int32 on beos) */
typedef int status_t;


#ifdef  _OBOS_TIME_T_
typedef _OBOS_TIME_T_     time_t;
#undef  _OBOS_TIME_T_
#endif /* _OBOS_TIME_T_ */

typedef int64 bigtime_t;

#ifndef NULL
#define NULL 0
#endif

#ifndef __cplusplus

#define false 0
#define true 1

typedef int bool;

#endif

/*
 *    XXX serious hack that doesn't really solve the problem.
 *       As of right now, some versions of the toolchain expect size_t to
 *       be unsigned long (newer ones than 2.95.2 and beos), and the older
 *       ones need it to be unsigned int. It's an actual failure when 
 *       operator new is declared. This will have to be resolved in the future. 
 */

#ifdef __BEOS__
typedef unsigned long       size_t;
typedef signed long         ssize_t;
#else
typedef unsigned int        size_t;
typedef signed int          ssize_t;
#endif
typedef int64               off_t;

typedef unsigned char           u_char;
typedef unsigned short          u_short;
typedef unsigned int            u_int;
typedef unsigned long           u_long;

typedef unsigned char           uchar;
typedef unsigned short          ushort; 
typedef unsigned int            uint;
typedef unsigned long           ulong;

// Handled in arch_ktypes.h

//typedef unsigned long addr;

typedef uint32                  socklen_t;

#ifdef __cplusplus
}
#endif

#endif
