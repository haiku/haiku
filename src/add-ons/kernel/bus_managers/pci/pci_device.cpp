/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002-2003, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pci.h"
#include "pci_private.h"


// information about one PCI device
struct pci_device {
	PCIDev*					device;
	device_node*			node;
};


static status_t
pci_device_init_driver(device_node* node, void** _cookie)
{
	uint8 domain;
	uint8 bus, deviceNumber, function;
	if (gDeviceManager->get_attr_uint8(node, B_PCI_DEVICE_DOMAIN, &domain,
			false) != B_OK
		|| gDeviceManager->get_attr_uint8(node, B_PCI_DEVICE_BUS, &bus,
			false) != B_OK
		|| gDeviceManager->get_attr_uint8(node, B_PCI_DEVICE_DEVICE,
			&deviceNumber, false) != B_OK
		|| gDeviceManager->get_attr_uint8(node, B_PCI_DEVICE_FUNCTION,
			&function, false) != B_OK)
		return B_ERROR;

	PCIDev *dev = gPCI->FindDevice(domain, bus, deviceNumber, function);
	if (dev == NULL) {
		panic("device not found!\n");
		return ENODEV;
	}

	pci_device* device = (pci_device*)malloc(sizeof(*device));
	if (device == NULL)
		return B_NO_MEMORY;

	device->device = dev;
	device->node = node;

	*_cookie = device;
	return B_OK;
}


static void
pci_device_uninit_driver(void* cookie)
{
	pci_device* device = (pci_device*)cookie;
	free(device);
}


static status_t
pci_device_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}

	return B_BAD_VALUE;
}


pci_device_module_info gPCIDeviceModule = {
	{
		{
			PCI_DEVICE_MODULE_NAME,
			0,
			pci_device_std_ops
		},

		.init_driver = pci_device_init_driver,
		.uninit_driver = pci_device_uninit_driver,
	},

	.read_io_8 = [](pci_device *device, addr_t mappedIOAddress) {
		return pci_read_io_8(mappedIOAddress);
	},
	.write_io_8 = [](pci_device *device, addr_t mappedIOAddress, uint8 value) {
		pci_write_io_8(mappedIOAddress, value);
	},
	.read_io_16 = [](pci_device *device, addr_t mappedIOAddress) {
		return pci_read_io_16(mappedIOAddress);
	},
	.write_io_16 = [](pci_device *device, addr_t mappedIOAddress, uint16 value) {
		pci_write_io_16(mappedIOAddress, value);
	},
	.read_io_32 = [](pci_device *device, addr_t mappedIOAddress) {
		return pci_read_io_32(mappedIOAddress);
	},
	.write_io_32 = [](pci_device *device, addr_t mappedIOAddress, uint32 value) {
		pci_write_io_32(mappedIOAddress, value);
	},
	.ram_address = [](pci_device *device, phys_addr_t physicalAddress) {
		return pci_ram_address(physicalAddress);
	},
	.read_pci_config = [](pci_device *device, uint16 offset, uint8 size) {
		return gPCI->ReadConfig(device->device, offset, size);
	},
	.write_pci_config = [](pci_device *device, uint16 offset, uint8 size, uint32 value) {
		gPCI->WriteConfig(device->device, offset, size, value);
	},
	.find_pci_capability = [](pci_device *device, uint8 capID, uint8 *offset) {
		return gPCI->FindCapability(device->device, capID, offset);
	},
	.get_pci_info = [](pci_device *device, struct pci_info *info) {
		if (info == NULL)
			return;
		*info = device->device->info;
	},
	.find_pci_extended_capability = [](pci_device *device, uint16 capID, uint16 *offset) {
		return gPCI->FindExtendedCapability(device->device, capID, offset);
	},
	.get_powerstate = [](pci_device *device) {
		return gPCI->GetPowerstate(device->device);
	},
	.set_powerstate = [](pci_device *device, uint8 state) {
		gPCI->SetPowerstate(device->device, state);
	},
	.get_msi_count = [](pci_device *device) {
		return gPCI->GetMSICount(device->device);
	},
	.configure_msi = [](pci_device *device, uint8 count, uint8 *startVector) {
		return gPCI->ConfigureMSI(device->device, count, startVector);
	},
	.unconfigure_msi = [](pci_device *device) {
		return gPCI->UnconfigureMSI(device->device);
	},
	.enable_msi = [](pci_device *device) {
		return gPCI->EnableMSI(device->device);
	},
	.disable_msi = [](pci_device *device) {
		return gPCI->DisableMSI(device->device);
	},
	.get_msix_count = [](pci_device *device) {
		return gPCI->GetMSIXCount(device->device);
	},
	.configure_msix = [](pci_device *device, uint8 count, uint8 *startVector) {
		return gPCI->ConfigureMSIX(device->device, count, startVector);
	},
	.enable_msix = [](pci_device *device) {
		return gPCI->EnableMSIX(device->device);
	}
};
