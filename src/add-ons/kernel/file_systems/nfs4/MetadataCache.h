/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef METADATACACHE_H
#define METADATACACHE_H


#include <lock.h>
#include <SupportDefs.h>
#include <util/AutoLock.h>
#include <util/AVLTreeMap.h>


struct AccessEntry {
	time_t	fExpire;
	uint32	fAllowed;
};

class MetadataCache {
public:
								MetadataCache();
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
private:
					struct stat	fStatCache;
					time_t		fExpire;
					bool		fForceValid;

					AVLTreeMap<uid_t, AccessEntry>	fAccessCache;

					mutex		fLock;
};


inline void
MetadataCache::InvalidateStat()
{
	MutexLocker _(fLock);
	fExpire = 0;
}


inline void
MetadataCache::InvalidateAccess()
{
	MutexLocker _(fLock);
	fAccessCache.MakeEmpty();
}


inline void
MetadataCache::Invalidate()
{
	InvalidateStat();
	InvalidateAccess();
}


#endif	// METADATACACHE_H
			
