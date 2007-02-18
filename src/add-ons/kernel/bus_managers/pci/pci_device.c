/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002-2003, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	PCI bus manager

	PCI device layer. There is one node per PCI device.
	The common root is the PCI root node.
*/


#include <malloc.h>
#include <string.h>
#include <stdio.h>

#include "pci.h"
#include "pci_priv.h"


// information about one PCI device
typedef struct pci_device_info {
	uint8				bus;			// bus number
	uint8				device;			// device number
	uint8				function;		// function number
	device_node_handle	node;
	pci_root_info		*root;			// parent, i.e. PCI root
	char				name[32];		// name (for fast log)
} pci_device_info;


static uint8
pci_device_read_io_8(pci_device_info *device, int mapped_io_addr)
{
	return pci_read_io_8(mapped_io_addr);
}


static void 
pci_device_write_io_8(pci_device_info *device, int mapped_io_addr, uint8 value)
{
	pci_write_io_8(mapped_io_addr, value);
}


static uint16
pci_device_read_io_16(pci_device_info *device, int mapped_io_addr)
{
	return pci_read_io_16(mapped_io_addr);
}


static void
pci_device_write_io_16(pci_device_info *device, int mapped_io_addr, uint16 value)
{
	pci_write_io_16(mapped_io_addr, value);
}


static uint32
pci_device_read_io_32(pci_device_info *device, int mapped_io_addr)
{
	return pci_read_io_32(mapped_io_addr);
}

static void
pci_device_write_io_32(pci_device_info *device, int mapped_io_addr, uint32 value)
{
	pci_write_io_32(mapped_io_addr, value);
}


static uint32 
pci_device_read_pci_config(pci_device_info *device, uint8 offset, uint8 size)
{
	return device->root->read_pci_config(device->bus, device->device, 
		device->function, offset, size);
}


static void 
pci_device_write_pci_config(pci_device device, uint8 offset, uint8 size, uint32 value)
{
	return device->root->write_pci_config(device->bus, device->device, 
		device->function, offset, size, value);
}


static void *
pci_device_ram_address(pci_device_info *device, 
	const void *physical_address_in_system_memory)
{
	return pci_ram_address(physical_address_in_system_memory);
}	


static status_t
pci_device_get_pci_info(pci_device device, struct pci_info *info)
{
	int i;

	if (device == NULL || info == NULL)
		return B_BAD_VALUE;

	// TODO: Implement more efficiently!	
	for (i = 0; pci_get_nth_pci_info(i, info) == B_OK; i++) {
		if (device->bus == info->bus && device->device == info->device
				&& device->function == info->function) {
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


static status_t 
pci_device_find_capability(pci_device device, uchar cap_id, uchar *offset)
{
	return pci_find_capability(device->bus, device->device, 
		device->function, cap_id, offset);
}


static status_t
pci_device_init_driver(device_node_handle node, void *user_cookie, void **cookie)
{
	uint8 bus, deviceNumber, function;
	device_node_handle parent;
	pci_device_info *device;
	pci_root_info *root;
	status_t status;
	void *dummy;

	if (gDeviceManager->get_attr_uint8(node, PCI_DEVICE_BUS_ITEM, &bus, false) != B_OK 
		|| gDeviceManager->get_attr_uint8(node, PCI_DEVICE_DEVICE_ITEM, &deviceNumber, false) != B_OK 
		|| gDeviceManager->get_attr_uint8(node, PCI_DEVICE_FUNCTION_ITEM, &function, false) != B_OK)
		return B_ERROR;

	parent = gDeviceManager->get_parent(node);

	status = gDeviceManager->init_driver(parent, NULL, (driver_module_info **)&root, &dummy);
	if (status != B_OK)
		goto err;

	device = malloc(sizeof(*device));
	if (device == NULL) {
		status = B_NO_MEMORY;
		goto err;
	}

	gDeviceManager->put_device_node(parent);

	memset(device, 0, sizeof(*device));

	device->bus = bus;
	device->device = deviceNumber;
	device->function = function;
	device->node = node;
	device->root = root;

	snprintf(device->name, sizeof(device->name), "pci_device %u:%u:%u", 
		bus, deviceNumber, function);

	*cookie = device;
	return B_OK;

err:
	gDeviceManager->uninit_driver(parent);
	gDeviceManager->put_device_node(parent);

	return status;
}


static status_t
pci_device_uninit_driver(void *cookie)
{
	pci_device_info *device = cookie;
	device_node_handle parent;

	parent = gDeviceManager->get_parent(device->node);
	gDeviceManager->uninit_driver(parent);
	gDeviceManager->put_device_node(parent);

	free(device);
	return B_OK;
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

		NULL,		// supports device
		NULL,		// register device (our parent registered us)
		pci_device_init_driver,
		pci_device_uninit_driver,
		NULL,		// removed
		NULL,		// cleanup
		NULL,		// get supported paths
	},

	pci_device_read_io_8,
	pci_device_write_io_8,
	pci_device_read_io_16,
	pci_device_write_io_16,
	pci_device_read_io_32,
	pci_device_write_io_32,

	pci_device_read_pci_config,
	pci_device_write_pci_config,

	pci_device_ram_address,

	pci_device_get_pci_info,

	pci_device_find_capability
};
