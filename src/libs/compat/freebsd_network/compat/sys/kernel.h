/*
 * Copyright 2007-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_KERNEL_H_
#define _FBSD_COMPAT_SYS_KERNEL_H_


#include <stddef.h>
#include <string.h>

#include <sys/haiku-module.h>

#include <sys/linker_set.h>
#include <sys/queue.h>


/*
 *
 * In FreeBSD hz holds the count of how often the thread scheduler is invoked
 * per second. Moreover this is the rate at which FreeBSD can generate callouts
 * (kind of timeout mechanism).
 * For FreeBSD 8 this is typically 1000 times per second. This value is defined
 * in a file called subr_param.c
 *
 * For Haiku this value is much higher, due to using another timeout scheduling
 * mechanism, which has a resolution of 1 MHz. So hz for Haiku is set to
 * 1000000. Suffixing LL prevents integer overflows during calculations.
 * as it defines a long long constant.*/
#define hz	1000000LL


typedef void (*system_init_func_t)(void *);


struct __system_init {
	system_init_func_t func;
};

/* TODO implement SYSINIT/SYSUNINIT subsystem */
#define SYSINIT(uniquifier, subsystem, order, func, ident) \
	struct __system_init __init_##uniquifier = { (system_init_func_t) func }

#define SYSUNINIT(uniquifier, subsystem, order, func, ident) \
	struct __system_init __uninit_##uniquifier = { (system_init_func_t) func }

#define TUNABLE_INT(path, var)
#define TUNABLE_INT_FETCH(path, var)

extern int ticks;

#endif
