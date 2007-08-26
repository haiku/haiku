/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ahci_defs.h"

#include <stdlib.h>
#include <string.h>

#include <block_io.h>


#define TRACE(a...) dprintf("\33[35mahci:\33[30m " a)
#define FLOW(a...)	dprintf("ahci: " a)

#define AHCI_ID_GENERATOR "ahci/id"
#define AHCI_ID_ITEM "ahci/id"

static device_manager_info *sDeviceManager;


static status_t
register_sim(device_node_handle parent)
{
	int32 id = sDeviceManager->create_id(AHCI_ID_GENERATOR);
	if (id < 0)
		return id;

	{
		device_attr attrs[] = {
			{ B_DRIVER_MODULE, B_STRING_TYPE,
				{ string: AHCI_SIM_MODULE_NAME }},
			{ B_DRIVER_FIXED_CHILD, B_STRING_TYPE,
				{ string: SCSI_FOR_SIM_MODULE_NAME }},

			{ SCSI_DESCRIPTION_CONTROLLER_NAME, B_STRING_TYPE, 
				{ string: AHCI_DEVICE_MODULE_NAME }},
			{ B_BLOCK_DEVICE_MAX_BLOCKS_ITEM, B_UINT32_TYPE, { ui32: 255 }},
			{ AHCI_ID_ITEM, B_UINT32_TYPE, { ui32: id }},
			{ PNP_MANAGER_ID_GENERATOR, B_STRING_TYPE,
				{ string: AHCI_ID_GENERATOR }},
			{ PNP_MANAGER_AUTO_ID, B_UINT32_TYPE, { ui32: id }},

			{ NULL }
		};

		device_node_handle node;
		status_t status = sDeviceManager->register_device(parent, attrs, NULL,
			&node);
		if (status < B_OK)
			sDeviceManager->free_id(AHCI_ID_GENERATOR, id);

		return status;
	}
}


//	#pragma mark -


static float
ahci_supports_device(device_node_handle parent, bool *_noConnection)
{
	uint8 baseClass, subClass, classAPI;
	uint16 vendorID;
	uint16 deviceID;
	char *bus;

	TRACE("ahci_supports_device\n");

	if (sDeviceManager->get_attr_string(parent, B_DRIVER_BUS, &bus,
				false) != B_OK
		|| sDeviceManager->get_attr_uint8(parent,
				PCI_DEVICE_BASE_CLASS_ID_ITEM, &baseClass, false) != B_OK
		|| sDeviceManager->get_attr_uint8(parent,
				PCI_DEVICE_SUB_CLASS_ID_ITEM, &subClass, false) != B_OK
		|| sDeviceManager->get_attr_uint8(parent,
				PCI_DEVICE_API_ID_ITEM, &classAPI, false) != B_OK
		|| sDeviceManager->get_attr_uint16(parent,
				PCI_DEVICE_VENDOR_ID_ITEM, &vendorID, false) != B_OK
		|| sDeviceManager->get_attr_uint16(parent,
			PCI_DEVICE_DEVICE_ID_ITEM, &deviceID, false) != B_OK)
		return B_ERROR;

#if 1
	if (strcmp(bus, "pci") || baseClass != PCI_mass_storage
		|| subClass != PCI_sata || classAPI != PCI_sata_ahci) {
		free(bus);
		return 0.0f;
	}
#else
	if (strcmp(bus, "pci") || baseClass != PCI_mass_storage
		|| subClass != PCI_ide) {
		free(bus);
		return 0.0;
	}
#endif

	TRACE("controller found! vendor 0x%04x, device 0x%04x\n",
		vendorID, deviceID);

	free(bus);
	return 0.5;
}


static status_t
ahci_register_device(device_node_handle parent)
{
	device_node_handle node;
	pci_device *pciDevice;
	status_t status;

	device_attr attrs[] = {
		// info about ourself and our consumer
		{ B_DRIVER_MODULE, B_STRING_TYPE, { string: AHCI_DEVICE_MODULE_NAME }},

		{ SCSI_DEVICE_MAX_TARGET_COUNT, B_UINT32_TYPE,
			{ ui32: 33 }},

		// DMA properties
		// data must be word-aligned; 
		{ B_BLOCK_DEVICE_DMA_ALIGNMENT, B_UINT32_TYPE,
			{ ui32: 1 }},
		// one S/G block must not cross 64K boundary
		{ B_BLOCK_DEVICE_DMA_BOUNDARY, B_UINT32_TYPE,
			{ ui32: 0xffff }},
		// max size of S/G block is 16 bits with zero being 64K
		{ B_BLOCK_DEVICE_MAX_SG_BLOCK_SIZE, B_UINT32_TYPE,
			{ ui32: 0x10000 }},
		// see definition of MAX_SG_COUNT
		{ B_BLOCK_DEVICE_MAX_SG_BLOCKS, B_UINT32_TYPE,
			{ ui32: 32 /* whatever... */ }},
		{ NULL }
	};

	TRACE("ahci_register_device\n");

	// initialize parent (the bus) to get the PCI interface and device
	status = sDeviceManager->init_driver(parent, NULL,
			(driver_module_info **)&gPCI, (void **)&pciDevice);
	if (status != B_OK)
		return status;

	status = sDeviceManager->register_device(parent, attrs,
		NULL, &node); 
	if (status < B_OK)
		return status;

	// HACK! this device manager stuff just sucks!
	extern pci_device pd;
	pd = *pciDevice;

	// TODO: register SIM for every controller we find!
	return register_sim(node);
}


static status_t
ahci_init_driver(device_node_handle node, void *user_cookie, void **_cookie)
{
	TRACE("ahci_init_driver, user_cookie %p\n", user_cookie);
	*_cookie = (void *)0x5678;
	TRACE("cookie = %p\n", *_cookie);
	return B_OK;
}


static status_t
ahci_uninit_driver(void *cookie)
{
	TRACE("ahci_uninit_driver, cookie %p\n", cookie);
	return B_OK;
}


static void
ahci_device_removed(device_node_handle node, void *cookie)
{
	TRACE("ahci_device_removed, cookie %p\n", cookie);
}


static void
ahci_device_cleanup(device_node_handle node)
{
	TRACE("ahci_device_cleanup\n");
}


static void
ahci_get_supported_paths(const char ***_busses, const char ***_devices)
{
	static const char *kBus[] = { "pci", NULL };
	static const char *kDevice[] = { "drivers/dev/disk/sata", NULL };

	TRACE("ahci_get_supported_paths\n");

	*_busses = kBus;
	*_devices = kDevice;
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


driver_module_info sAHCIDevice = {
	{
		AHCI_DEVICE_MODULE_NAME,
		0,
		std_ops
	},
	ahci_supports_device,
	ahci_register_device,
	ahci_init_driver,
	ahci_uninit_driver,
	ahci_device_removed,
	ahci_device_cleanup,
	ahci_get_supported_paths,
};

module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager },
	{ SCSI_FOR_SIM_MODULE_NAME, (module_info **)&gSCSI },
	{}
};

module_info *modules[] = {
	(module_info *)&sAHCIDevice,
	(module_info *)&gAHCISimInterface,
	NULL
};
