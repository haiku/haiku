//******************************************************************************
//
// File:			BlockCache.h		
//
// Description:		Manages a cache of same size blocks of memory
//
// Copyright (c) 2002 Massimiliano Origgi
//
// License:			
//
//******************************************************************************

#ifndef _BLOCK_CACHE_H
#define _BLOCK_CACHE_H

#include <BeBuild.h>
#include <Locker.h>

// Allocation type
enum
{
	B_OBJECT_CACHE = 0,
	B_MALLOC_CACHE = 1
};


class BBlockCache
{
public:

					BBlockCache(size_t CacheSize, size_t BlockSize,	uint32 Type);
virtual				~BBlockCache(void);

		void		*Get(size_t BlockSize);
		void		Save(void *Data, size_t BlockSize);

private:

virtual	void		_ReservedBlockCache1();
virtual	void		_ReservedBlockCache2();

					BBlockCache(const BBlockCache &);
		BBlockCache	&operator=(const BBlockCache &);

		size_t		fCacheSize;
		size_t		fBlockSize;
		void		*fCache;
		size_t			fMark;
		BLocker		fLock;
		void		*(*fAlloc)(size_t Size);
		void		(*fFree)(void *Data);
		
		uint32		_reserved[2];
};

#endif
