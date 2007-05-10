#ifndef _FBSD_COMPAT_SYS_KERNEL_H_
#define _FBSD_COMPAT_SYS_KERNEL_H_

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <sys/haiku-module.h>
#include <sys/module.h>
#include <sys/errno.h>

#define hz	1000000LL

#define bootverbose		1

#define KASSERT(cond,msg) do { \
	if (!cond) \
		panic msg; \
} while(0)

typedef void (*system_init_func_t)(void *);

struct __system_init {
	system_init_func_t func;
};

/* TODO */
#define SYSINIT(uniquifier, subsystem, order, func, ident) \
	struct __system_init __init_##uniquifier = { func }

#define TUNABLE_INT(path, var)

#define __packed			__attribute__ ((packed))
#define __printflike(a, b)	__attribute__ ((format (__printf__, a, b)))

int printf(const char *format, ...) __printflike(1, 2);

#endif
