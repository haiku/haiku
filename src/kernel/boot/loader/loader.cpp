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
#include <string.h>

#ifndef BOOT_ARCH
#	error BOOT_ARCH has to be defined to differentiate the kernel per platform
#endif

#define KERNEL_IMAGE	"kernel_" BOOT_ARCH
#define KERNEL_PATH		"beos/system/" KERNEL_IMAGE


// temp. VFS API
extern Node *get_node_from(int fd);

addr_t gKernelEntry;


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

	// init kernel args with loaded image data
	gKernelArgs.kernel_seg0_addr.start = image.text_region.start;
	gKernelArgs.kernel_seg0_addr.size = image.text_region.size;
	gKernelArgs.kernel_seg1_addr.start = image.data_region.start;
	gKernelArgs.kernel_seg1_addr.size = image.data_region.size;
	gKernelArgs.kernel_dynamic_section_addr = image.dynamic_section;

	gKernelEntry = image.elf_header.e_entry;

	return B_OK;
}


static status_t
load_modules_from(Directory *volume, const char *path)
{
	// we don't have readdir() & co. (yet?)...

	int fd = open_from(volume, path, O_RDONLY);
	if (fd < B_OK)
		return fd;

	Directory *modules = (Directory *)get_node_from(fd);
	if (modules == NULL)
		return B_ENTRY_NOT_FOUND;

	void *cookie;
	if (modules->Open(&cookie, O_RDONLY) == B_OK) {
		char name[B_FILE_NAME_LENGTH];
		while (modules->GetNextEntry(cookie, name, sizeof(name)) == B_OK) {
			if (!strcmp(name, ".") || !strcmp(name, ".."))
				continue;

			status_t status = elf_load_image(modules, name);
			if (status != B_OK)
				dprintf("Could not load \"%s\" error %ld\n", name, status);
		}

		modules->Close(cookie);
	}

	return B_OK;
}


status_t 
load_modules(stage2_args *args, Directory *volume)
{
	const char *paths[] = {
		"beos/system/add-ons/kernel/boot",
		"home/config/add-ons/kernel/boot",
		NULL
	};
	
	for (int32 i = 0; paths[i]; i++) {
		load_modules_from(volume, paths[i]);
	}

	return B_OK;
}

