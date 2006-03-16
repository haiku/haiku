/*
 * Distributed under the terms of the OpenBeOS license
 */
#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H


#include <BeBuild.h>


/* BSD compatibility */
typedef unsigned long 		u_long;
typedef unsigned int 		u_int;
typedef unsigned short 		u_short;
typedef unsigned char 		u_char;


/* sysV compatibility */
#ifndef _SUPPORT_DEFS_H 
  typedef unsigned long 	ulong;
  typedef unsigned short 	ushort;
  typedef unsigned int 		uint;
#endif
typedef unsigned char		unchar;


typedef long long 			blkcnt_t;
typedef int 				blksize_t;
typedef long long 			fsblkcnt_t;
typedef long long			fsfilcnt_t;
typedef long long           off_t;
typedef long long           ino_t;
typedef int                 cnt_t;
typedef long                dev_t;
typedef long		        pid_t;
typedef long				id_t;

typedef unsigned int 		uid_t;
typedef unsigned int 		gid_t;
typedef unsigned int        mode_t;
typedef unsigned int 		umode_t;
typedef int                 nlink_t;

typedef int          		daddr_t;
typedef char *				caddr_t;

typedef unsigned long		addr_t;
typedef long 				key_t;

#include <null.h>
#include <size_t.h>
#include <time.h>

#endif
