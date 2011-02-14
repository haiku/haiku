/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__BLOCK_BUFFER_CACHE_NO_LOCK_H
#define _PACKAGE__BLOCK_BUFFER_CACHE_NO_LOCK_H


#include <package/hpkg/BlockBufferCache.h>


namespace BPackageKit {


class BBlockBufferCacheNoLock : public BHPKG::BBlockBufferCache {
public:
								BBlockBufferCacheNoLock(size_t blockSize,
									uint32 maxCachedBlocks);
	virtual						~BBlockBufferCacheNoLock();

	virtual	bool				Lock();
	virtual	void				Unlock();
};


}	// namespace BPackageKit


#endif	// _PACKAGE__BLOCK_BUFFER_CACHE_NO_LOCK_H
