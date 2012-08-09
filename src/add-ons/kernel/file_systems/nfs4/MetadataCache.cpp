/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "MetadataCache.h"

#include <NodeMonitor.h>

#include "Inode.h"


MetadataCache::MetadataCache(Inode* inode)
	:
	fExpire(0),
	fForceValid(false),
	fInode(inode),
	fInited(false)
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
		st->st_blksize = fStatCache.st_blksize;
		st->st_blocks = fStatCache.st_blocks;
		return B_OK;
	}

	return B_ERROR;
}


void
MetadataCache::SetStat(const struct stat& st)
{
	MutexLocker _(fLock);
	if (fInited)
		NotifyChanges(&fStatCache, &st);

	fStatCache = st;
	fExpire = time(NULL) + kExpirationTime;
	fInited = true;
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

	if (!fForceValid)
		it.CurrentValuePointer()->fForceValid = false;

	if (!it.Current().fForceValid && it.Current().fExpire < time(NULL)) {
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
	entry.fForceValid = fForceValid;
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


void
MetadataCache::NotifyChanges(const struct stat* oldStat,
	const struct stat* newStat)
{
	uint32 flags = 0;
	if (oldStat->st_size != newStat->st_size)
		flags |= B_STAT_SIZE;
	if (oldStat->st_mode != newStat->st_mode)
		flags |= B_STAT_MODE;
	if (oldStat->st_uid != newStat->st_uid)
		flags |= B_STAT_UID;
	if (oldStat->st_gid != newStat->st_gid)
		flags |= B_STAT_GID;

	if (memcmp(&oldStat->st_atim, &newStat->st_atim,
		sizeof(struct timespec) == 0))
		flags |= B_STAT_ACCESS_TIME;

	if (memcmp(&oldStat->st_ctim, &newStat->st_ctim,
		sizeof(struct timespec) == 0))
		flags |= B_STAT_CHANGE_TIME;

	if (memcmp(&oldStat->st_crtim, &newStat->st_crtim,
		sizeof(struct timespec) == 0))
		flags |= B_STAT_CREATION_TIME;

	if (memcmp(&oldStat->st_mtim, &newStat->st_mtim,
		sizeof(struct timespec) == 0))
		flags |= B_STAT_MODIFICATION_TIME;

	notify_stat_changed(fInode->GetFileSystem()->DevId(), fInode->ID(), flags);
}

