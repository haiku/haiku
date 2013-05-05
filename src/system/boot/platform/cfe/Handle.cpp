/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "Handle.h"

#include <SupportDefs.h>

#include <boot/platform/cfe/cfe.h>
#include <util/kernel_cpp.h>


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
		cfe_close(fHandle);
}


void
Handle::SetHandle(int handle, bool takeOwnership)
{
	if (fHandle && fOwnHandle)
		cfe_close(fHandle);

	fHandle = handle;
	fOwnHandle = takeOwnership;
}


ssize_t
Handle::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	int32 err;
	if (pos == -1)
		pos = 0;//XXX
	err = cfe_readblk(fHandle, pos, buffer, bufferSize);

	if (err < 0)
		return cfe_error(err);

	return err;
}


ssize_t
Handle::WriteAt(void *cookie, off_t pos, const void *buffer,
	size_t bufferSize)
{
	int32 err;
	if (pos == -1)
		pos = 0;//XXX
	err = cfe_writeblk(fHandle, pos, buffer, bufferSize);

	if (err < 0)
		return cfe_error(err);

	return err;
}


off_t 
Handle::Size() const
{
	// ToDo: fix this!
	return 1024LL * 1024 * 1024 * 1024;
		// 1024 GB
}

