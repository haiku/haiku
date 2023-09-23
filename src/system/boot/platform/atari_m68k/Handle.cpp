/*
 * Copyright 2008-2010, François Revol, revol@free.fr. All rights reserved.
 * Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <SupportDefs.h>
#include <platform/openfirmware/openfirmware.h>
#include <util/kernel_cpp.h>

#include "Handle.h"
#include "toscalls.h"

/*
 * (X)BIOS supports char and block devices with a separate namespace
 * for char devs handle is {DEV_PRINTER, ... DEV_CONSOLE, ...}
 * for block devs handle is either:
 * 		 - the partition number {0=A:, 2=C:...} in logical mode.
 * 		 - the drive number {0=A:, 1=B:, 2=ACSI-0, 10=SCSI-0, ...} 
 *			in phys mode (RW_NOTRANSLATE).
 * BlockHandle is in devices.cpp
 * 
 * XXX: handle network devices ? not sure how TOS net extensions do this
 * not sure it'll ever be supported anyway.
 * XXX: BIOSDrive/BIOSHandle : public BlockHandle ?
 */

Handle::Handle(int handle)
	:
	fHandle((int16)handle)
{
}


Handle::Handle(void)
	:
	fHandle(DEV_CONSOLE)
{
}


Handle::~Handle()
{
}


void
Handle::SetHandle(int handle)
{
	fHandle = (int16)handle;
}


ssize_t
Handle::ReadAt(void *cookie, off_t pos, void *buffer, size_t bufferSize)
{
	return B_ERROR;
}


ssize_t
Handle::WriteAt(void *cookie, off_t pos, const void *buffer, size_t bufferSize)
{
	return B_ERROR;
}


off_t 
Handle::Size() const
{
	// ToDo: fix this!
	return 1024LL * 1024 * 1024 * 1024;
		// 1024 GB
}
