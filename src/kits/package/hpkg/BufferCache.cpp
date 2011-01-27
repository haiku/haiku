/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <package/hpkg/BufferCache.h>

#include <stdlib.h>


namespace BPackageKit {

namespace BHaikuPackage {

namespace BPrivate {


// #pragma mark - CachedBuffer


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


// #pragma mark - BufferCache


BufferCache::~BufferCache()
{
}


}	// namespace BPrivate

}	// namespace BHaikuPackage

}	// namespace BPackageKit
