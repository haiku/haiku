/*
 * Copyright 2008, Rene Gollent, rene@gollent.com. All rights reserved.
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "load_driver_settings.h"

#include <OS.h>
#include <drivers/driver_settings.h>

#include <boot/driver_settings.h>
#include <boot/kernel_args.h>
#include <boot/stage2.h>
#include <boot/platform.h>

#include <string.h>
#include <unistd.h>


static driver_settings_file *
find_driver_settings_file(const char *name)
{
	for (driver_settings_file *file = gKernelArgs.driver_settings; file != NULL; file = file->next) {
		if (!strcmp(file->name, name))
			return file;
	}

	return NULL;
}


static status_t
load_driver_settings_file(Directory *directory, const char *name)
{
	int fd = open_from(directory, name, O_RDONLY);
	if (fd < 0)
		return fd;

	struct stat stat;
	fstat(fd, &stat);

	char *buffer = (char *)kernel_args_malloc(stat.st_size + 1);
	if (buffer == NULL)
		return B_NO_MEMORY;

	if (read(fd, buffer, stat.st_size) != stat.st_size)
		return B_IO_ERROR;

	driver_settings_file *file = (driver_settings_file *)kernel_args_malloc(sizeof(driver_settings_file));
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


status_t
load_driver_settings(stage2_args */*args*/, Directory *volume)
{
	int fd = open_from(volume, "home/config/settings/kernel/drivers", O_RDONLY);
	if (fd < B_OK)
		return fd;

	Directory *settings = (Directory *)get_node_from(fd);
	if (settings == NULL)
		return B_ENTRY_NOT_FOUND;

	void *cookie;
	if (settings->Open(&cookie, O_RDONLY) == B_OK) {
		char name[B_FILE_NAME_LENGTH];
		while (settings->GetNextEntry(cookie, name, sizeof(name)) == B_OK) {
			if (!strcmp(name, ".") || !strcmp(name, ".."))
				continue;

			status_t status = load_driver_settings_file(settings, name);
			if (status != B_OK)
				dprintf("Could not load \"%s\" error %ld\n", name, status);
		}

		settings->Close(cookie);
	}

	// check if a kernel settings file exists
	// if it does, prepend it to the safe mode settings. This allows the 
	// settings from the kernel file to take effect while still allowing 
	// overrides by safe mode since the settings are searched
	// in reverse order. This allows us to permanently set things like 
	// disable_smp
	driver_settings_file *kernelFile = find_driver_settings_file("kernel");
	if (kernelFile != NULL) {
		driver_settings_file *safemodeFile = 
			find_driver_settings_file(B_SAFEMODE_DRIVER_SETTINGS);
		if (safemodeFile != NULL) {
			char *buffer = (char *)kernel_args_malloc(
				safemodeFile->size + kernelFile->size + 2);
			if (buffer != NULL) {
				memcpy(buffer, kernelFile->buffer, 
					kernelFile->size);

				// insert a newline just in case the kernel settings file
				// doesn't end with one
				buffer[kernelFile->size] = '\n';

				memcpy(buffer + kernelFile->size + 1, 
					safemodeFile->buffer, 
					safemodeFile->size);

				kernel_args_free(safemodeFile->buffer);
				safemodeFile->buffer = buffer;
				safemodeFile->size = safemodeFile->size + 
					kernelFile->size + 1;
				buffer[safemodeFile->size] = '\0';
			} 
		} else 
			add_safe_mode_settings(kernelFile->buffer);
	}

	return B_OK;
}


status_t
add_safe_mode_settings(char *settings)
{
	if (settings == NULL || settings[0] == '\0')
		return B_OK;

	size_t length = strlen(settings);
	char *buffer = (char *)kernel_args_malloc(length + 1);
	if (buffer == NULL)
		return B_NO_MEMORY;

	driver_settings_file *file = (driver_settings_file *)kernel_args_malloc(sizeof(driver_settings_file));
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

