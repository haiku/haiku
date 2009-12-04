/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS_CONDVAR_H_
#define _FBSD_COMPAT_SYS_CONDVAR_H_


#include <sys/queue.h>

#ifdef __cplusplus
} /* extern "C" */
#include <kernel_c++_structs.h>
extern "C" {
#else
#include <kernel_c++_structs.h>
#endif


struct cv {
	struct ConditionVariable condition;
};


void cv_init(struct cv*, const char*);
void cv_destroy(struct cv*);
void cv_wait(struct cv*, struct mtx*);
int cv_timedwait(struct cv*, struct mtx*, int);
void cv_signal(struct cv*);

#endif /* _FBSD_COMPAT_SYS_CONDVAR_H_ */
