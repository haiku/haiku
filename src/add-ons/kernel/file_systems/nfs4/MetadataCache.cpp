/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "MetadataCache.h"


MetadataCache::MetadataCache()
	:
	fExpire(0),
	fForceValid(false)
{
	mutex_init(&fLock, NULL);
}


MetadataCache::~MetadataCache()
{
	mutex_destroy(&fLock);
}


status_t
MetadataCache::GetStat(struct stat* st)
{
	MutexLocker _(fLock);
	if (fForceValid || fExpire > time(NULL)) {
		// Do not touch other members of struct stat
		st->st_size = fStatCache.st_size;
		st->st_mode = fStatCache.st_mode;
		st->st_nlink = fStatCache.st_nlink;
		st->st_uid = fStatCache.st_uid;
		st->st_gid = fStatCache.st_gid;
		st->st_atim = fStatCache.st_atim;
		st->st_ctim = fStatCache.st_ctim;
		st->st_crtim = fStatCache.st_crtim;
		st->st_mtim = fStatCache.st_mtim;

		return B_OK;
	}

	return B_ERROR;
}


void
MetadataCache::SetStat(const struct stat& st)
{
	MutexLocker _(fLock);
	fStatCache = st;
	fExpire = time(NULL) + kExpirationTime;
}


void
MetadataCache::GrowFile(size_t newSize)
{
	MutexLocker _(fLock);
	fStatCache.st_size = max_c(newSize, fStatCache.st_size);
}


status_t
MetadataCache::GetAccess(uid_t uid, uint32* allowed)
{
	MutexLocker _(fLock);
	AVLTreeMap<uid_t, AccessEntry>::Iterator it = fAccessCache.Find(uid);
	if (!it.HasCurrent())
		return B_ENTRY_NOT_FOUND;

	if (it.Current().fExpire < time(NULL)) {
		it.Remove();
		return B_ERROR;
	}

	*allowed = it.Current().fAllowed;

	return B_OK;
}


void
MetadataCache::SetAccess(uid_t uid, uint32 allowed)
{
	MutexLocker _(fLock);
	AVLTreeMap<uid_t, AccessEntry>::Iterator it = fAccessCache.Find(uid);
	if (it.HasCurrent())
		it.Remove();

	AccessEntry entry;
	entry.fAllowed = allowed;
	entry.fExpire = time(NULL) + kExpirationTime;
	fAccessCache.Insert(uid, entry);
}


status_t
MetadataCache::LockValid()
{
	MutexLocker _(fLock);
	if (fForceValid || fExpire > time(NULL)) {
		fForceValid = true;
		return B_OK;
	}

	return B_ERROR;
}


void
MetadataCache::UnlockValid()
{
	MutexLocker _(fLock);
	fExpire = time(NULL) + kExpirationTime;
	fForceValid = false;
}


