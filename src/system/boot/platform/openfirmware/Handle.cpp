/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "Handle.h"

#include <SupportDefs.h>

#include <platform/openfirmware/openfirmware.h>
#include <util/kernel_cpp.h>


Handle::Handle(intptr_t handle, bool takeOwnership)
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
		of_close(fHandle);
}


void
Handle::SetHandle(intptr_t handle, bool takeOwnership)
{
	if (fHandle && fOwnHandle)
		of_close(fHandle);

	fHandle = handle;
	fOwnHandle = takeOwnership;
}


ssize_t
Handle::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	if (pos == -1 || of_seek(fHandle, pos) != OF_FAILED)
		return of_read(fHandle, buffer, bufferSize);

	return B_ERROR;
}


ssize_t
Handle::WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize)
{
	if (pos == -1 || of_seek(fHandle, pos) != OF_FAILED)
		return of_write(fHandle, buffer, bufferSize);

	return B_ERROR;
}


off_t 
Handle::Size() const
{
	// ToDo: fix this!
	return 1024LL * 1024 * 1024 * 1024;
		// 1024 GB
}

