/*
 * Copyright 2008, Rene Gollent, rene@gollent.com. All rights reserved.
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "load_driver_settings.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <OS.h>
#include <drivers/driver_settings.h>

#include <safemode_defs.h>

#include <boot/driver_settings.h>
#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <boot/platform.h>


static status_t
load_driver_settings_file(Directory* directory, const char* name)
{
	int fd = open_from(directory, name, O_RDONLY);
	if (fd < 0)
		return fd;

	struct stat stat;
	if (fstat(fd, &stat) < 0)
		return errno;

	char* buffer = (char*)kernel_args_malloc(stat.st_size + 1);
	if (buffer == NULL)
		return B_NO_MEMORY;

	if (read(fd, buffer, stat.st_size) != stat.st_size) {
		kernel_args_free(buffer);
		return B_IO_ERROR;
	}

	driver_settings_file* file = (driver_settings_file*)kernel_args_malloc(
		sizeof(driver_settings_file));
	if (file == NULL) {
		kernel_args_free(buffer);
		return B_NO_MEMORY;
	}

	buffer[stat.st_size] = '\0';
		// null terminate the buffer

	strlcpy(file->name, name, sizeof(file->name));
	file->buffer = buffer;
	file->size = stat.st_size;

	// add it to the list
	file->next = gKernelArgs.driver_settings;
	gKernelArgs.driver_settings = file;

	return B_OK;
}


static void
apply_boot_settings(void* kernelSettings, void* safemodeSettings)
{
#if B_HAIKU_PHYSICAL_BITS > 32
	if ((kernelSettings != NULL
			&& get_driver_boolean_parameter(kernelSettings,
				B_SAFEMODE_4_GB_MEMORY_LIMIT, false, false))
		|| (safemodeSettings != NULL
			&& get_driver_boolean_parameter(safemodeSettings,
				B_SAFEMODE_4_GB_MEMORY_LIMIT, false, false))) {
		ignore_physical_memory_ranges_beyond_4gb();
	}
#endif
}


//	#pragma mark -


status_t
load_driver_settings(stage2_args* /*args*/, Directory* volume)
{
	int fd = open_from(volume, "home/config/settings/kernel/drivers", O_RDONLY);
	if (fd < B_OK)
		return fd;

	Directory* settings = (Directory*)get_node_from(fd);
	if (settings == NULL)
		return B_ENTRY_NOT_FOUND;

	void* cookie;
	if (settings->Open(&cookie, O_RDONLY) == B_OK) {
		char name[B_FILE_NAME_LENGTH];
		while (settings->GetNextEntry(cookie, name, sizeof(name)) == B_OK) {
			if (!strcmp(name, ".") || !strcmp(name, ".."))
				continue;

			status_t status = load_driver_settings_file(settings, name);
			if (status != B_OK)
				dprintf("Could not load \"%s\" error %" B_PRIx32 "\n", name, status);
		}

		settings->Close(cookie);
	}

	return B_OK;
}


status_t
add_stage2_driver_settings(stage2_args* args)
{
	// TODO: split more intelligently
	for (const char** arg = args->arguments;
			arg != NULL && args->arguments_count-- && arg[0] != NULL; arg++) {
		//dprintf("adding args: '%s'\n", arg[0]);
		add_safe_mode_settings((char*)arg[0]);
	}
	return B_OK;
}


status_t
add_safe_mode_settings(const char* settings)
{
	if (settings == NULL || settings[0] == '\0')
		return B_OK;

	size_t length = strlen(settings);
	char* buffer = (char*)kernel_args_malloc(length + 1);
	if (buffer == NULL)
		return B_NO_MEMORY;

	driver_settings_file* file = (driver_settings_file*)kernel_args_malloc(
		sizeof(driver_settings_file));
	if (file == NULL) {
		kernel_args_free(buffer);
		return B_NO_MEMORY;
	}

	strlcpy(file->name, B_SAFEMODE_DRIVER_SETTINGS, sizeof(file->name));
	memcpy(buffer, settings, length + 1);
	file->buffer = buffer;
	file->size = length;

	// add it to the list
	file->next = gKernelArgs.driver_settings;
	gKernelArgs.driver_settings = file;

	return B_OK;
}


void
apply_boot_settings()
{
	void* kernelSettings = load_driver_settings("kernel");
	void* safemodeSettings = load_driver_settings(B_SAFEMODE_DRIVER_SETTINGS);

	apply_boot_settings(kernelSettings, safemodeSettings);

	if (safemodeSettings != NULL)
		unload_driver_settings(safemodeSettings);
	if (kernelSettings)
		unload_driver_settings(kernelSettings);
}
