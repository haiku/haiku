/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "bus.h"

#include <string.h>

#include <KernelExport.h>
#include <PCI.h>


#define DRIVER_MODULE_NAME "drivers/graphics/specific_driver/driver_v1"
#define DRIVER_DEVICE_MODULE_NAME "drivers/graphics/specific_driver/device_v1"


//	#pragma mark - driver


static float
supports_device(device_node *parent)
{
	bus_for_driver_module_info* module;
	void* data;
	gDeviceManager->get_driver(parent, (driver_module_info**)&module, &data);

	if (strcmp(module->info.info.name, BUS_FOR_DRIVER_NAME))
		return -1;

	bus_info info;
	if (module->get_bus_info(data, &info) == B_OK
		&& info.vendor_id == 0x1001
		&& info.device_id == 0x0002)
		return 0.8;

	return 0.0;
}


static status_t
register_device(device_node *parent)
{
	return gDeviceManager->register_node(parent, DRIVER_MODULE_NAME, NULL,
		NULL, NULL);
}


static status_t
init_driver(device_node *node, void **_cookie)
{
	*_cookie = node;
	return B_OK;
}


static void
uninit_driver(void* cookie)
{
}


static status_t
register_child_devices(void* cookie)
{
	device_node* node = (device_node*)cookie;

	gDeviceManager->publish_device(node, "graphics/specific/0",
		DRIVER_DEVICE_MODULE_NAME);
	return B_OK;
}


static void
device_removed(void* cookie)
{
}


//	#pragma mark - device


static status_t
init_device(void* driverCookie, void** _deviceCookie)
{
	// called once before one or several open() calls
	return B_OK;
}


static void
uninit_device(void* deviceCookie)
{
	// supposed to free deviceCookie, called when the last reference to
	// the device is closed
}


static status_t
device_open(void* deviceCookie, int openMode, void** _cookie)
{
	// deviceCookie is an object attached to the published device
	return B_ERROR;
}


static status_t
device_close(void* cookie)
{
	return B_ERROR;
}


static status_t
device_free(void* cookie)
{
	return B_ERROR;
}


static status_t
device_read(void* cookie, off_t pos, void* buffer, size_t* _length)
{
	return B_ERROR;
}


static status_t
device_write(void* cookie, off_t pos, const void* buffer, size_t* _length)
{
	return B_ERROR;
}


static status_t
device_ioctl(void* cookie, int32 op, void* buffer, size_t length)
{
	return B_ERROR;
}


static status_t
device_io(void* cookie, io_request* request)
{
	// new function to deal with I/O requests directly.
	return B_ERROR;
}


//	#pragma mark -


struct driver_module_info gSpecificVideoDriverModuleInfo = {
	{
		DRIVER_MODULE_NAME,
		0,
		NULL,
	},

	supports_device,
	register_device,
	init_driver,
	uninit_driver,
	register_child_devices,
	NULL,
	device_removed,
};

struct device_module_info gSpecificVideoDeviceModuleInfo = {
	{
		DRIVER_DEVICE_MODULE_NAME,
		0,
		NULL,
	},

	init_device,
	uninit_device,
	NULL,	// device_removed

	device_open,
	device_close,
	device_free,
	device_read,
	device_write,
	device_ioctl,
	device_io,
};
