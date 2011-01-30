/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__BLOCK_BUFFER_CACHE_H_
#define _PACKAGE__HPKG__PRIVATE__BLOCK_BUFFER_CACHE_H_


#include <SupportDefs.h>

#include <util/DoublyLinkedList.h>

#include <package/hpkg/BufferCache.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


class CachedBuffer;


class BlockBufferCacheImpl : public BBufferCache {
public:
								BlockBufferCacheImpl(size_t blockSize,
									uint32 maxCachedBlocks,
									BBufferCacheLockable* lockable);
								~BlockBufferCacheImpl();

			status_t			Init();

			CachedBuffer*		GetBuffer(size_t size,
									CachedBuffer** owner = NULL,
									bool* _newBuffer = NULL);
			void				PutBufferAndCache(CachedBuffer** owner);
			void				PutBuffer(CachedBuffer** owner);

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
			BBufferCacheLockable*	fLockable;
};


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__BLOCK_BUFFER_CACHE_H_
