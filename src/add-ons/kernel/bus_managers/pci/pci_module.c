/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2003, Marcus Overhagen. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>
#include <device_manager.h>
#include <bus_manager.h>
#include <PCI.h>

#include <string.h>
#include <stdlib.h>

#include "pci_priv.h"
#include "pci_info.h"
#include "pci.h"


device_manager_info *gDeviceManager;


// name of PCI root module
#define PCI_ROOT_MODULE_NAME "bus_managers/pci/root/device_v1"


static float
pci_module_supports_device(device_node_handle parent, bool *_noConnection)
{
	char *bus;

	// make sure parent is really device root
	if (gDeviceManager->get_attr_string(parent, B_DRIVER_BUS, &bus, false))
		return B_ERROR;

	if (strcmp(bus, "root")) {
		free(bus);
		return 0.0;
	}

	free(bus);
	return 1.0;
}


static status_t 
pci_module_register_device(device_node_handle parent)
{

// XXX how do we handle this for PPC?
// I/O port for PCI config space address
#define PCI_CONFIG_ADDRESS 0xcf8

	io_resource resources[2] = {
		{ IO_PORT, PCI_CONFIG_ADDRESS, 8 },
		{}
	};
	device_attr attrs[] = {
		// info about ourself
		{ B_DRIVER_MODULE, B_STRING_TYPE, { string: PCI_ROOT_MODULE_NAME }},
		// unique connection name
		{ PNP_DRIVER_CONNECTION, B_STRING_TYPE, { string: "PCI" }},

		// mark as being a bus
		{ PNP_BUS_IS_BUS, B_UINT8_TYPE, { ui8: 1 }},
		// search for device drivers once all devices are detected
		//{ PNP_BUS_DEFER_PROBE, B_UINT8_TYPE, { ui8: 1 }},
		// don't scan if loaded as we own I/O resources
		//{ PNP_DRIVER_NO_LIVE_RESCAN, B_UINT8_TYPE, { ui8: 1 }},
		{}
	};

	io_resource_handle resourceHandles[2];
	device_node_handle node;
//	char *parent_type;
	status_t res;

	res = gDeviceManager->acquire_io_resources(resources, resourceHandles);
	if (res < B_OK)
		return res;

	return gDeviceManager->register_device(parent, attrs, resourceHandles, &node);
}


static status_t 
pci_module_register_child_devices(void *cookie)
{
	device_node_handle node = cookie;
	pci_info device;
	int32 i = 0;

	while (pci_get_nth_pci_info(i, &device) == B_OK) {
		device_attr attrs[] = {
			// info about device
			{ B_DRIVER_MODULE, B_STRING_TYPE, { string: PCI_DEVICE_MODULE_NAME }},
			{ PNP_DRIVER_CONNECTION, B_STRING_TYPE, { string: 
				"bus: %"PCI_DEVICE_BUS_ITEM
				"%, device: %"PCI_DEVICE_DEVICE_ITEM
				"%, function: %"PCI_DEVICE_FUNCTION_ITEM"%" }},

			// location on PCI bus
			{ PCI_DEVICE_BUS_ITEM, B_UINT8_TYPE, { ui8: device.bus }},
			{ PCI_DEVICE_DEVICE_ITEM, B_UINT8_TYPE, { ui8: device.device }},
			{ PCI_DEVICE_FUNCTION_ITEM, B_UINT8_TYPE, { ui8: device.function }},

			// info about the device
			{ PCI_DEVICE_VENDOR_ID_ITEM, B_UINT16_TYPE, { ui16: device.vendor_id }},
			{ PCI_DEVICE_DEVICE_ID_ITEM, B_UINT16_TYPE, { ui16: device.device_id }},
			{ PCI_DEVICE_SUBSYSTEM_ID_ITEM, B_UINT16_TYPE, { ui16: device.u.h0.subsystem_id }},
			{ PCI_DEVICE_SUBVENDOR_ID_ITEM, B_UINT16_TYPE, { ui16: device.u.h0.subsystem_vendor_id }},

			{ PCI_DEVICE_BASE_CLASS_ID_ITEM, B_UINT8_TYPE, { ui8: device.class_base }},
			{ PCI_DEVICE_SUB_CLASS_ID_ITEM, B_UINT8_TYPE, { ui8: device.class_sub }},
			{ PCI_DEVICE_API_ID_ITEM, B_UINT8_TYPE, { ui8: device.class_api }},

			// consumer specification
			{ B_DRIVER_BUS, B_STRING_TYPE, { string: "pci" }},
			// ToDo: this is a hack
			{ B_DRIVER_DEVICE_TYPE, B_STRING_TYPE, { string: device.class_base == 1 ? "drivers/dev/disk" : "drivers/dev" }},
			{ NULL }
		};
		device_node_handle deviceNode;

		gDeviceManager->register_device(node, attrs, NULL, &deviceNode);
		// currently, we don't care about loosing devices as we cannot do
		// anything about that but want to register remaining devices (if possible)

		i++;
	}

	return B_OK;
}


static void
pci_module_get_paths(const char ***_bus, const char ***_device)
{
	static const char *kBus[] = {"root", NULL};

	*_bus = kBus;
	*_device = NULL;
}


static status_t
pci_module_init(device_node_handle node, void *user_cookie, void **_cookie)
{
	*_cookie = node;
	return B_OK;
}


static int32
pci_old_module_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			status_t status;
		
			TRACE(("PCI: pci_module_init\n"));
		
			status = pci_init();
			if (status < B_OK)
				return status;
		
			pci_print_info();
		
			return B_OK;
		}

		case B_MODULE_UNINIT:
			TRACE(("PCI: pci_module_uninit\n"));
			pci_uninit();
			return B_OK;
	}

	return B_BAD_VALUE;	
}


static int32
pci_module_std_ops(int32 op, ...)
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


static struct pci_module_info sOldPCIModule = {
	{
		{
			B_PCI_MODULE_NAME,
			B_KEEP_LOADED,
			pci_old_module_std_ops
		},
		NULL
	},
	&pci_read_io_8,
	&pci_write_io_8,
	&pci_read_io_16,
	&pci_write_io_16,
	&pci_read_io_32,
	&pci_write_io_32,
	&pci_get_nth_pci_info,
	&pci_read_config,
	&pci_write_config,
	&pci_ram_address
};

static struct pci_root_info sPCIModule = {
	{
		{
			{
				PCI_ROOT_MODULE_NAME,
				0,
				pci_module_std_ops
			},

			pci_module_supports_device,
			pci_module_register_device,
			pci_module_init,
			NULL,	// uninit
			NULL,	// removed
			NULL,	// cleanup
			pci_module_get_paths,
		},

		pci_module_register_child_devices,
		NULL,		// rescan bus
	},

	&pci_read_config,
	&pci_write_config
};

module_dependency module_dependencies[] = {
	{B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&gDeviceManager},
	{}
};

module_info *modules[] = {
	(module_info *)&sOldPCIModule,
	(module_info *)&sPCIModule,
	(module_info *)&gPCIDeviceModule,
	NULL
};
