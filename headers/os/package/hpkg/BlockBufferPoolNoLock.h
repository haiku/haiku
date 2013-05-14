/*
 * Copyright 2009,2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PACKAGE__BLOCK_BUFFER_POOL_NO_LOCK_H
#define _PACKAGE__BLOCK_BUFFER_POOL_NO_LOCK_H


#include <package/hpkg/BlockBufferPool.h>


namespace BPackageKit {

namespace BHPKG {


class BBlockBufferPoolNoLock : public BHPKG::BBlockBufferPool {
public:
								BBlockBufferPoolNoLock(size_t blockSize,
									uint32 maxCachedBlocks);
	virtual						~BBlockBufferPoolNoLock();

	virtual	bool				Lock();
	virtual	void				Unlock();
};


}	// namespace BHPKG

}	// namespace BPackageKit


#endif	// _PACKAGE__BLOCK_BUFFER_POOL_NO_LOCK_H
