/*
 * Copyright 2007-2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_KERNEL_H_
#define _FBSD_COMPAT_SYS_KERNEL_H_


#include <stddef.h>

#include <sys/haiku-module.h>

#include <sys/linker_set.h>
#include <sys/queue.h>


/* The rate at which FreeBSD can generate callouts (kind of timeout mechanism).
 * For FreeBSD 8 this is typically 1000 times per second (100 for ARM).
 * This value is defined in a file called subr_param.c
 *
 * While Haiku can have a much higher granularity, it is not a good idea to have
 * this since FreeBSD tries to do certain tasks based on ticks, for instance
 * autonegotiation and wlan scanning.
 * Suffixing LL prevents integer overflows during calculations.
 * as it defines a long long constant. */
#define hz	1000LL

int32_t get_ticks();
#define ticks (get_ticks())

#define ticks_to_usecs(t) (1000000*((bigtime_t)t) / hz)
#define usecs_to_ticks(t) (((bigtime_t)t*hz) / 1000000)


/* sysinit */
enum sysinit_elem_order {
	SI_ORDER_FIRST = 0x0000000,
	SI_ORDER_SECOND = 0x0000001,
	SI_ORDER_THIRD = 0x0000002,
	SI_ORDER_FOURTH = 0x0000003,
	SI_ORDER_MIDDLE = 0x1000000,
	SI_ORDER_ANY = 0xfffffff	/* last */
};

typedef void (*system_init_func_t)(void *);

struct sysinit {
	const char* name;
	enum sysinit_elem_order order;
	system_init_func_t func;
	const void* arg;
};

#define SYSINIT(uniquifier, subsystem, _order, _func, ident) \
static struct sysinit sysinit_##uniquifier = { \
	.name 		= #uniquifier,		\
	.order 		= _order,			\
	.func		= _func,			\
	.arg		= ident,			\
};									\
DATA_SET(__freebsd_sysinit, sysinit_##uniquifier)

#define SYSUNINIT(uniquifier, subsystem, _order, _func, ident) \
static struct sysinit sysuninit_##uniquifier = { \
	.name 		= #uniquifier,		\
	.order 		= _order,			\
	.func		= _func,			\
	.arg		= ident,			\
};									\
DATA_SET(__freebsd_sysuninit, sysuninit_##uniquifier)


/* confighooks */
typedef void (*ich_func_t)(void *_arg);

struct intr_config_hook {
	TAILQ_ENTRY(intr_config_hook) ich_links;
	ich_func_t	ich_func;
	void		*ich_arg;
};

int config_intrhook_establish(struct intr_config_hook *hook);
void config_intrhook_disestablish(struct intr_config_hook *hook);


/* misc. */
#define TUNABLE_INT(path, var)
#define TUNABLE_INT_FETCH(path, var)


#endif // _FBSD_COMPAT_SYS_KERNEL_H_
