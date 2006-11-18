/*
 * Copyright 2006, Jérôme Duval. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

#include <malloc.h>
#include <string.h>
#include <stdio.h>

#include "acpi_priv.h"

// information about one ACPI device
typedef struct acpi_device_info {
	char			*path;			// path
	device_node_handle	node;
	char			name[32];		// name (for fast log)
} acpi_device_info;


static status_t
acpi_device_init_driver(device_node_handle node, void *user_cookie, void **cookie)
{
	char *path;
	acpi_device_info *device;
	status_t status = B_OK;
	
	if (gDeviceManager->get_attr_string(node, ACPI_DEVICE_PATH_ITEM, &path, false) != B_OK)
		return B_ERROR;

	device = malloc(sizeof(*device));
	if (device == NULL) {
		return B_NO_MEMORY;
	}

	memset(device, 0, sizeof(*device));

	device->path = path;
	device->node = node;
	
	snprintf(device->name, sizeof(device->name), "acpi_device %s", 
		path);

	*cookie = device;

	return status;
}


static status_t
acpi_device_uninit_driver(void *cookie)
{
	acpi_device_info *device = cookie;
	
	free(device->path);
	free(device);
	return B_OK;
}


static status_t
acpi_device_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}

	return B_BAD_VALUE;
}

acpi_device_module_info gACPIDeviceModule = {
	{
		{
			ACPI_DEVICE_MODULE_NAME,
			0,
			acpi_device_std_ops
		},

		NULL,		// supports device
		NULL,		// register device (our parent registered us)
		acpi_device_init_driver,
		acpi_device_uninit_driver,
		NULL,		// removed
		NULL,		// cleanup
		NULL,		// get supported paths
	},

	
};
