/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "BlockBufferPoolKernel.h"


BlockBufferPoolKernel::BlockBufferPoolKernel(size_t blockSize,
	uint32 maxCachedBlocks)
	:
	BlockBufferPoolImpl(blockSize, maxCachedBlocks, this)
{
	mutex_init(&fLock, "BlockBufferPool");
}


BlockBufferPoolKernel::~BlockBufferPoolKernel()
{
	mutex_destroy(&fLock);
}


bool
BlockBufferPoolKernel::Lock()
{
	mutex_lock(&fLock);
	return true;
}


void
BlockBufferPoolKernel::Unlock()
{
	mutex_unlock(&fLock);
}
