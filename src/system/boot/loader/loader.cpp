/*
 * Copyright 2003-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "loader.h"
#include "elf.h"
#include "RootFileSystem.h"

#include <directories.h>
#include <OS.h>
#include <util/list.h>
#include <boot/stage2.h>
#include <boot/vfs.h>
#include <boot/platform.h>
#include <boot/stdio.h>
#include <boot/partitions.h>

#include <unistd.h>
#include <string.h>

#ifndef BOOT_ARCH
#	error BOOT_ARCH has to be defined to differentiate the kernel per platform
#endif

#define KERNEL_IMAGE	"kernel_" BOOT_ARCH
#define KERNEL_PATH		"system/" KERNEL_IMAGE


static const char *sPaths[] = {
	kVolumeLocalSystemKernelAddonsDirectory,
	kVolumeLocalCommonNonpackagedKernelAddonsDirectory,
	kVolumeLocalCommonKernelAddonsDirectory,
	kVolumeLocalUserNonpackagedKernelAddonsDirectory,
	kVolumeLocalUserKernelAddonsDirectory,
	NULL
};


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

	dprintf("load kernel...\n");

	elf_init();
	status_t status = elf_load_image(fd, &gKernelArgs.kernel_image);

	close(fd);

	if (status < B_OK) {
		dprintf("loading kernel failed: %lx!\n", status);
		return status;
	}

	status = elf_relocate_image(&gKernelArgs.kernel_image);
	if (status < B_OK) {
		dprintf("relocating kernel failed: %lx!\n", status);
		return status;
	}

	gKernelArgs.kernel_image.name = kernel_args_strdup(KERNEL_IMAGE);

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


/** Loads a module by module name. This basically works in the same
 *	way as the kernel module loader; it will cut off the last part
 *	of the module name until it could find a module and loads it.
 *	It tests both, kernel and user module directories.
 */

static status_t
load_module(Directory *volume, const char *name)
{
	char moduleName[B_FILE_NAME_LENGTH];
	if (strlcpy(moduleName, name, sizeof(moduleName)) > sizeof(moduleName))
		return B_NAME_TOO_LONG;

	for (int32 i = 0; sPaths[i]; i++) {
		// get base path
		int baseFD = open_from(volume, sPaths[i], O_RDONLY);
		if (baseFD < B_OK)
			continue;

		Directory *base = (Directory *)get_node_from(baseFD);
		if (base == NULL) {
			close(baseFD);
			continue;
		}

		while (true) {
			int fd = open_from(base, moduleName, O_RDONLY);
			if (fd >= B_OK) {
				struct stat stat;
				if (fstat(fd, &stat) != 0 || !S_ISREG(stat.st_mode))
					return B_BAD_VALUE;

				status_t status = elf_load_image(base, moduleName);

				close(fd);
				close(baseFD);
				return status;
			}

			// cut off last name element (or stop trying if there are no more)

			char *last = strrchr(moduleName, '/');
			if (last != NULL)
				last[0] = '\0';
			else
				break;
		}

		close(baseFD);
	}

	return B_OK;
}


status_t
load_modules(stage2_args *args, Directory *volume)
{
	int32 failed = 0;

	// ToDo: this should be mostly replaced by a hardware oriented detection mechanism

	int32 i = 0;
	for (; sPaths[i]; i++) {
		char path[B_FILE_NAME_LENGTH];
		snprintf(path, sizeof(path), "%s/boot", sPaths[i]);

		if (load_modules_from(volume, path) != B_OK)
			failed++;
	}

	if (failed == i) {
		// couldn't load any boot modules
		// fall back to load all modules (currently needed by the boot floppy)
		const char *paths[] = { "bus_managers", "busses/ide", "busses/scsi",
			"generic", "partitioning_systems", "drivers/bin", NULL};

		for (int32 i = 0; paths[i]; i++) {
			char path[B_FILE_NAME_LENGTH];
			snprintf(path, sizeof(path), "%s/%s", sPaths[0], paths[i]);
			load_modules_from(volume, path);
		}
	}

	// and now load all partitioning and file system modules
	// needed to identify the boot volume

	if (!gKernelArgs.boot_volume.GetBool(BOOT_VOLUME_BOOTED_FROM_IMAGE,
			false)) {
		// iterate over the mounted volumes and load their file system
		Partition *partition;
		if (gRoot->GetPartitionFor(volume, &partition) == B_OK) {
			while (partition != NULL) {
				load_module(volume, partition->ModuleName());
				partition = partition->Parent();
			}
		}
	} else {
		// The boot image should only contain the file system
		// needed to boot the system, so we just load it.
		// ToDo: this is separate from the fall back from above
		//	as this piece will survive a more intelligent module
		//	loading approach...
		char path[B_FILE_NAME_LENGTH];
		snprintf(path, sizeof(path), "%s/%s", sPaths[0], "file_systems");
		load_modules_from(volume, path);
	}

	return B_OK;
}

