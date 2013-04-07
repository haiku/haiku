/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef _PTHREAD_MUTEX_LOCKER_H
#define _PTHREAD_MUTEX_LOCKER_H


#include <pthread.h>

#include <AutoLocker.h>


namespace BPrivate {


class AutoLockerMutexLocking {
public:
	inline bool Lock(pthread_mutex_t* lockable)
	{
		return pthread_mutex_lock(lockable) == 0;
	}

	inline void Unlock(pthread_mutex_t* lockable)
	{
		pthread_mutex_unlock(lockable);
	}
};


typedef AutoLocker<pthread_mutex_t, AutoLockerMutexLocking> PthreadMutexLocker;


}	// namespace BPrivate

using BPrivate::PthreadMutexLocker;


#endif	// _PTHREAD_MUTEX_LOCKER_H
