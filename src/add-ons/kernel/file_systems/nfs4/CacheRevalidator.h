/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef CACHEREVALIDATOR_H
#define CACHEREVALIDATOR_H


#include <lock.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedQueue.h>

#include "DirectoryCache.h"


class CacheRevalidator {
public:
							CacheRevalidator();
							~CacheRevalidator();

	inline	void			Lock();
	inline	void			Unlock();


	inline	void			AddDirectory(DirectoryCache* cache);
	inline	void			RemoveDirectory(DirectoryCache* cache);

private:
			void			_DirectoryCacheRevalidator();
	static	status_t		_DirectoryRevalidatorStart(void* object);
			status_t		_StartRevalidator();

			thread_id		fThread;
			bool			fThreadCancel;
			sem_id			fWaitCancel;

			DoublyLinkedQueue<DirectoryCache>	fDirectoryCaches;
			mutex			fDirectoryCachesLock;
};


inline void
CacheRevalidator::Lock()
{
	mutex_lock(&fDirectoryCachesLock);
}

inline void
CacheRevalidator::Unlock()
{
	mutex_unlock(&fDirectoryCachesLock);
}


inline void
CacheRevalidator::AddDirectory(DirectoryCache* cache)
{
	fDirectoryCaches.Add(cache);
}


inline void
CacheRevalidator::RemoveDirectory(DirectoryCache* cache)
{
	fDirectoryCaches.Remove(cache);
}


#endif	// CACHEREVALIDATOR_H
			
