/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_COMPAT_SYS__MUTEX_H_
#define _FBSD_COMPAT_SYS__MUTEX_H_


#include <lock.h>
#include <KernelExport.h>


struct mtx {
	int						type;
	union {
		struct {
			mutex			lock;
			thread_id		owner;
		}					mutex;
		recursive_lock		recursive;
		struct {
			spinlock		lock;
			cpu_status		state;
		}					spinlock;
	} u;
};


#endif /* _FBSD_COMPAT_SYS__MUTEX_H_ */
