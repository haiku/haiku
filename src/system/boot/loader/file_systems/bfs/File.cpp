/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "File.h"

#include <util/kernel_cpp.h>


namespace BFS {

File::File(Volume &volume, block_run run)
	:
	fStream(volume, run)
{
}


File::File(Volume &volume, off_t id)
	:
	fStream(volume, id)
{
}


File::File(const Stream &stream)
	:
	fStream(stream)
{
}


File::~File()
{
}


status_t 
File::InitCheck()
{
	return fStream.InitCheck();
}


ssize_t 
File::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	status_t status = fStream.ReadAt(pos, (uint8 *)buffer, &bufferSize);
	if (status < B_OK)
		return status;

	return bufferSize;
}


ssize_t 
File::WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize)
{
	return EROFS;
}


status_t
File::GetName(char *nameBuffer, size_t bufferSize) const
{
	return fStream.GetName(nameBuffer, bufferSize);
}


int32
File::Type() const
{
	return S_IFREG;
}


off_t
File::Size() const
{
	return fStream.Size();
}


ino_t
File::Inode() const
{
	return fStream.ID();
}

}	// namespace BFS
