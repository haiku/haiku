/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/PoolBuffer.h>

#include <stdlib.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


PoolBuffer::PoolBuffer(size_t size)
	:
	fOwner(NULL),
	fBuffer(malloc(size)),
	fSize(size),
	fCached(false)
{
}


PoolBuffer::~PoolBuffer()
{
	free(fBuffer);
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
