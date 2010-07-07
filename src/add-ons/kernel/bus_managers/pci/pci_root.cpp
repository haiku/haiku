/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2003, Marcus Overhagen. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <device_manager.h>
#include <PCI.h>

#include <string.h>

#include "pci_private.h"
#include "pci.h"


// name of PCI root module
#define PCI_ROOT_MODULE_NAME "bus_managers/pci/root/driver_v1"


static float
pci_root_supports_device(device_node* parent)
{
	// make sure parent is really device root
	const char* bus;
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return B_ERROR;

	if (strcmp(bus, "root"))
		return 0.0;

	return 1.0;
}


static status_t
pci_root_register_device(device_node* parent)
{
// XXX how do we handle this for PPC?
// I/O port for PCI config space address
#define PCI_CONFIG_ADDRESS 0xcf8

	io_resource resources[2] = {
		{B_IO_PORT, PCI_CONFIG_ADDRESS, 8},
		{}
	};
	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "PCI"}},
		{B_DEVICE_FLAGS, B_UINT32_TYPE, {ui32: B_KEEP_DRIVER_LOADED}},
		{}
	};

	return gDeviceManager->register_node(parent, PCI_ROOT_MODULE_NAME, attrs,
		resources, NULL);
}


static status_t
pci_root_register_child_devices(void* cookie)
{
	device_node* node = (device_node*)cookie;

	pci_info info;
	for (int32 i = 0; pci_get_nth_pci_info(i, &info) == B_OK; i++) {
		int domain;
		uint8 bus;
		if (gPCI->ResolveVirtualBus(info.bus, &domain, &bus) != B_OK) {
dprintf("FAILED!!!!\n");
			continue;
		}

		device_attr attrs[] = {
			// info about device
			{B_DEVICE_BUS, B_STRING_TYPE, {string: "pci"}},

			// location on PCI bus
			{B_PCI_DEVICE_DOMAIN, B_UINT32_TYPE, {ui32: domain}},
			{B_PCI_DEVICE_BUS, B_UINT8_TYPE, {ui8: bus}},
			{B_PCI_DEVICE_DEVICE, B_UINT8_TYPE, {ui8: info.device}},
			{B_PCI_DEVICE_FUNCTION, B_UINT8_TYPE, {ui8: info.function}},

			// info about the device
			{B_DEVICE_VENDOR_ID, B_UINT16_TYPE, { ui16: info.vendor_id }},
			{B_DEVICE_ID, B_UINT16_TYPE, { ui16: info.device_id }},

			{B_DEVICE_TYPE, B_UINT16_TYPE, { ui16: info.class_base }},
			{B_DEVICE_SUB_TYPE, B_UINT16_TYPE, { ui16: info.class_sub }},
			{B_DEVICE_INTERFACE, B_UINT16_TYPE, { ui16: info.class_api }},

			{B_DEVICE_FLAGS, B_UINT32_TYPE, {ui32: B_FIND_CHILD_ON_DEMAND}},
			{}
		};

		gDeviceManager->register_node(node, PCI_DEVICE_MODULE_NAME, attrs,
			NULL, NULL);
	}

	return B_OK;
}


static status_t
pci_root_init(device_node* node, void** _cookie)
{
	*_cookie = node;
	return B_OK;
}


static int32
pci_root_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			module_info *module;
			return get_module(B_PCI_MODULE_NAME, &module);
				// this serializes our module initialization
		}

		case B_MODULE_UNINIT:
			return put_module(B_PCI_MODULE_NAME);
	}

	return B_BAD_VALUE;
}


struct pci_root_module_info gPCIRootModule = {
	{
		{
			PCI_ROOT_MODULE_NAME,
			0,
			pci_root_std_ops
		},

		pci_root_supports_device,
		pci_root_register_device,
		pci_root_init,
		NULL,	// uninit
		pci_root_register_child_devices,
		NULL,	// rescan devices
		NULL,	// device removed
	},

	&pci_read_config,
	&pci_write_config
};
