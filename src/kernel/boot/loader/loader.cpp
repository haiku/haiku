/*
** Copyright 2003-2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "loader.h"
#include "elf.h"

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
#define KERNEL_PATH		"beos/system/" KERNEL_IMAGE


// temp. VFS API
extern Node *get_node_from(int fd);


bool
is_bootable(Directory *volume)
{
	if (volume->IsEmpty())
		return false;

	// check for the existance of a kernel (for our platform)
	int fd = open_from(volume, KERNEL_PATH, O_RDONLY);
	if (fd < B_OK)
		return false;

	close(fd);

	return true;
}


status_t 
load_kernel(stage2_args *args, Directory *volume)
{
	int fd = open_from(volume, KERNEL_PATH, O_RDONLY);
	if (fd < B_OK)
		return fd;

	puts("load kernel...");

	preloaded_image image;
	status_t status = elf_load_image(fd, &image);

	close(fd);

	if (status < B_OK) {
		printf("loading kernel failed: %ld!\n", status);
		return status;
	}

	puts("kernel loaded successfully");

/*	void *cookie;
	if (volume->Open(&cookie, O_RDONLY) == B_OK) {
		char name[B_FILE_NAME_LENGTH];
		while (volume->GetNextEntry(cookie, name, sizeof(name)) == B_OK)
			printf("\t%s\n", name);

		volume->Close(cookie);
	}*/
	return B_OK;
}


status_t 
load_modules(stage2_args *args, Directory *volume)
{
	// we don't have readdir() & co. yet...

	int fd = open_from(volume, "beos/system/add-ons/boot", O_RDONLY);
	if (fd < B_OK)
		return fd;

	Directory *modules = (Directory *)get_node_from(fd);
	if (modules == NULL)
		return B_ENTRY_NOT_FOUND;

	void *cookie;
	if (modules->Open(&cookie, O_RDONLY) == B_OK) {
		char name[B_FILE_NAME_LENGTH];
		while (modules->GetNextEntry(cookie, name, sizeof(name)) == B_OK)
			printf("\t%s\n", name);

		modules->Close(cookie);
	}

	return B_OK;
}


void
start_kernel()
{
	// shouldn't return...
	panic("starting the kernel is not yet implemented :)");
}

