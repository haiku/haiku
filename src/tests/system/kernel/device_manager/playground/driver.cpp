/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "bus.h"

#include <KernelExport.h>


#define DRIVER_MODULE_NAME "drivers/net/sample_driver/driver_v1"
#define DRIVER_DEVICE_MODULE_NAME "drivers/net/sample_driver/device_v1"


//	#pragma mark - driver


static float
supports_device(device_node* parent)
{
#if 0
	const char* bus;
	if (gDeviceManager->get_attr_string(parent, B_DRIVER_BUS, &bus, false)
			!= B_OK)
		return -1;

	if (bus == NULL || strcmp(bus, BUS_NAME))
		return -1;
#endif

	bus_for_driver_module_info* module;
	void* data;
	gDeviceManager->get_driver(parent, (driver_module_info**)&module, &data);

	if (strcmp(module->info.info.name, BUS_FOR_DRIVER_NAME))
		return -1;

	bus_info info;
	if (module->get_bus_info(data, &info) == B_OK
		&& info.vendor_id == 0x1001
		&& info.device_id == 0x0001)
		return 1.0;

	return 0.0;
}


static status_t
register_device(device_node* parent)
{
	return gDeviceManager->register_node(parent, DRIVER_MODULE_NAME, NULL,
		NULL, NULL);
}


static status_t
init_driver(device_node* node, void** _cookie)
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

	gDeviceManager->publish_device(node, "net/sample/0",
		DRIVER_DEVICE_MODULE_NAME);
	return B_OK;
}


static void
driver_device_removed(void* cookie)
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


static void
device_removed(void* deviceCookie)
{
	dprintf("network device removed!\n");
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


struct driver_module_info gDriverModuleInfo = {
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
	driver_device_removed,
};

struct device_module_info gDeviceModuleInfo = {
	{
		DRIVER_DEVICE_MODULE_NAME,
		0,
		NULL,
	},

	init_device,
	uninit_device,
	device_removed,

	device_open,
	device_close,
	device_free,
	device_read,
	device_write,
	device_ioctl,
	device_io,
};
