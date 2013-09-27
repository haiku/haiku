/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BLOCK_BUFFER_POOL_KERNEL_H
#define BLOCK_BUFFER_POOL_KERNEL_H


#include <lock.h>

#include <package/hpkg/BlockBufferPoolImpl.h>
#include <package/hpkg/BufferPool.h>


using BPackageKit::BHPKG::BBufferPoolLockable;
using BPackageKit::BHPKG::BPrivate::BlockBufferPoolImpl;


class BlockBufferPoolKernel : public BlockBufferPoolImpl, BBufferPoolLockable {
public:
								BlockBufferPoolKernel(size_t blockSize,
									uint32 maxCachedBlocks);
	virtual						~BlockBufferPoolKernel();

	virtual	bool				Lock();
	virtual	void				Unlock();

private:
			mutex				fLock;
};


#endif	// BLOCK_BUFFER_POOL_KERNEL_H
