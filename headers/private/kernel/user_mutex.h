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

void		user_mutex_init();

status_t	_user_mutex_lock(int32* mutex, const char* name, uint32 flags,
				bigtime_t timeout);
status_t	_user_mutex_unlock(int32* mutex, uint32 flags);
status_t	_user_mutex_switch_lock(int32* fromMutex, int32* toMutex,
				const char* name, uint32 flags, bigtime_t timeout);

#ifdef __cplusplus
}
#endif


#endif	/* _KERNEL_USER_MUTEX_H */
