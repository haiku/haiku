/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _ARCH_x86_TYPES_H
#define _ARCH_x86_TYPES_H

#ifndef WIN32
  typedef unsigned long long uint64;
  typedef long long           int64;
#else /* WIN32 */
  typedef unsigned __int64   uint64;
  typedef __int64             int64;
#endif

typedef unsigned long     uint32;
typedef long               int32;
typedef unsigned short    uint16;
typedef short              int16;
typedef unsigned char      uint8;
typedef char                int8;
typedef unsigned long       addr;

#define _OBOS_TIME_T_   int   /* basic time_t type */

/* define this as not all platforms have it set, but we'll make sure as 
 * some conditional checks need it
 */
#define __INTEL__ 1

#endif /* _ARCH_x86_TYPES_H */
