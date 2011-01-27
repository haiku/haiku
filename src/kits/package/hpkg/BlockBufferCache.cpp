/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/BlockBufferCache.h>

#include <algorithm>
#include <new>

#include <AutoLocker.h>


namespace BPackageKit {

namespace BHaikuPackage {

namespace BPrivate {


// #pragma mark - BlockBufferCache


BlockBufferCache::BlockBufferCache(size_t blockSize, uint32 maxCachedBlocks)
	:
	fBlockSize(blockSize),
	fMaxCachedBlocks(maxCachedBlocks),
	fAllocatedBlocks(0)
{
}


BlockBufferCache::~BlockBufferCache()
{
	// delete all cached blocks
	while (CachedBuffer* block = fCachedBuffers.RemoveHead())
		delete block;

	while (CachedBuffer* block = fUnusedBuffers.RemoveHead())
		delete block;
}


status_t
BlockBufferCache::Init()
{
	return B_OK;
}


CachedBuffer*
BlockBufferCache::GetBuffer(size_t size, CachedBuffer** owner, bool* _newBuffer)
{
	// for sizes greater than the block size, we always allocate a new buffer
	if (size > fBlockSize)
		return _AllocateBuffer(size, owner, _newBuffer);

	AutoLocker<BlockBufferCache> locker(this);

	// if an owner is given and the buffer is still cached, return it
	if (owner != NULL && *owner != NULL) {
		CachedBuffer* buffer = *owner;
		fCachedBuffers.Remove(buffer);

		if (_newBuffer != NULL)
			*_newBuffer = false;
		return buffer;
	}

	// we need a new buffer -- try unused ones first
	CachedBuffer* buffer = fUnusedBuffers.RemoveHead();
	if (buffer != NULL) {
		buffer->SetOwner(owner);

		if (owner != NULL)
			*owner = buffer;
		if (_newBuffer != NULL)
			*_newBuffer = true;
		return buffer;
	}

	// if we have already hit the max block limit, steal a cached block
	if (fAllocatedBlocks >= fMaxCachedBlocks) {
		buffer = fCachedBuffers.RemoveHead();
		if (buffer != NULL) {
			buffer->SetCached(false);
			*buffer->Owner() = NULL;
			buffer->SetOwner(owner);

			if (owner != NULL)
				*owner = buffer;
			if (_newBuffer != NULL)
				*_newBuffer = true;
			return buffer;
		}
	}

	// allocate a new buffer
	locker.Unlock();
	return _AllocateBuffer(size, owner, _newBuffer);
}


void
BlockBufferCache::PutBufferAndCache(CachedBuffer** owner)
{
	CachedBuffer* buffer = *owner;

	// always delete buffers with non-standard size
	if (buffer->Size() != fBlockSize) {
		*owner = NULL;
		delete buffer;
		return;
	}

	AutoLocker<BlockBufferCache> locker(this);

	// queue the cached buffer
	buffer->SetOwner(owner);
	fCachedBuffers.Add(buffer);
	buffer->SetCached(true);

	if (fAllocatedBlocks > fMaxCachedBlocks) {
		// We have exceeded the limit -- we need to free a buffer.
		CachedBuffer* otherBuffer = fUnusedBuffers.RemoveHead();
		if (otherBuffer == NULL) {
			otherBuffer = fCachedBuffers.RemoveHead();
			*otherBuffer->Owner() = NULL;
			otherBuffer->SetCached(false);
		}

		delete otherBuffer;
	}
}


void
BlockBufferCache::PutBuffer(CachedBuffer** owner)
{
	AutoLocker<BlockBufferCache> locker(this);

	CachedBuffer* buffer = *owner;

	if (buffer == NULL)
		return;

	if (buffer->IsCached()) {
		fCachedBuffers.Remove(buffer);
		buffer->SetCached(false);
	}

	buffer->SetOwner(NULL);
	*owner = NULL;

	if (buffer->Size() == fBlockSize && fAllocatedBlocks < fMaxCachedBlocks)
		fUnusedBuffers.Add(buffer);
	else
		delete buffer;
}


CachedBuffer*
BlockBufferCache::_AllocateBuffer(size_t size, CachedBuffer** owner,
	bool* _newBuffer)
{
	CachedBuffer* buffer = new(std::nothrow) CachedBuffer(
		std::max(size, fBlockSize));
	if (buffer == NULL || buffer->Buffer() == NULL) {
		delete buffer;
		return NULL;
	}

	buffer->SetOwner(owner);

	if (_newBuffer != NULL)
		*_newBuffer = true;

	AutoLocker<BlockBufferCache> locker(this);
	fAllocatedBlocks++;

	if (owner != NULL)
		*owner = buffer;

	return buffer;
}


// #pragma mark - BlockBufferCacheNoLock


BlockBufferCacheNoLock::BlockBufferCacheNoLock(size_t blockSize,
	uint32 maxCachedBlocks)
	:
	BlockBufferCache(blockSize, maxCachedBlocks)
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


}	// namespace BPrivate

}	// namespace BHaikuPackage

}	// namespace BPackageKit
