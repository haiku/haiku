/*
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/BlockBufferCache.h>

#include <new>

#include <package/hpkg/BlockBufferCacheImpl.h>


namespace BPackageKit {

namespace BHPKG {


BBlockBufferCache::BBlockBufferCache(size_t blockSize, uint32 maxCachedBlocks)
	:
	fImpl(new (std::nothrow) BlockBufferCacheImpl(blockSize, maxCachedBlocks,
		this))
{
}


BBlockBufferCache::~BBlockBufferCache()
{
	delete fImpl;
}


status_t
BBlockBufferCache::Init()
{
	if (fImpl == NULL)
		return B_NO_MEMORY;

	return fImpl->Init();
}


CachedBuffer*
BBlockBufferCache::GetBuffer(size_t size, CachedBuffer** owner,
	bool* _newBuffer)
{
	if (fImpl == NULL)
		return NULL;

	return fImpl->GetBuffer(size, owner, _newBuffer);
}


void
BBlockBufferCache::PutBufferAndCache(CachedBuffer** owner)
{
	if (fImpl != NULL)
		fImpl->PutBufferAndCache(owner);
}


void
BBlockBufferCache::PutBuffer(CachedBuffer** owner)
{
	if (fImpl != NULL)
		fImpl->PutBuffer(owner);
}


}	// namespace BHPKG

}	// namespace BPackageKit
