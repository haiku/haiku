/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef METADATACACHE_H
#define METADATACACHE_H


#include <fs_interface.h>
#include <lock.h>
#include <SupportDefs.h>
#include <util/AutoLock.h>
#include <util/AVLTreeMap.h>


class Inode;

struct AccessEntry {
	time_t	fExpire;
	bool	fForceValid;

	uint32	fAllowed;
};

class MetadataCache {
public:
								MetadataCache(Inode* inode);
								~MetadataCache();

					status_t	GetStat(struct stat* st);
					void		SetStat(const struct stat& st);
					void		GrowFile(size_t newSize);

					status_t	GetAccess(uid_t uid, uint32* allowed);
					void		SetAccess(uid_t uid, uint32 allowed);

					status_t	LockValid();
					void		UnlockValid();

	inline			void		InvalidateStat();
	inline			void		InvalidateAccess();

	inline			void		Invalidate();

	static const	time_t		kExpirationTime	= 60;

protected:
					void		NotifyChanges(const struct stat* oldStat,
									const struct stat* newStat);

private:
					struct stat	fStatCache;
					time_t		fExpire;
					bool		fForceValid;

					Inode*		fInode;
					bool		fInited;

					AVLTreeMap<uid_t, AccessEntry>	fAccessCache;

					mutex		fLock;
};


inline void
MetadataCache::InvalidateStat()
{
	MutexLocker _(fLock);
	if (!fForceValid)
		fExpire = 0;
}


inline void
MetadataCache::InvalidateAccess()
{
	MutexLocker _(fLock);
	if (!fForceValid)
		fAccessCache.MakeEmpty();
}


inline void
MetadataCache::Invalidate()
{
	InvalidateStat();
	InvalidateAccess();
}


#endif	// METADATACACHE_H
			
