/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BLOCK_BUFFER_CACHE_H
#define BLOCK_BUFFER_CACHE_H


#include "BufferCache.h"


class BlockBufferCache : public BufferCache {
public:
								BlockBufferCache(size_t blockSize,
									uint32 maxCachedBlocks);
	virtual						~BlockBufferCache();

	virtual	status_t			Init();

	virtual	CachedBuffer*		GetBuffer(size_t size,
									CachedBuffer** owner = NULL,
									bool* _newBuffer = NULL);
	virtual	void				PutBufferAndCache(CachedBuffer** owner);
	virtual	void				PutBuffer(CachedBuffer** owner);

	virtual	bool				Lock() = 0;
	virtual	void				Unlock() = 0;

private:
			typedef DoublyLinkedList<CachedBuffer> BufferList;

private:
			CachedBuffer*		_AllocateBuffer(size_t size,
									CachedBuffer** owner, bool* _newBuffer);
									// object must not be locked

private:
			size_t				fBlockSize;
			uint32				fMaxCachedBlocks;
			uint32				fAllocatedBlocks;
			BufferList			fUnusedBuffers;
			BufferList			fCachedBuffers;
};


class BlockBufferCacheNoLock : public BlockBufferCache {
public:
								BlockBufferCacheNoLock(size_t blockSize,
									uint32 maxCachedBlocks);
	virtual						~BlockBufferCacheNoLock();

	virtual	bool				Lock();
	virtual	void				Unlock();
};


#endif	// BLOCK_BUFFER_CACHE_H
