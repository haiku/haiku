/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BLOCK_BUFFER_CACHE_NO_LOCK_H
#define BLOCK_BUFFER_CACHE_NO_LOCK_H


#include <package/hpkg/BlockBufferCache.h>


using BPackageKit::BHPKG::BBlockBufferCache;


class BlockBufferCacheNoLock : public BBlockBufferCache {
public:
								BlockBufferCacheNoLock(size_t blockSize,
									uint32 maxCachedBlocks);
	virtual						~BlockBufferCacheNoLock();

	virtual	bool				Lock();
	virtual	void				Unlock();
};


#endif	// BLOCK_BUFFER_CACHE_NO_LOCK_H
