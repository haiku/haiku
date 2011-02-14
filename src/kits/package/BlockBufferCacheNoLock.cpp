/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/BlockBufferCacheNoLock.h>


namespace BPackageKit {


BBlockBufferCacheNoLock::BBlockBufferCacheNoLock(size_t blockSize,
	uint32 maxCachedBlocks)
	:
	BBlockBufferCache(blockSize, maxCachedBlocks)
{
}


BBlockBufferCacheNoLock::~BBlockBufferCacheNoLock()
{
}


bool
BBlockBufferCacheNoLock::Lock()
{
	return true;
}


void
BBlockBufferCacheNoLock::Unlock()
{
}


}	// namespace BPackageKit
