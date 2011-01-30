/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/BlockBufferCacheImpl.h>

#include <algorithm>
#include <new>

#include <AutoLocker.h>

#include <package/hpkg/CachedBuffer.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


// #pragma mark - BlockBufferCacheImpl


BlockBufferCacheImpl::BlockBufferCacheImpl(size_t blockSize,
	uint32 maxCachedBlocks, BBufferCacheLockable* lockable)
	:
	fBlockSize(blockSize),
	fMaxCachedBlocks(maxCachedBlocks),
	fAllocatedBlocks(0),
	fLockable(lockable)
{
}


BlockBufferCacheImpl::~BlockBufferCacheImpl()
{
	// delete all cached blocks
	while (CachedBuffer* block = fCachedBuffers.RemoveHead())
		delete block;

	while (CachedBuffer* block = fUnusedBuffers.RemoveHead())
		delete block;
}


status_t
BlockBufferCacheImpl::Init()
{
	return B_OK;
}


CachedBuffer*
BlockBufferCacheImpl::GetBuffer(size_t size, CachedBuffer** owner, bool* _newBuffer)
{
	// for sizes greater than the block size, we always allocate a new buffer
	if (size > fBlockSize)
		return _AllocateBuffer(size, owner, _newBuffer);

	AutoLocker<BBufferCacheLockable> locker(fLockable);

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
BlockBufferCacheImpl::PutBufferAndCache(CachedBuffer** owner)
{
	CachedBuffer* buffer = *owner;

	// always delete buffers with non-standard size
	if (buffer->Size() != fBlockSize) {
		*owner = NULL;
		delete buffer;
		return;
	}

	AutoLocker<BBufferCacheLockable> locker(fLockable);

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
BlockBufferCacheImpl::PutBuffer(CachedBuffer** owner)
{
	AutoLocker<BBufferCacheLockable> locker(fLockable);

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
BlockBufferCacheImpl::_AllocateBuffer(size_t size, CachedBuffer** owner,
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

	AutoLocker<BBufferCacheLockable> locker(fLockable);
	fAllocatedBlocks++;

	if (owner != NULL)
		*owner = buffer;

	return buffer;
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
