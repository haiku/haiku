/*
 * Copyright 2009-2010, Colin Günther, coling@gmx.de.
 * All Rights Reserved. Distributed under the terms of the MIT License.
 *
 */


#include <posix/sys/mman.h>

#include <compat/sys/param.h>
#include <compat/sys/firmware.h>
#include <compat/sys/haiku-module.h>

#include <stdlib.h>
#include <string.h>

#include <FindDirectory.h>
#include <StorageDefs.h>
#include <SupportDefs.h>

#include <driver_settings.h>
#include <kernel/vm/vm.h>
#include <syscalls.h>
#include <vm_defs.h>

#include <device.h>



#define MAX_FBSD_FIRMWARE_NAME_CHARS 64
	// For strndup, beeing cautious in kernel code is a good thing.
	// NB: This constant doesn't exist in FreeBSD.


const struct firmware*
firmware_get(const char* fbsdFirmwareName)
{
	area_id				area;
	void*				driverSettings = NULL;
	char*				fbsdFirmwareNameCopy = NULL;
	int					fileDescriptor = 0;
	struct firmware*	firmware = NULL;
	int32				firmwareFileSize;
	char*				firmwarePath = NULL;
	const char*			haikuFirmwareName = NULL;

	driverSettings = load_driver_settings(gDriverName);
	if (driverSettings == NULL) {
		driver_printf("%s: settings file %s is missing.\n", __func__,
			gDriverName);
		return NULL;
	}

	haikuFirmwareName = get_driver_parameter(driverSettings, fbsdFirmwareName,
		NULL, NULL);
	if (haikuFirmwareName == NULL) {
		driver_printf("%s: settings file %s file contains no mapping for %s.\n", 
			__func__, gDriverName, fbsdFirmwareName);
		goto cleanup;
	}

	firmwarePath = (char*)malloc(B_PATH_NAME_LENGTH);
	if (firmwarePath == NULL)
		goto cleanup;

	if (find_directory(B_SYSTEM_DATA_DIRECTORY, -1, false,
		firmwarePath, B_PATH_NAME_LENGTH) != B_OK)
		goto cleanup;

	strlcat(firmwarePath, "/firmware/", B_PATH_NAME_LENGTH);
	strlcat(firmwarePath, gDriverName, B_PATH_NAME_LENGTH);
	strlcat(firmwarePath, "/", B_PATH_NAME_LENGTH);
	strlcat(firmwarePath, haikuFirmwareName, B_PATH_NAME_LENGTH);

	fileDescriptor = open(firmwarePath, B_READ_ONLY);
	if (fileDescriptor == -1)
		goto cleanup;

	firmwareFileSize = lseek(fileDescriptor, 0, SEEK_END);
	lseek(fileDescriptor, 0, SEEK_SET);

	fbsdFirmwareNameCopy = strndup(fbsdFirmwareName,
		MAX_FBSD_FIRMWARE_NAME_CHARS);
	if (fbsdFirmwareNameCopy == NULL)
		goto cleanup;

	firmware = (struct firmware*)malloc(sizeof(struct firmware));
	if (firmware == NULL)
		goto cleanup;

	firmware->data = NULL;
	area = _user_map_file("mmap area", (void*)&firmware->data, B_ANY_ADDRESS,
		firmwareFileSize, B_READ_AREA, REGION_PRIVATE_MAP, true, fileDescriptor,
		0);
	if (area < 0)
		goto cleanup;

	firmware->datasize = firmwareFileSize;
	firmware->name = fbsdFirmwareNameCopy;
	firmware->version = __haiku_firmware_version;

	close(fileDescriptor);
	free(firmwarePath);
	unload_driver_settings(driverSettings);
	return firmware;

cleanup:
	if (firmware)
		free(firmware);
	if (fbsdFirmwareNameCopy)
		free(fbsdFirmwareNameCopy);
	if (firmwarePath)
		free(firmwarePath);
	if (fileDescriptor)
		close(fileDescriptor);
	if (driverSettings)
		unload_driver_settings(driverSettings);
	return NULL;
}


void
firmware_put(const struct firmware* firmware, int flags)
{
	if (firmware == NULL)
		return;

	_user_unmap_memory((void*)firmware->data, firmware->datasize);
	if (firmware->name)
		free((void*)firmware->name);
	free((void*)firmware);
}
