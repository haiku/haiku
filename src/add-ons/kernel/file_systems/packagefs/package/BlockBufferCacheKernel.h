/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BLOCK_BUFFER_CACHE_KERNEL_H
#define BLOCK_BUFFER_CACHE_KERNEL_H


#include <lock.h>

#include <package/hpkg/BlockBufferCacheImpl.h>
#include <package/hpkg/BufferCache.h>


using BPackageKit::BHPKG::BBufferCacheLockable;
using BPackageKit::BHPKG::BPrivate::BlockBufferCacheImpl;


class BlockBufferCacheKernel
	: public BlockBufferCacheImpl, BBufferCacheLockable {
public:
								BlockBufferCacheKernel(size_t blockSize,
									uint32 maxCachedBlocks);
	virtual						~BlockBufferCacheKernel();

	virtual	bool				Lock();
	virtual	void				Unlock();

private:
			mutex				fLock;
};


#endif	// BLOCK_BUFFER_CACHE_KERNEL_H
