/*
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *	JiSheng Zhang
 */

#ifndef _FW_GLUE_H
#define _FW_GLUE_H

#include <stdint.h>
#include <dpc.h>

#define device_printf(dev, a...) dprintf("firewire:" a)
#define printf(a...) dprintf(a)
#define KASSERT(cond,msg) do { \
	if (!cond) \
		panic msg; \
} while(0)

#ifndef howmany
#define howmany(x, y)   (((x)+((y)-1))/(y)) // x/y的上界
#endif
#define rounddown(x, y) (((x)/(y))*(y)) // 比x小，y的最大的倍数
#define roundup(x, y)   ((((x)+((y)-1))/(y))*(y))  /* to any y */ // 比x大，y的最小倍数
#define roundup2(x, y)  (((x)+((y)-1))&(~((y)-1))) /* if y is powers of two */
#define powerof2(x)     ((((x)-1)&(x))==0) // 是否是2的次方

typedef uint32_t bus_addr_t;
typedef uint32_t bus_size_t;

#define atomic_readandclear_int(ptr) atomic_set((int32 *)(ptr), 0)
#define atomic_set_int(ptr, value) atomic_or((int32 *)(ptr), value)

#define mtx_lock mutex_lock
#define mtx_unlock mutex_unlock
#define mtx_destroy mutex_destroy
#define mtx_init(lockaddr, name, type, opts) mutex_init(lockaddr, name)

#define wakeup(i) release_sem_etc(i->Sem, 0, B_RELEASE_IF_WAITING_ONLY | B_RELEASE_ALL)

#define splfw() 0
#define splx(s) (void)s

#define hz 1000000LL

#define DELAY(n)	snooze(n)

#define OWRITE(sc, offset, value) (*(volatile uint32 *)((char *)(sc->regAddr) + (offset)) = value)
#define OREAD(sc, offset) (*(volatile uint32 *)((char *)(sc->regAddr) + (offset)))

#define MAX_CARDS 4
extern dpc_module_info *gDpc;

#define	__offsetof(type, field)	((size_t)(&((type *)0)->field))

#endif /*_FW_GLUE_H*/
