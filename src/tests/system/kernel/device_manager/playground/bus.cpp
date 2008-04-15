/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "bus.h"


#define BUS_MODULE_NAME "bus_managers/sample_bus/driver_v1"


//	#pragma mark - bus


static float
supports_device(device_node *parent)
{
	const char* bus;
	if (gDeviceManager->get_attr_string(parent, B_DRIVER_BUS, &bus, false)
			!= B_OK)
		return -1;

	if (bus != NULL && !strcmp(bus, "root"))
		return 1.0;

	return -1;
}


static status_t
register_device(device_node *parent)
{
	device_attr attrs[] = {
		{B_DRIVER_PRETTY_NAME,	B_STRING_TYPE,	{string: "My Bus"}},
		{B_DRIVER_BUS,			B_STRING_TYPE,	{string: BUS_NAME}},
		{B_DRIVER_IS_BUS,		B_INT8_TYPE,	{ui8: true}},
		{NULL}
	};

	return gDeviceManager->register_device(parent, BUS_MODULE_NAME, attrs, NULL,
		NULL);
}


static status_t
init_driver(device_node *node, void **_cookie)
{
	return B_OK;
}


static void
uninit_driver(device_node *node)
{
}


static status_t
register_child_devices(device_node *node)
{
	const struct device_info {
		uint16		vendor;
		uint16		device;
		const char	*type;
	} kDevices[] = {
		{0x1000, 0x0001, B_DISK_DRIVER_TYPE},
		{0x1001, 0x0001, B_NETWORK_DRIVER_TYPE},
		{0x1002, 0x0001, B_AUDIO_DRIVER_TYPE},
		{0x1002, 0x0002, B_BUS_DRIVER_TYPE},
	};
	const size_t kNumDevices = sizeof(kDevices) / sizeof(kDevices[0]);

	for (uint32 i = 0; i < kNumDevices; i++) {
		device_attr attrs[] = {
			// info about the device
			{ "bus/vendor", B_UINT16_TYPE, { ui16: kDevices[i].vendor }},
			{ "bus/device", B_UINT16_TYPE, { ui16: kDevices[i].device }},

			{ B_DRIVER_BUS, B_STRING_TYPE, { string: "pci" }},
			{ B_DRIVER_DEVICE_TYPE, B_STRING_TYPE, { string: kDevices[i].type}},
			{ NULL }
		};

		gDeviceManager->register_device(node, BUS_DEVICE_NAME, attrs, NULL,
			NULL);
	}

	return B_OK;
}


static status_t
rescan_child_devices(device_node *node)
{
	return B_ERROR;
}


static void
device_removed(device_node *node)
{
}


//	#pragma mark -


struct driver_module_info gBusModuleInfo = {
	{
		BUS_MODULE_NAME,
		0,
		NULL,
	},

	supports_device,
	register_device,

	init_driver,
	uninit_driver,
	register_child_devices,
	rescan_child_devices,
	device_removed,
};

struct driver_module_info gBusDriverModuleInfo = {
	{
		BUS_DEVICE_NAME,
		0,
		NULL,
	},

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

