/*
 * Copyright 2009, Colin Günther, coling@gmx.de
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_SLEEPQUEUE_H_
#define _FBSD_COMPAT_SYS_SLEEPQUEUE_H_


void sleepq_add(void*, struct mtx*, const char*, int, int);
int	sleepq_broadcast(void*, int, int, int);
void sleepq_lock(void*);
void sleepq_release(void*);
void sleepq_remove(struct thread*, void*);
int	sleepq_timedwait(void*, int);

#endif /* _FBSD_COMPAT_SYS_SLEEPQUEUE_H_ */
