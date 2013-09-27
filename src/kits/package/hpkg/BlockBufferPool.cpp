/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/BlockBufferPool.h>

#include <new>

#include <package/hpkg/BlockBufferPoolImpl.h>


namespace BPackageKit {

namespace BHPKG {


BBlockBufferPool::BBlockBufferPool(size_t blockSize, uint32 maxCachedBlocks)
	:
	fImpl(new (std::nothrow) BlockBufferPoolImpl(blockSize, maxCachedBlocks,
		this))
{
}


BBlockBufferPool::~BBlockBufferPool()
{
	delete fImpl;
}


status_t
BBlockBufferPool::Init()
{
	if (fImpl == NULL)
		return B_NO_MEMORY;

	return fImpl->Init();
}


PoolBuffer*
BBlockBufferPool::GetBuffer(size_t size, PoolBuffer** owner,
	bool* _newBuffer)
{
	if (fImpl == NULL)
		return NULL;

	return fImpl->GetBuffer(size, owner, _newBuffer);
}


void
BBlockBufferPool::PutBufferAndCache(PoolBuffer** owner)
{
	if (fImpl != NULL)
		fImpl->PutBufferAndCache(owner);
}


void
BBlockBufferPool::PutBuffer(PoolBuffer** owner)
{
	if (fImpl != NULL)
		fImpl->PutBuffer(owner);
}


}	// namespace BHPKG

}	// namespace BPackageKit
