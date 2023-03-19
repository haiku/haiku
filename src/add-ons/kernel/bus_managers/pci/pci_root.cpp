/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2003, Marcus Overhagen. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <device_manager.h>
#include <PCI.h>
#include <drivers/ACPI.h>
#include <drivers/bus/FDT.h>

#include <string.h>

#include <AutoDeleterDrivers.h>

#include "pci_private.h"
#include "pci.h"

#define CHECK_RET(err) {status_t _err = (err); if (_err < B_OK) return _err;}


// name of PCI root module
#define PCI_ROOT_MODULE_NAME "bus_managers/pci/root/driver_v1"


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
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {.string = "PCI"}},
		{B_DEVICE_FLAGS, B_UINT32_TYPE, {.ui32 = B_KEEP_DRIVER_LOADED}},
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
		uint8 domain;
		uint8 bus;
		if (gPCI->ResolveVirtualBus(info.bus, &domain, &bus) != B_OK) {
			dprintf("ResolveVirtualBus(%u) failed\n", info.bus);
			continue;
		}

		device_attr attrs[] = {
			// info about device
			{B_DEVICE_BUS, B_STRING_TYPE, {.string = "pci"}},

			// location on PCI bus
			{B_PCI_DEVICE_DOMAIN, B_UINT8_TYPE, {.ui8 = domain}},
			{B_PCI_DEVICE_BUS, B_UINT8_TYPE, {.ui8 = bus}},
			{B_PCI_DEVICE_DEVICE, B_UINT8_TYPE, {.ui8 = info.device}},
			{B_PCI_DEVICE_FUNCTION, B_UINT8_TYPE, {.ui8 = info.function}},

			// info about the device
			{B_DEVICE_VENDOR_ID, B_UINT16_TYPE, {.ui16 = info.vendor_id}},
			{B_DEVICE_ID, B_UINT16_TYPE, {.ui16 = info.device_id}},

			{B_DEVICE_TYPE, B_UINT16_TYPE, {.ui16 = info.class_base}},
			{B_DEVICE_SUB_TYPE, B_UINT16_TYPE, {.ui16 = info.class_sub}},
			{B_DEVICE_INTERFACE, B_UINT16_TYPE, {.ui16 = info.class_api}},

			{B_DEVICE_FLAGS, B_UINT32_TYPE, {.ui32 = B_FIND_CHILD_ON_DEMAND}},
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

	DeviceNodePutter<&gDeviceManager> pciHostNode(gDeviceManager->get_parent_node(node));

	pci_controller_module_info* pciHostModule;
	void* pciHostDev;
	CHECK_RET(gDeviceManager->get_driver(pciHostNode.Get(), (driver_module_info**)&pciHostModule, &pciHostDev));

	module_info *module;
	status_t res = get_module(B_PCI_MODULE_NAME, &module);
	if (res < B_OK)
		return res;

	CHECK_RET(gPCI->AddController(pciHostModule, pciHostDev, node));
	CHECK_RET(pci_init_deferred());

	return B_OK;
}


static int32
pci_root_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return B_OK;

		case B_MODULE_UNINIT:
			return B_OK;
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

		NULL,
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
