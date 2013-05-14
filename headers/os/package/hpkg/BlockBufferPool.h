/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__HPKG__BLOCK_BUFFER_POOL_H_
#define _PACKAGE__HPKG__BLOCK_BUFFER_POOL_H_


#include <SupportDefs.h>

#include <package/hpkg/BufferPool.h>


namespace BPackageKit {

namespace BHPKG {


namespace BPrivate {
	class BlockBufferPoolImpl;
}
using BPrivate::BlockBufferPoolImpl;


class BBlockBufferPool : public BBufferPool, public BBufferPoolLockable {
public:
								BBlockBufferPool(size_t blockSize,
									uint32 maxCachedBlocks);
	virtual						~BBlockBufferPool();

	virtual	status_t			Init();

	virtual	PoolBuffer*			GetBuffer(size_t size,
									PoolBuffer** owner = NULL,
									bool* _newBuffer = NULL);
	virtual	void				PutBufferAndCache(PoolBuffer** owner);
	virtual	void				PutBuffer(PoolBuffer** owner);

private:
			BlockBufferPoolImpl*	fImpl;
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__HPKG__BLOCK_BUFFER_POOL_H_
