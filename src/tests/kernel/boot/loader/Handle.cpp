/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "Handle.h"

#include <SupportDefs.h>
#include <boot/platform.h>

#include <stdio.h>
#include <unistd.h>


#ifndef HAVE_READ_POS
#	define read_pos(fd, pos, buffer, size) pread(fd, buffer, size, pos)
#	define write_pos(fd, pos, buffer, size) pwrite(fd, buffer, size, pos)
#endif


Handle::Handle(int handle, bool takeOwnership)
	:
	fHandle(handle),
	fOwnHandle(takeOwnership)
{
}


Handle::Handle(void)
	:
	fHandle(0)
{
}


Handle::~Handle()
{
	if (fOwnHandle)
		close(fHandle);
}


void
Handle::SetHandle(int handle, bool takeOwnership)
{
	if (fHandle && fOwnHandle)
		close(fHandle);

	fHandle = handle;
	fOwnHandle = takeOwnership;
}


ssize_t
Handle::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	return read_pos(fHandle, pos, buffer, bufferSize);
}


ssize_t
Handle::WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize)
{
	return write_pos(fHandle, pos, buffer, bufferSize);
}


off_t 
Handle::Size() const
{
	// ToDo: fix this!
	return 1024LL * 1024 * 1024 * 1024;
		// 1024 GB
}

