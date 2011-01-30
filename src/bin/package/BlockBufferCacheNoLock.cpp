/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include "BlockBufferCacheNoLock.h"


BlockBufferCacheNoLock::BlockBufferCacheNoLock(size_t blockSize,
	uint32 maxCachedBlocks)
	:
	BBlockBufferCache(blockSize, maxCachedBlocks)
{
}


BlockBufferCacheNoLock::~BlockBufferCacheNoLock()
{
}


bool
BlockBufferCacheNoLock::Lock()
{
	return true;
}


void
BlockBufferCacheNoLock::Unlock()
{
}
