/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_USER_MUTEX_H
#define _KERNEL_USER_MUTEX_H


#include <SupportDefs.h>


#ifdef __cplusplus
extern "C" {
#endif

struct user_mutex_context;

void		user_mutex_init();
void		delete_user_mutex_context(struct user_mutex_context* context);

status_t	_user_mutex_lock(int32* mutex, const char* name, uint32 flags,
				bigtime_t timeout);
status_t	_user_mutex_unblock(int32* mutex, uint32 flags);
status_t	_user_mutex_switch_lock(int32* fromMutex, uint32 fromFlags,
				int32* toMutex, const char* name, uint32 toFlags, bigtime_t timeout);
status_t	_user_mutex_sem_acquire(int32* sem, const char* name, uint32 flags,
				bigtime_t timeout);
status_t	_user_mutex_sem_release(int32* sem, uint32 flags);

#ifdef __cplusplus
}
#endif


#endif	/* _KERNEL_USER_MUTEX_H */
