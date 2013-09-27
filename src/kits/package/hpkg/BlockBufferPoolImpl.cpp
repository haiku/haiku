/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/BlockBufferPoolImpl.h>

#include <algorithm>
#include <new>

#include <AutoLocker.h>

#include <package/hpkg/PoolBuffer.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


// #pragma mark - BlockBufferPoolImpl


BlockBufferPoolImpl::BlockBufferPoolImpl(size_t blockSize,
	uint32 maxCachedBlocks, BBufferPoolLockable* lockable)
	:
	fBlockSize(blockSize),
	fMaxCachedBlocks(maxCachedBlocks),
	fAllocatedBlocks(0),
	fLockable(lockable)
{
}


BlockBufferPoolImpl::~BlockBufferPoolImpl()
{
	// delete all cached blocks
	while (PoolBuffer* block = fCachedBuffers.RemoveHead())
		delete block;

	while (PoolBuffer* block = fUnusedBuffers.RemoveHead())
		delete block;
}


status_t
BlockBufferPoolImpl::Init()
{
	return B_OK;
}


PoolBuffer*
BlockBufferPoolImpl::GetBuffer(size_t size, PoolBuffer** owner, bool* _newBuffer)
{
	// for sizes greater than the block size, we always allocate a new buffer
	if (size > fBlockSize)
		return _AllocateBuffer(size, owner, _newBuffer);

	AutoLocker<BBufferPoolLockable> locker(fLockable);

	// if an owner is given and the buffer is still cached, return it
	if (owner != NULL && *owner != NULL) {
		PoolBuffer* buffer = *owner;
		fCachedBuffers.Remove(buffer);

		if (_newBuffer != NULL)
			*_newBuffer = false;
		return buffer;
	}

	// we need a new buffer -- try unused ones first
	PoolBuffer* buffer = fUnusedBuffers.RemoveHead();
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
BlockBufferPoolImpl::PutBufferAndCache(PoolBuffer** owner)
{
	PoolBuffer* buffer = *owner;

	// always delete buffers with non-standard size
	if (buffer->Size() != fBlockSize) {
		*owner = NULL;
		delete buffer;
		return;
	}

	AutoLocker<BBufferPoolLockable> locker(fLockable);

	// queue the cached buffer
	buffer->SetOwner(owner);
	fCachedBuffers.Add(buffer);
	buffer->SetCached(true);

	if (fAllocatedBlocks > fMaxCachedBlocks) {
		// We have exceeded the limit -- we need to free a buffer.
		PoolBuffer* otherBuffer = fUnusedBuffers.RemoveHead();
		if (otherBuffer == NULL) {
			otherBuffer = fCachedBuffers.RemoveHead();
			*otherBuffer->Owner() = NULL;
			otherBuffer->SetCached(false);
		}

		delete otherBuffer;
	}
}


void
BlockBufferPoolImpl::PutBuffer(PoolBuffer** owner)
{
	AutoLocker<BBufferPoolLockable> locker(fLockable);

	PoolBuffer* buffer = *owner;

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


PoolBuffer*
BlockBufferPoolImpl::_AllocateBuffer(size_t size, PoolBuffer** owner,
	bool* _newBuffer)
{
	PoolBuffer* buffer = new(std::nothrow) PoolBuffer(
		std::max(size, fBlockSize));
	if (buffer == NULL || buffer->Buffer() == NULL) {
		delete buffer;
		return NULL;
	}

	buffer->SetOwner(owner);

	if (_newBuffer != NULL)
		*_newBuffer = true;

	AutoLocker<BBufferPoolLockable> locker(fLockable);
	fAllocatedBlocks++;

	if (owner != NULL)
		*owner = buffer;

	return buffer;
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
