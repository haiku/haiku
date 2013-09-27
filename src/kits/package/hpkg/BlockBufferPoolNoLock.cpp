/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2011, Oliver Tappe <zooey@hirschkaefer.de>
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/BlockBufferPoolNoLock.h>


namespace BPackageKit {

namespace BHPKG {


BBlockBufferPoolNoLock::BBlockBufferPoolNoLock(size_t blockSize,
	uint32 maxCachedBlocks)
	:
	BBlockBufferPool(blockSize, maxCachedBlocks)
{
}


BBlockBufferPoolNoLock::~BBlockBufferPoolNoLock()
{
}


bool
BBlockBufferPoolNoLock::Lock()
{
	return true;
}


void
BBlockBufferPoolNoLock::Unlock()
{
}


}	// namespace BHPKG

}	// namespace BPackageKit
