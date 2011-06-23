/*
 * Copyright 2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "EmptyAttributeDirectoryCookie.h"


status_t
EmptyAttributeDirectoryCookie::Close()
{
	return B_OK;
}


status_t
EmptyAttributeDirectoryCookie::Read(dev_t volumeID, ino_t nodeID,
	struct dirent* buffer, size_t bufferSize, uint32* _count)
{
	*_count = 0;
	return B_OK;
}


status_t
EmptyAttributeDirectoryCookie::Rewind()
{
	return B_OK;
}
