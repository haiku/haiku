/*
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <SupportDefs.h>
#include <platform/openfirmware/openfirmware.h>
#include <util/kernel_cpp.h>

#include "Handle.h"
#include "toscalls."

Handle::Handle(int handle, bool takeOwnership)
	:
	fHandle((int16)handle),
	fOwnHandle(takeOwnership)
{
}


Handle::Handle(void)
	:
	fHandle(DEV_CONSOLE)
{
}


Handle::~Handle()
{
	//if (fOwnHandle)
	//	of_close(fHandle);
}


void
Handle::SetHandle(int handle, bool takeOwnership)
{
	//if (fHandle && fOwnHandle)
	//	of_close(fHandle);

	fHandle = (int16)handle;
	fOwnHandle = takeOwnership;
}


ssize_t
Handle::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	const char *string = (const char *)buffer;

	// can't seek
	for (i = 0; i < bufferSize; i++) {
		if (Bconstat(fHandle) == 0)
			return i;
		Bconin(fHandle, string[i]);
	}

	return bufferSize;
}


ssize_t
Handle::WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize)
{
	const char *string = (const char *)buffer;

	// can't seek
	for (i = 0; i < bufferSize; i++) {
		Bconout(fHandle, string[i]);
	}

	return bufferSize;
}


off_t 
Handle::Size() const
{
	// ToDo: fix this!
	return 1024LL * 1024 * 1024 * 1024;
		// 1024 GB
}

