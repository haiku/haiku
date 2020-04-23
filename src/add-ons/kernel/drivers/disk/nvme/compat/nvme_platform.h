/*
 * Copyright 2019, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Augustin Cavalier <waddlesplash>
 */
#ifndef __NVME_PLATFORM_H__
#define __NVME_PLATFORM_H__

#include <lock.h>


#define pthread_mutex_t				mutex
#undef PTHREAD_MUTEX_INITIALIZER
#define PTHREAD_MUTEX_INITIALIZER	MUTEX_INITIALIZER(__FUNCTION__)
#define pthread_mutex_init(mtx, attr) mutex_init(mtx, __FUNCTION__)
#define pthread_mutex_destroy		mutex_destroy
#define pthread_mutex_lock			mutex_lock
#define pthread_mutex_unlock		mutex_unlock


#endif /* __NVME_PLATFORM_H__ */
