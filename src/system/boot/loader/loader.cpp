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

#define SYSTEM_DIRECTORY_PREFIX	"system/"
#define KERNEL_IMAGE	"kernel_" BOOT_ARCH
#define KERNEL_PATH		SYSTEM_DIRECTORY_PREFIX KERNEL_IMAGE

#ifdef ALTERNATE_BOOT_ARCH
# define ALTERNATE_KERNEL_IMAGE	"kernel_" ALTERNATE_BOOT_ARCH
# define ALTERNATE_KERNEL_PATH	"system/" ALTERNATE_KERNEL_IMAGE
#endif


static const char* const kSystemDirectoryPrefix = SYSTEM_DIRECTORY_PREFIX;

static const char *sKernelPaths[][2] = {
	{ KERNEL_PATH, KERNEL_IMAGE },
#ifdef ALTERNATE_BOOT_ARCH
	{ ALTERNATE_KERNEL_PATH, ALTERNATE_KERNEL_IMAGE },
#endif
	{ NULL, NULL },
};

static const char *sAddonPaths[] = {
	kVolumeLocalSystemKernelAddonsDirectory,
	kVolumeLocalCommonNonpackagedKernelAddonsDirectory,
	kVolumeLocalCommonKernelAddonsDirectory,
	kVolumeLocalUserNonpackagedKernelAddonsDirectory,
	kVolumeLocalUserKernelAddonsDirectory,
	NULL
};


static int
open_maybe_packaged(BootVolume& volume, const char* path, int openMode)
{
	if (strncmp(path, kSystemDirectoryPrefix, strlen(kSystemDirectoryPrefix))
			== 0) {
		path += strlen(kSystemDirectoryPrefix);
		return open_from(volume.SystemDirectory(), path, openMode);
	}

	return open_from(volume.RootDirectory(), path, openMode);
}


static int
find_kernel(BootVolume& volume, const char** name = NULL)
{
	for (int32 i = 0; sKernelPaths[i][0] != NULL; i++) {
		int fd = open_maybe_packaged(volume, sKernelPaths[i][0], O_RDONLY);
		if (fd >= 0) {
			if (name)
				*name = sKernelPaths[i][1];

			return fd;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


bool
is_bootable(Directory *volume)
{
	if (volume->IsEmpty())
		return false;

	BootVolume bootVolume;
	if (bootVolume.SetTo(volume) != B_OK)
		return false;

	// check for the existance of a kernel (for our platform)
	int fd = find_kernel(bootVolume);
	if (fd < 0)
		return false;

	close(fd);

	return true;
}


status_t
load_kernel(stage2_args* args, BootVolume& volume)
{
	const char *name;
	int fd = find_kernel(volume, &name);
	if (fd < B_OK)
		return fd;

	dprintf("load kernel %s...\n", name);

	elf_init();
	preloaded_image *image;
	status_t status = elf_load_image(fd, &image);

	close(fd);

	if (status < B_OK) {
		dprintf("loading kernel failed: %" B_PRIx32 "!\n", status);
		return status;
	}

	gKernelArgs.kernel_image = image;

	status = elf_relocate_image(gKernelArgs.kernel_image);
	if (status < B_OK) {
		dprintf("relocating kernel failed: %" B_PRIx32 "!\n", status);
		return status;
	}

	gKernelArgs.kernel_image->name = kernel_args_strdup(name);

	return B_OK;
}


static status_t
load_modules_from(BootVolume& volume, const char* path)
{
	// we don't have readdir() & co. (yet?)...

	int fd = open_maybe_packaged(volume, path, O_RDONLY);
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
				dprintf("Could not load \"%s\" error %" B_PRIx32 "\n", name, status);
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
load_module(BootVolume& volume, const char* name)
{
	char moduleName[B_FILE_NAME_LENGTH];
	if (strlcpy(moduleName, name, sizeof(moduleName)) > sizeof(moduleName))
		return B_NAME_TOO_LONG;

	for (int32 i = 0; sAddonPaths[i]; i++) {
		// get base path
		int baseFD = open_maybe_packaged(volume, sAddonPaths[i], O_RDONLY);
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
load_modules(stage2_args* args, BootVolume& volume)
{
	int32 failed = 0;

	// ToDo: this should be mostly replaced by a hardware oriented detection mechanism

	int32 i = 0;
	for (; sAddonPaths[i]; i++) {
		char path[B_FILE_NAME_LENGTH];
		snprintf(path, sizeof(path), "%s/boot", sAddonPaths[i]);

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
			snprintf(path, sizeof(path), "%s/%s", sAddonPaths[0], paths[i]);
			load_modules_from(volume, path);
		}
	}

	// and now load all partitioning and file system modules
	// needed to identify the boot volume

	if (!gBootVolume.GetBool(BOOT_VOLUME_BOOTED_FROM_IMAGE, false)
		&& gBootVolume.GetInt32(BOOT_METHOD, BOOT_METHOD_DEFAULT)
			!= BOOT_METHOD_CD) {
		// iterate over the mounted volumes and load their file system
		Partition *partition;
		if (gRoot->GetPartitionFor(volume.RootDirectory(), &partition)
				== B_OK) {
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
		snprintf(path, sizeof(path), "%s/%s", sAddonPaths[0], "file_systems");
		load_modules_from(volume, path);
	}

	return B_OK;
}

