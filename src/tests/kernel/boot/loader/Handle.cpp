/*
 * Copyright 2003-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Handle.h"

#include <SupportDefs.h>
#include <boot/platform.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>


#ifndef HAVE_READ_POS
#	define read_pos(fd, pos, buffer, size) pread(fd, buffer, size, pos)
#	define write_pos(fd, pos, buffer, size) pwrite(fd, buffer, size, pos)
#endif


Handle::Handle(int handle, bool takeOwnership)
	:
	fHandle(handle),
	fOwnHandle(takeOwnership),
	fPath(NULL)
{
}


Handle::Handle(const char *path)
	:
	fOwnHandle(true),
	fPath(NULL)
{
	fHandle = open(path, O_RDONLY);
	if (fHandle < B_OK) {
		fHandle = errno;
		return;
	}

	fPath = strdup(path);
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

	free(fPath);
}


status_t 
Handle::InitCheck()
{
	return fHandle < B_OK ? fHandle : B_OK;
}


void
Handle::SetTo(int handle, bool takeOwnership)
{
	if (fHandle && fOwnHandle)
		close(fHandle);

	fHandle = handle;
	fOwnHandle = takeOwnership;
}


ssize_t
Handle::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	//printf("Handle::ReadAt(pos = %Ld, buffer = %p, size = %lu)\n", pos, buffer, bufferSize);
	return read_pos(fHandle, pos, buffer, bufferSize);
}


ssize_t
Handle::WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize)
{
	return write_pos(fHandle, pos, buffer, bufferSize);
}


status_t 
Handle::GetName(char *nameBuffer, size_t bufferSize) const
{
	if (fPath == NULL)
		return B_ERROR;

	strncpy(nameBuffer, fPath, bufferSize - 1);
	nameBuffer[bufferSize - 1] = '\0';
	return B_OK;
}


off_t 
Handle::Size() const
{
	struct stat stat;
	if (fstat(fHandle, &stat) == B_OK) {
		if (stat.st_size == 0) {
			// ToDo: fix this!
			return 1024LL * 1024 * 1024 * 1024;
				// 1024 GB
		}
		return stat.st_size;
	}

	return 0LL;
}

