/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "File.h"

#include <util/kernel_cpp.h>

#include <sys/stat.h>
#include <unistd.h>


namespace FFS {

File::File(Volume &volume, int32 block)
{
}


File::~File()
{
}


status_t 
File::InitCheck()
{
	if (!fNode.IsFile())
		return B_BAD_TYPE;

	return fNode.ValidateCheckSum();
}


ssize_t 
File::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	return B_ERROR;
}


ssize_t 
File::WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize)
{
	return EROFS;
}


status_t 
File::GetName(char *nameBuffer, size_t bufferSize) const
{
	return fNode.GetName(nameBuffer, bufferSize);
}


int32 
File::Type() const
{
	return S_IFREG;
}


off_t 
File::Size() const
{
	return fNode.Size();
}

}	// namespace FFS
