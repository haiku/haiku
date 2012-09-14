/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "CacheRevalidator.h"

#include <fs_cache.h>

#include "NFS4Defs.h"


CacheRevalidator::CacheRevalidator()
	:
	fThreadCancel(false),
	fWaitCancel(create_sem(0, NULL))
{
	mutex_init(&fDirectoryCachesLock, NULL);

	_StartRevalidator();
}


CacheRevalidator::~CacheRevalidator()
{
	fThreadCancel = true;
	release_sem(fWaitCancel);
	status_t result;
	wait_for_thread(fThread, &result);

	delete_sem(fThreadCancel);
	mutex_destroy(&fDirectoryCachesLock);
}


status_t
CacheRevalidator::_StartRevalidator()
{
	fThreadCancel = false;
	fThread = spawn_kernel_thread(&CacheRevalidator::_DirectoryRevalidatorStart,
		"NFSv4 Cache Revalidator", B_NORMAL_PRIORITY, this);
	if (fThread < B_OK)
		return fThread;

	status_t result = resume_thread(fThread);
	if (result != B_OK) {
		kill_thread(fThread);
		return result;
	}

	return B_OK;
}


status_t
CacheRevalidator::_DirectoryRevalidatorStart(void* object)
{
	CacheRevalidator* revalidator = reinterpret_cast<CacheRevalidator*>(object);
	revalidator->_DirectoryCacheRevalidator();
	return B_OK;
}


void
CacheRevalidator::_DirectoryCacheRevalidator()
{
	while (!fThreadCancel) {
		MutexLocker locker(fDirectoryCachesLock);
		DirectoryCache* current = fDirectoryCaches.Head();

		if (current == NULL) {
			locker.Unlock();

			status_t result = acquire_sem_etc(fWaitCancel, 1,
				B_RELATIVE_TIMEOUT,	DirectoryCache::kExpirationTime);

			if (result != B_TIMED_OUT) {
				if (result == B_OK)
					release_sem(fWaitCancel);
				return;
			}
			continue;
		}

		current->Lock();
		if (!current->Valid()) {
			RemoveDirectory(current);
			current->Unlock();
			continue;
		}

		if (current->ExpireTime() > system_time()) {
			current->Unlock();
			locker.Unlock();

			status_t result = acquire_sem_etc(fWaitCancel, 1,
				B_ABSOLUTE_TIMEOUT, current->ExpireTime());
			if (result != B_TIMED_OUT) {
				if (result == B_OK)
					release_sem(fWaitCancel);
				return;
			}

			continue;
		}

		fDirectoryCaches.RemoveHead();
		current->fRevalidated = false;

		if (current->Revalidate() == B_OK)
			AddDirectory(current);

		current->Unlock();
	}
}

