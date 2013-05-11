/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "BlockBufferCacheKernel.h"


BlockBufferCacheKernel::BlockBufferCacheKernel(size_t blockSize,
	uint32 maxCachedBlocks)
	:
	BlockBufferCacheImpl(blockSize, maxCachedBlocks, this)
{
	mutex_init(&fLock, "BlockBufferCache");
}


BlockBufferCacheKernel::~BlockBufferCacheKernel()
{
	mutex_destroy(&fLock);
}


bool
BlockBufferCacheKernel::Lock()
{
	mutex_lock(&fLock);
	return true;
}


void
BlockBufferCacheKernel::Unlock()
{
	mutex_unlock(&fLock);
}
