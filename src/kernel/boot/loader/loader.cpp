/*
** Copyright 2003, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "loader.h"

#include <OS.h>
#include <boot/stage2.h>
#include <boot/vfs.h>
#include <boot/platform.h>
#include <boot/stdio.h>

#include <unistd.h>

#ifndef BOOT_ARCH
#	error BOOT_ARCH has to be defined to differentiate the kernel per platform
#endif

#define KERNEL_IMAGE	"kernel_" BOOT_ARCH


bool
is_bootable(Directory *volume)
{
	if (volume->IsEmpty())
		return false;

	// check for the existance of a kernel (for our platform)
	int fd = open_from(volume, "beos/system/" KERNEL_IMAGE, O_RDONLY);
	if (fd < B_OK)
		return false;

	close(fd);

	return true;
}


status_t 
load_kernel(stage2_args *args, Directory *volume)
{
	if (!is_bootable(volume))
		return B_ENTRY_NOT_FOUND;

	puts("load kernel...");

	// ToDo: really load that thing :)

	void *cookie;
	if (volume->Open(&cookie, O_RDONLY) == B_OK) {
		char name[B_FILE_NAME_LENGTH];
		while (volume->GetNextEntry(cookie, name, sizeof(name)) == B_OK)
			printf("\t%s\n", name);

		volume->Close(cookie);
	}
	return B_OK;
}


status_t 
load_modules(stage2_args *args, Directory *volume)
{
	return B_OK;
}


void
start_kernel()
{
	// shouldn't return...
}

