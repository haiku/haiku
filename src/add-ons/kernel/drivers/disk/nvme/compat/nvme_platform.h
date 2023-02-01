/*
 * Copyright 2019-2022, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 */
#ifndef __NVME_PLATFORM_H__
#define __NVME_PLATFORM_H__

#include <lock.h>


#define pthread_mutex_t				recursive_lock
#undef PTHREAD_RECURSIVE_MUTEX_INITIALIZER
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER	RECURSIVE_LOCK_INITIALIZER(__FUNCTION__)
#define pthread_mutex_init(mtx, attr) recursive_lock_init(mtx, __FUNCTION__)
#define pthread_mutex_destroy		recursive_lock_destroy
#define pthread_mutex_lock			recursive_lock_lock
#define pthread_mutex_unlock		recursive_lock_unlock


#endif /* __NVME_PLATFORM_H__ */
