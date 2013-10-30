/*
 * Copyright 2008, Haiku, Inc. All Rights Reserved.
 *
 * Distributed under the terms of the MIT License.
 */

#include <KernelExport.h>
#include <Drivers.h>
#include <Errors.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include <ACPI.h>
#include "acpi_lid.h"

#define ACPI_LID_MODULE_NAME "drivers/power/acpi_lid/driver_v1"

#define ACPI_LID_DEVICE_MODULE_NAME "drivers/power/acpi_lid/device_v1"

/* Base Namespace devices are published to */
#define ACPI_LID_BASENAME "power/acpi_lid/%d"

// name of pnp generator of path ids
#define ACPI_LID_PATHID_GENERATOR "acpi_lid/path_id"

static device_manager_info *sDeviceManager;

typedef struct acpi_ns_device_info {
	device_node *node;
	acpi_device_module_info *acpi;
	acpi_device acpi_cookie;
} acpi_lid_device_info;


//	#pragma mark - device module API


static status_t
acpi_lid_init_device(void *_cookie, void **cookie)
{
	device_node *node = (device_node *)_cookie;
	acpi_lid_device_info *device;
	device_node *parent;
	
	device = (acpi_lid_device_info *)calloc(1, sizeof(*device));
	if (device == NULL)
		return B_NO_MEMORY;

	device->node = node;

	parent = sDeviceManager->get_parent_node(node);
	sDeviceManager->get_driver(parent, (driver_module_info **)&device->acpi,
		(void **)&device->acpi_cookie);
	sDeviceManager->put_node(parent);

	*cookie = device;
	return B_OK;
}


static void
acpi_lid_uninit_device(void *_cookie)
{
	acpi_lid_device_info *device = (acpi_lid_device_info *)_cookie;
	free(device);
}


static status_t
acpi_lid_open(void *_cookie, const char *path, int flags, void** cookie)
{
	acpi_lid_device_info *device = (acpi_lid_device_info *)_cookie;
	*cookie = device;
	return B_OK;
}


static status_t
acpi_lid_read(void* _cookie, off_t position, void *buf, size_t* num_bytes)
{
	return B_ERROR;
}


static status_t
acpi_lid_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	return B_ERROR;
}


static status_t
acpi_lid_control(void* _cookie, uint32 op, void* arg, size_t len)
{
	acpi_lid_device_info* device = (acpi_lid_device_info*)_cookie;
	
	return B_ERROR;
}


static status_t
acpi_lid_close (void* cookie)
{
	return B_OK;
}


static status_t
acpi_lid_free (void* cookie)
{	
	return B_OK;
}


//	#pragma mark - driver module API


static float
acpi_lid_support(device_node *parent)
{
	const char *bus;
	uint32 device_type;
	const char *hid;

	dprintf("acpi_lid_support\n");

	// make sure parent is really the ACPI bus manager
	if (sDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return -1;
	
	if (strcmp(bus, "acpi"))
		return 0.0;

	// check whether it's really a device
	if (sDeviceManager->get_attr_uint32(parent, ACPI_DEVICE_TYPE_ITEM, &device_type, false) != B_OK
		|| device_type != ACPI_TYPE_DEVICE) {
		return 0.0;
	}

	// check whether it's a lid device
	if (sDeviceManager->get_attr_string(parent, ACPI_DEVICE_HID_ITEM, &hid, false) != B_OK
		|| strcmp(hid, "PNP0C0D")) {
		return 0.0;
	}

	return 0.6;
}


static status_t
acpi_lid_register_device(device_node *node)
{
	device_attr attrs[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE, { string: "ACPI Lid" }},
		{ NULL }
	};

	return sDeviceManager->register_node(node, ACPI_LID_MODULE_NAME, attrs, NULL, NULL);
}


static status_t
acpi_lid_init_driver(device_node *node, void **_driverCookie)
{
	*_driverCookie = node;
	return B_OK;
}


static void
acpi_lid_uninit_driver(void *driverCookie)
{
}


static status_t
acpi_lid_register_child_devices(void *_cookie)
{
	device_node *node = _cookie;
	int path_id;
	char name[128];

	dprintf("acpi_lid_register_child_devices\n");
		
	path_id = sDeviceManager->create_id(ACPI_LID_PATHID_GENERATOR);
	if (path_id < 0) {
		dprintf("acpi_lid_register_child_devices: couldn't create a path_id\n");
		return B_ERROR;
	}
	
	snprintf(name, sizeof(name), ACPI_LID_BASENAME, path_id);
		
	return sDeviceManager->publish_device(node, name, ACPI_LID_DEVICE_MODULE_NAME);
}


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager },
	{}
};


driver_module_info acpi_lid_driver_module = {
	{
		ACPI_LID_MODULE_NAME,
		0,
		NULL
	},

	acpi_lid_support,
	acpi_lid_register_device,
	acpi_lid_init_driver,
	acpi_lid_uninit_driver,
	acpi_lid_register_child_devices,
	NULL,	// rescan
	NULL,	// removed
};


struct device_module_info acpi_lid_device_module = {
	{
		ACPI_LID_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	acpi_lid_init_device,
	acpi_lid_uninit_device,
	NULL,

	acpi_lid_open,
	acpi_lid_close,
	acpi_lid_free,
	acpi_lid_read,
	acpi_lid_write,
	NULL,
	acpi_lid_control,

	NULL,
	NULL
};

module_info *modules[] = {
	(module_info *)&acpi_lid_driver_module,
	(module_info *)&acpi_lid_device_module,
	NULL
};
