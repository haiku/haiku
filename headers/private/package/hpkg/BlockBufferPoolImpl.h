/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__PRIVATE__BLOCK_BUFFER_POOL_H_
#define _PACKAGE__HPKG__PRIVATE__BLOCK_BUFFER_POOL_H_


#include <SupportDefs.h>

#include <util/DoublyLinkedList.h>

#include <package/hpkg/BufferPool.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


class PoolBuffer;


class BlockBufferPoolImpl : public BBufferPool {
public:
								BlockBufferPoolImpl(size_t blockSize,
									uint32 maxCachedBlocks,
									BBufferPoolLockable* lockable);
								~BlockBufferPoolImpl();

			status_t			Init();

			PoolBuffer*			GetBuffer(size_t size,
									PoolBuffer** owner = NULL,
									bool* _newBuffer = NULL);
			void				PutBufferAndCache(PoolBuffer** owner);
			void				PutBuffer(PoolBuffer** owner);

private:
			typedef DoublyLinkedList<PoolBuffer> BufferList;

private:
			PoolBuffer*			_AllocateBuffer(size_t size,
									PoolBuffer** owner, bool* _newBuffer);
									// object must not be locked

private:
			size_t				fBlockSize;
			uint32				fMaxCachedBlocks;
			uint32				fAllocatedBlocks;
			BufferList			fUnusedBuffers;
			BufferList			fCachedBuffers;
			BBufferPoolLockable*	fLockable;
};


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__PRIVATE__BLOCK_BUFFER_POOL_H_
