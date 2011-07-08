/*
 * Copyright 2007 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_TYPES_H_
#define _FBSD_COMPAT_SYS_TYPES_H_


#include <posix/stdint.h>
#include <posix/sys/types.h>

#include <sys/cdefs.h>

#include <machine/endian.h>
#include <sys/_types.h>


typedef int boolean_t;
typedef __const char* c_caddr_t;
typedef uint64_t u_quad_t;

#endif
