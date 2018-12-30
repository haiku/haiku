/*
 * Copyright 2018, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 */
#ifndef _FBSD_COMPAT_SYS_SX_H_
#define _FBSD_COMPAT_SYS_SX_H_


#include <lock.h>


struct sx {
	rw_lock l;
};

#define sx_init(lock, name)		rw_lock_init(&(lock)->l, name)
#define sx_xlock(lock)			rw_lock_write_lock(&(lock)->l)
#define sx_xunlock(lock)		rw_lock_write_unlock(&(lock)->l)
#define sx_destroy(lock)		rw_lock_destroy(&(lock)->l)

#define sx_assert(sx, what)


#endif /* _FBSD_COMPAT_SYS_SX_H_ */
