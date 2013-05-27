/*
 * Copyright 2004-2013, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */


#include "Link.h"


namespace BFS {


Link::Link(Volume &volume, block_run run)
	: File(volume, run)
{
}


Link::Link(Volume &volume, off_t id)
	: File(volume, id)
{
}


Link::Link(const Stream &stream)
	: File(stream)
{
}


status_t
Link::InitCheck()
{
	return fStream.InitCheck();
}


status_t
Link::ReadLink(char *buffer, size_t bufferSize)
{
	return fStream.ReadLink(buffer, bufferSize);
}


ssize_t
Link::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	return B_NOT_ALLOWED;
}


ssize_t
Link::WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize)
{
	return B_NOT_ALLOWED;
}


int32
Link::Type() const
{
	return S_IFLNK;
}


}	// namespace BFS
