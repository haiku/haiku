/*
 * Copyright 2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _OBSD_COMPAT_SYS_MUTEX_H_
#define _OBSD_COMPAT_SYS_MUTEX_H_


#include_next <sys/mutex.h>


struct mutex_openbsd {
	struct mtx mtx;
};
#define mutex mutex_openbsd


static inline void
mtx_init_openbsd(struct mutex* mtx, int wantipl)
{
	mtx_init(&mtx->mtx, "OpenBSD mutex", NULL,
		wantipl == IPL_NONE ? 0 : MTX_SPIN);
}
#define mtx_init(mutex, wantipl) mtx_init_openbsd(mutex, wantipl)


static inline void
mtx_enter(struct mutex* mtx)
{
	mtx_lock(&mtx->mtx);
}

static inline void
mtx_leave(struct mutex* mtx)
{
	mtx_unlock(&mtx->mtx);
}


#endif	/* _OBSD_COMPAT_SYS_MUTEX_H_ */
