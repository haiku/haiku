/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "ahci_defs.h"

#include <stdlib.h>
#include <string.h>

#include <block_io.h>


#define TRACE(a...) dprintf("\33[35mahci:\33[0m " a)
#define FLOW(a...)	dprintf("ahci: " a)

#define AHCI_ID_GENERATOR "ahci/id"
#define AHCI_ID_ITEM "ahci/id"

device_manager_info *gDeviceManager;


device_info sSupportedDevices[] =
{
	{ 0x1002, 0x4380, "ATI SB600" },
	{ 0x1002, 0x4390, "ATI SB700/800" },
	{ 0x1002, 0x4391, "ATI IXP700" },
	{ 0x1002, 0x4392, "ATI SB700/800" },
	{ 0x1002, 0x4393, "ATI SB700/800" },
	{ 0x1002, 0x4394, "ATI SB700/800" },
	{ 0x1002, 0x4395, "ATI SB700/800" },
	{ 0x1039, 0x1184, "SiS 966" },
	{ 0x1039, 0x1185, "SiS 966" },
	{ 0x1039, 0x0186, "SiS 968" },
	{ 0x10b9, 0x5288, "Acer Labs M5288" },
	{ 0x10de, 0x044c, "NVIDIA MCP65" },
	{ 0x10de, 0x044d, "NVIDIA MCP65" },
	{ 0x10de, 0x044e, "NVIDIA MCP65" },
	{ 0x10de, 0x044f, "NVIDIA MCP65" },
	{ 0x10de, 0x045c, "NVIDIA MCP65" },
	{ 0x10de, 0x045d, "NVIDIA MCP65" },
	{ 0x10de, 0x045e, "NVIDIA MCP65" },
	{ 0x10de, 0x045f, "NVIDIA MCP65" },
	{ 0x10de, 0x0550, "NVIDIA MCP67" },
	{ 0x10de, 0x0551, "NVIDIA MCP67" },
	{ 0x10de, 0x0552, "NVIDIA MCP67" },
	{ 0x10de, 0x0553, "NVIDIA MCP67" },
	{ 0x10de, 0x0554, "NVIDIA MCP67" },
	{ 0x10de, 0x0555, "NVIDIA MCP67" },
	{ 0x10de, 0x0556, "NVIDIA MCP67" },
	{ 0x10de, 0x0557, "NVIDIA MCP67" },
	{ 0x10de, 0x0558, "NVIDIA MCP67" },
	{ 0x10de, 0x0559, "NVIDIA MCP67" },
	{ 0x10de, 0x055a, "NVIDIA MCP67" },
	{ 0x10de, 0x055b, "NVIDIA MCP67" },
	{ 0x10de, 0x07f0, "NVIDIA MCP73" },
	{ 0x10de, 0x07f1, "NVIDIA MCP73" },
	{ 0x10de, 0x07f2, "NVIDIA MCP73" },
	{ 0x10de, 0x07f3, "NVIDIA MCP73" },
	{ 0x10de, 0x07f4, "NVIDIA MCP73" },
	{ 0x10de, 0x07f5, "NVIDIA MCP73" },
	{ 0x10de, 0x07f6, "NVIDIA MCP73" },
	{ 0x10de, 0x07f7, "NVIDIA MCP73" },
	{ 0x10de, 0x07f8, "NVIDIA MCP73" },
	{ 0x10de, 0x07f9, "NVIDIA MCP73" },
	{ 0x10de, 0x07fa, "NVIDIA MCP73" },
	{ 0x10de, 0x07fb, "NVIDIA MCP73" },
	{ 0x10de, 0x0ad0, "NVIDIA MCP77" },
	{ 0x10de, 0x0ad1, "NVIDIA MCP77" },
	{ 0x10de, 0x0ad2, "NVIDIA MCP77" },
	{ 0x10de, 0x0ad3, "NVIDIA MCP77" },
	{ 0x10de, 0x0ad4, "NVIDIA MCP77" },
	{ 0x10de, 0x0ad5, "NVIDIA MCP77" },
	{ 0x10de, 0x0ad6, "NVIDIA MCP77" },
	{ 0x10de, 0x0ad7, "NVIDIA MCP77" },
	{ 0x10de, 0x0ad8, "NVIDIA MCP77" },
	{ 0x10de, 0x0ad9, "NVIDIA MCP77" },
	{ 0x10de, 0x0ada, "NVIDIA MCP77" },
	{ 0x10de, 0x0adb, "NVIDIA MCP77" },
	{ 0x1106, 0x3349, "VIA VT8251" },
	{ 0x1106, 0x6287, "VIA VT8251" },
	{ 0x11ab, 0x6145, "Marvell 6145" },
	{ 0x197b, 0x2360, "JMicron JMB360" },
	{ 0x197b, 0x2361, "JMicron JMB361" },
	{ 0x197b, 0x2362, "JMicron JMB362" },
	{ 0x197b, 0x2363, "JMicron JMB363" },
	{ 0x197b, 0x2366, "JMicron JMB366" },
	{ 0x8086, 0x2652, "Intel ICH6R" },
	{ 0x8086, 0x2653, "Intel ICH6-M" },
	{ 0x8086, 0x2681, "Intel 63xxESB" },
	{ 0x8086, 0x2682, "Intel ESB2" },
	{ 0x8086, 0x2683, "Intel ESB2" },
	{ 0x8086, 0x27c1, "Intel ICH7R (AHCI mode)" },
	{ 0x8086, 0x27c3, "Intel ICH7R (RAID mode)" },
	{ 0x8086, 0x27c5, "Intel ICH7-M (AHCI mode)" },
	{ 0x8086, 0x27c6, "Intel ICH7-M DH (RAID mode)" },
	{ 0x8086, 0x2821, "Intel ICH8 (AHCI mode)" },
	{ 0x8086, 0x2822, "Intel ICH8R / ICH9 (RAID mode)" },
	{ 0x8086, 0x2824, "Intel ICH8 (AHCI mode)" },
	{ 0x8086, 0x2829, "Intel ICH8M (AHCI mode)" },
	{ 0x8086, 0x282a, "Intel ICH8M (RAID mode)" },
	{ 0x8086, 0x2922, "Intel ICH9 (AHCI mode)" },
	{ 0x8086, 0x2923, "Intel ICH9 (AHCI mode)" },
	{ 0x8086, 0x2924, "Intel ICH9" },
	{ 0x8086, 0x2925, "Intel ICH9" },
	{ 0x8086, 0x2927, "Intel ICH9" },
	{ 0x8086, 0x2929, "Intel ICH9M" },
	{ 0x8086, 0x292a, "Intel ICH9M" },
	{ 0x8086, 0x292b, "Intel ICH9M" },
	{ 0x8086, 0x292c, "Intel ICH9M" },
	{ 0x8086, 0x292f, "Intel ICH9M" },
	{ 0x8086, 0x294d, "Intel ICH9" },
	{ 0x8086, 0x294e, "Intel ICH9M" },
	{}
};


status_t
get_device_info(uint16 vendorID, uint16 deviceID, const char **name, uint32 *flags)
{
	device_info *info;
	for (info = sSupportedDevices; info->vendor; info++) {
		if (info->vendor == vendorID && info->device == deviceID) {
			if (name)
				*name = info->name;
			if (flags)
				*flags = info->flags;
			return B_OK;
		}
	}
	return B_ERROR;
}


static status_t
register_sim(device_node_handle parent)
{
	int32 id = gDeviceManager->create_id(AHCI_ID_GENERATOR);
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
		status_t status = gDeviceManager->register_device(parent, attrs, NULL,
			&node);
		if (status < B_OK)
			gDeviceManager->free_id(AHCI_ID_GENERATOR, id);

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
	bool isPCI;
	const char *name;
	char *bus;

	TRACE("ahci_supports_device\n");

	if (gDeviceManager->get_attr_string(parent, B_DRIVER_BUS, &bus,	false) < B_OK)
		return 0.0f;
	isPCI = !strcmp(bus, "pci");
	free(bus);
	if (!isPCI)
		return 0.0f;

	if (gDeviceManager->get_attr_uint8(parent, PCI_DEVICE_BASE_CLASS_ID_ITEM, &baseClass, false) < B_OK
		|| gDeviceManager->get_attr_uint8(parent, PCI_DEVICE_SUB_CLASS_ID_ITEM, &subClass, false) < B_OK
		|| gDeviceManager->get_attr_uint8(parent, PCI_DEVICE_API_ID_ITEM, &classAPI, false) < B_OK
		|| gDeviceManager->get_attr_uint16(parent, PCI_DEVICE_VENDOR_ID_ITEM, &vendorID, false) < B_OK
		|| gDeviceManager->get_attr_uint16(parent, PCI_DEVICE_DEVICE_ID_ITEM, &deviceID, false) < B_OK)
		return 0.0f;

	if (get_device_info(vendorID, deviceID, &name, NULL) < B_OK) {
		if (baseClass != PCI_mass_storage || subClass != PCI_sata || classAPI != PCI_sata_ahci)
			return 0.0f;
		TRACE("generic AHCI controller found! vendor 0x%04x, device 0x%04x\n", vendorID, deviceID);
		return 0.8f;
	}

	if (vendorID == PCI_VENDOR_JMICRON) {
		// JMicron uses the same device ID for SATA and PATA controllers,
		// check if BAR5 exists to determine if it's a AHCI controller
		uint8 bus, device, function;
		pci_info info;
		pci_module_info *pci;
		size_t size = 0;
		int i;
		gDeviceManager->get_attr_uint8(parent, PCI_DEVICE_BUS_ITEM, &bus, false);
		gDeviceManager->get_attr_uint8(parent, PCI_DEVICE_DEVICE_ITEM, &device, false);
		gDeviceManager->get_attr_uint8(parent, PCI_DEVICE_FUNCTION_ITEM, &function, false);
		get_module(B_PCI_MODULE_NAME, (module_info **)&pci);
		for (i = 0; B_OK == pci->get_nth_pci_info(i, &info); i++) {
			if (info.bus == bus && info.device == device && info.function == function) {
				size = info.u.h0.base_register_sizes[5];
				break;
			}
		}
		put_module(B_PCI_MODULE_NAME);
		if (size == 0)
			return 0.0f;
	}

	TRACE("AHCI controller %s found!\n", name);
	return 1.0f;
}


static status_t
ahci_register_device(device_node_handle parent)
{
	device_node_handle node;
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

	status = gDeviceManager->register_device(parent, attrs,
		NULL, &node); 
	if (status < B_OK)
		return status;

	// TODO: register SIM for every controller we find!
	return register_sim(node);
}


static status_t
ahci_init_driver(device_node_handle node, void *userCookie, void **_cookie)
{
	pci_device *_userCookie = userCookie;
	pci_device pciDevice;
	device_node_handle parent;
	status_t status;

	TRACE("ahci_init_driver, userCookie %p\n", userCookie);

	parent = gDeviceManager->get_parent(node);

	// initialize parent (the bus) to get the PCI interface and device
	status = gDeviceManager->init_driver(parent, NULL,
		(driver_module_info **)&gPCI, (void **)&pciDevice);
	if (status != B_OK)
		return status;

	TRACE("ahci_init_driver: gPCI %p, pciDevice %p\n", gPCI, pciDevice);

	gDeviceManager->put_device_node(parent);

	*_userCookie = pciDevice;
	*_cookie = node;
	return B_OK;
}


static status_t
ahci_uninit_driver(void *cookie)
{
	device_node_handle node = cookie;
	device_node_handle parent;

	TRACE("ahci_uninit_driver, cookie %p\n", cookie);

	parent = gDeviceManager->get_parent(node);
	gDeviceManager->uninit_driver(parent);
	gDeviceManager->put_device_node(parent);
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
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&gDeviceManager },
	{ SCSI_FOR_SIM_MODULE_NAME, (module_info **)&gSCSI },
	{}
};

module_info *modules[] = {
	(module_info *)&sAHCIDevice,
	(module_info *)&gAHCISimInterface,
	NULL
};
