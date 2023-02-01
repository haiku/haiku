/*
 * Copyright 2022, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <new>
#include <stdio.h>
#include <string.h>

#include <ByteOrder.h>
#include <bus/PCI.h>
#include <PCI_x86.h>

#define DRIVER_NAME "ccp_rng_pci"
#include "ccp.h"


typedef struct {
	ccp_device_info info;
	pci_device_module_info* pci;
	pci_device* device;

	pci_info pciinfo;
} ccp_pci_sim_info;


//	#pragma mark -


static status_t
init_device(device_node* node, void** device_cookie)
{
	CALLED();
	ccp_pci_sim_info* bus = (ccp_pci_sim_info*)calloc(1,
		sizeof(ccp_pci_sim_info));
	if (bus == NULL)
		return B_NO_MEMORY;

	pci_device_module_info* pci;
	pci_device* device;
	{
		device_node* pciParent = gDeviceManager->get_parent_node(node);
		gDeviceManager->get_driver(pciParent, (driver_module_info**)&pci,
			(void**)&device);
		gDeviceManager->put_node(pciParent);
	}

	bus->pci = pci;
	bus->device = device;

	pci_info *pciInfo = &bus->pciinfo;
	pci->get_pci_info(device, pciInfo);

#define BAR_INDEX 2
	bus->info.base_addr = pciInfo->u.h0.base_registers[BAR_INDEX];
	bus->info.map_size = pciInfo->u.h0.base_register_sizes[BAR_INDEX];
	if ((pciInfo->u.h0.base_register_flags[BAR_INDEX] & PCI_address_type)
			== PCI_address_type_64) {
		bus->info.base_addr |= (uint64)pciInfo->u.h0.base_registers[BAR_INDEX + 1] << 32;
		bus->info.map_size |= (uint64)pciInfo->u.h0.base_register_sizes[BAR_INDEX + 1] << 32;
	}

	if (bus->info.base_addr == 0) {
		ERROR("PCI BAR not assigned\n");
		free(bus);
		return B_ERROR;
	}

	// enable bus master and memory
	uint16 pcicmd = pci->read_pci_config(device, PCI_command, 2);
	pcicmd |= PCI_command_master | PCI_command_memory;
	pci->write_pci_config(device, PCI_command, 2, pcicmd);

	*device_cookie = bus;
	return B_OK;
}


static void
uninit_device(void* device_cookie)
{
	ccp_pci_sim_info* bus = (ccp_pci_sim_info*)device_cookie;
	free(bus);
}


static status_t
register_device(device_node* parent)
{
	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {.string = "CCP PCI"}},
		{B_DEVICE_BUS, B_STRING_TYPE, {.string = "CCP"}},
		{B_DEVICE_FIXED_CHILD, B_STRING_TYPE, {.string = CCP_DEVICE_MODULE_NAME}},
		{}
	};

	return gDeviceManager->register_node(parent,
		CCP_PCI_DEVICE_MODULE_NAME, attrs, NULL, NULL);
}


static float
supports_device(device_node* parent)
{
	const char* bus;
	uint16 vendorID, deviceID;

	// make sure parent is a CCP PCI device node
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false)
		< B_OK || gDeviceManager->get_attr_uint16(parent, B_DEVICE_VENDOR_ID,
				&vendorID, false) < B_OK
		|| gDeviceManager->get_attr_uint16(parent, B_DEVICE_ID, &deviceID,
				false) < B_OK) {
		return -1;
	}

	if (strcmp(bus, "pci") != 0)
		return 0.0f;

	if (vendorID != 0x1022)
		return 0.0f;

	switch (deviceID) {
		case 0x1456:
		case 0x1468:
		case 0x1486:
		case 0x1537:
		case 0x15df:
			break;
		default:
			return 0.0f;
	}
#ifdef TRACE_CCP_RNG
	TRACE("CCP RNG device found! vendor 0x%04x, device 0x%04x\n", vendorID, deviceID);
#endif
	return 0.8f;
}


//	#pragma mark -


driver_module_info gCcpPciDevice = {
	{
		CCP_PCI_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	supports_device,
	register_device,
	init_device,
	uninit_device,
	NULL,	// register_child_devices,
	NULL,	// rescan
	NULL,	// device removed
};

