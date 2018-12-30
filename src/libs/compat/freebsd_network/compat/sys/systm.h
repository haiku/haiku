/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_SYSTM_H_
#define _FBSD_COMPAT_SYS_SYSTM_H_


#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include <machine/atomic.h>
#include <machine/cpufunc.h>

#include <sys/callout.h>
#include <sys/cdefs.h>
#include <sys/queue.h>

#include <sys/libkern.h>


#define printf freebsd_printf
int printf(const char *format, ...) __printflike(1, 2);


#define ovbcopy(f, t, l) bcopy((f), (t), (l))

#if KDEBUG_LEVEL_2
#define INVARIANTS
#endif

#if KDEBUG_LEVEL_1
#define bootverbose 1
#else
#define bootverbose 0
#endif

#ifdef INVARIANTS
#define KASSERT(cond,msg) do {	\
	if (!(cond))				\
		panic msg;				\
} while (0)
#else
#define KASSERT(exp,msg) do { \
} while (0)
#endif

#define DELAY(n) \
	do {				\
		if (n < 1000)	\
			spin(n);	\
		else			\
			snooze(n);	\
	} while (0)

#define	cold 0

void wakeup(void *);
void wakeup_one(void *);

#ifndef CTASSERT	/* Allow lint to override */
#define	CTASSERT(x)	_Static_assert(x, "compile-time assertion failed")
#endif


static inline int
copyin(const void * __restrict udaddr, void * __restrict kaddr,
	size_t len)
{
	return user_memcpy(kaddr, udaddr, len);
}


static inline int
copyout(const void * __restrict kaddr, void * __restrict udaddr,
	size_t len)
{
	return user_memcpy(udaddr, kaddr, len);
}


static inline void log(int level, const char *fmt, ...) { }


int snprintf(char *, size_t, const char *, ...) __printflike(3, 4);
extern int sprintf(char *buf, const char *, ...);

extern int driver_vprintf(const char *format, va_list vl);
#define vprintf(fmt, vl) driver_vprintf(fmt, vl)

extern int vsnprintf(char *, size_t, const char *, __va_list)
	__printflike(3, 0);

int msleep(void *, struct mtx *, int, const char *, int);
int _pause(const char *, int);
#define pause(waitMessage, timeout) _pause((waitMessage), (timeout))
#define tsleep(channel, priority, waitMessage, timeout) \
	msleep((channel), NULL, (priority), (waitMessage), (timeout))
#define msleep_spin(chan, mtx, wmesg, timo) \
	msleep(chan, mtx, PZERO, wmesg, timo)
#define mtx_sleep msleep

void critical_enter(void);
void critical_exit(void);

struct unrhdr;
struct unrhdr *new_unrhdr(int low, int high, struct mtx *mutex);
void delete_unrhdr(struct unrhdr *);
int alloc_unr(struct unrhdr *);
void free_unr(struct unrhdr *, u_int);

extern char *getenv(const char *name);
extern void    freeenv(char *env);
extern char *kern_getenv(const char *name);

#endif	/* _FBSD_COMPAT_SYS_SYSTM_H_ */
