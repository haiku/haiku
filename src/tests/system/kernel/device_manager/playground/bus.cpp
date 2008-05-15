/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "bus.h"

#include <KernelExport.h>
#include <PCI.h>


void
bus_trigger_device_removed(device_node* node)
{
	// the network device
	device_attr attrs[] = {
		{B_DEVICE_VENDOR_ID, B_UINT16_TYPE, {ui16: 0x1001}},
		{B_DEVICE_ID, B_UINT16_TYPE, {ui16: 0x0001}},
		{NULL}
	};

	device_node* child = NULL;
	while (gDeviceManager->get_next_child_node(node, attrs, &child) == B_OK) {
		gDeviceManager->unregister_node(child);
	}
}


void
bus_trigger_device_added(device_node* node)
{
}


//	#pragma mark - bus


static float
supports_device(device_node* parent)
{
	const char* bus;
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false)
			!= B_OK)
		return -1;

	if (bus != NULL && !strcmp(bus, "root"))
		return 1.0;

	return -1;
}


static status_t
register_device(device_node* parent)
{
	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME,	B_STRING_TYPE,	{string: "My Bus"}},
		{B_DEVICE_BUS,			B_STRING_TYPE,	{string: BUS_NAME}},
		{NULL}
	};

	return gDeviceManager->register_node(parent, BUS_MODULE_NAME, attrs, NULL,
		NULL);
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

	const struct device_info {
		uint16		vendor;
		uint16		device;
		uint16		type;
		uint16		sub_type;
		uint16		interface;
	} kDevices[] = {
		{0x1000, 0x0001, PCI_mass_storage, PCI_sata, PCI_sata_ahci},
		{0x1001, 0x0001, PCI_network, PCI_ethernet, 0},
		{0x1001, 0x0002, PCI_display, 0, 0},
		{0x1002, 0x0001, PCI_multimedia, PCI_audio, 0},
		{0x1002, 0x0002, PCI_serial_bus, PCI_usb, PCI_usb_ehci},
	};
	const size_t kNumDevices = sizeof(kDevices) / sizeof(kDevices[0]);

	for (uint32 i = 0; i < kNumDevices; i++) {
		device_attr attrs[] = {
			// info about the device
			{B_DEVICE_VENDOR_ID, B_UINT16_TYPE, {ui16: kDevices[i].vendor}},
			{B_DEVICE_ID, B_UINT16_TYPE, {ui16: kDevices[i].device}},

			{B_DEVICE_BUS, B_STRING_TYPE, {string: BUS_NAME}},
			{B_DEVICE_TYPE, B_UINT16_TYPE, {ui16: kDevices[i].type}},
			{B_DEVICE_SUB_TYPE, B_UINT16_TYPE,
				{ui16: kDevices[i].sub_type}},
			{B_DEVICE_INTERFACE, B_UINT16_TYPE,
				{ui16: kDevices[i].interface}},

			{B_DEVICE_FLAGS, B_UINT32_TYPE, {ui32: B_FIND_CHILD_ON_DEMAND}},
			{NULL}
		};

		gDeviceManager->register_node(node, BUS_FOR_DRIVER_NAME, attrs, NULL,
			NULL);
	}

	device_attr attrs[] = {
		{B_DEVICE_FIXED_CHILD, B_STRING_TYPE,
			{string: "non_existing/driver_v1"}},
		{NULL}
	};

#if 1
	// this is supposed to fail
	dprintf("non-existing child: %ld\n", gDeviceManager->register_node(node,
		BUS_FOR_DRIVER_NAME, attrs, NULL, NULL));
#endif
	return B_OK;
}


static status_t
rescan_child_devices(void* cookie)
{
	return B_ERROR;
}


static void
device_removed(void* cookie)
{
}


//	#pragma mark - for driver


static status_t
get_bus_info(void* cookie, bus_info* info)
{
	gDeviceManager->get_attr_uint16((device_node*)cookie, B_DEVICE_VENDOR_ID,
		&info->vendor_id, false);
	gDeviceManager->get_attr_uint16((device_node*)cookie, B_DEVICE_ID,
		&info->device_id, false);
	return B_OK;
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

struct bus_for_driver_module_info gBusDriverModuleInfo = {
	{
		{
			BUS_FOR_DRIVER_NAME,
			0,
			NULL,
		},

		NULL,
		NULL,

		init_driver,
	},
	get_bus_info
};

