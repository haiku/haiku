/* 
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _KERNEL_KTYPES_H
#define _KERNEL_KTYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <SupportDefs.h>

typedef uint16 mode_t;
typedef int    pid_t;
typedef int32  thread_id;
typedef int32  region_id;
typedef int32  aspace_id;
typedef int32  team_id;
typedef int32  sem_id;
typedef int32  port_id;
typedef int32  image_id;
typedef uint32 dev_t;
typedef uint64 ino_t;
typedef uint16 nlink_t;
typedef uint32 uid_t;
typedef uint32 gid_t;


#ifndef NULL
#define NULL 0
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

typedef unsigned long			addr;

#ifdef __cplusplus
}
#endif

#endif	/* _KERNEL_KTYPES_H */
