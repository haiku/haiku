/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/CachedBuffer.h>

#include <stdlib.h>


namespace BPackageKit {

namespace BHPKG {

namespace BPrivate {


CachedBuffer::CachedBuffer(size_t size)
	:
	fOwner(NULL),
	fBuffer(malloc(size)),
	fSize(size),
	fCached(false)
{
}


CachedBuffer::~CachedBuffer()
{
	free(fBuffer);
}


}	// namespace BPrivate

}	// namespace BHPKG

}	// namespace BPackageKit
