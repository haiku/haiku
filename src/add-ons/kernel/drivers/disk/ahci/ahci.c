/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <KernelExport.h>
#include <device_manager.h>


#define TRACE(a...) dprintf("\33[35mahci:\33[30m " a)
#define FLOW(a...)	dprintf("ahci: " a)

static float
ahci_supports_device(device_node_handle parent, bool *_noConnection)
{
	TRACE("ahci_supports_device\n");

#if 0
	uint16 vendor_id;
	uint16 device_id;
	uint8 base_class;
	uint8 sub_class;
	uint8 class_api;
	status_t res;

	if (base_class != PCI_mass_storage || sub_class != PCI_sata || class_api != PCI_sata_ahci)
		return 0.0f;

	TRACE("controller found! vendor 0x%04x, device 0x%04x\n", vendor_id, device_id);
	
	res = pci->find_pci_capability(device, PCI_cap_id_sata, &cap_ofs);
	if (res == B_OK) {
		uint32 satacr0;
		uint32 satacr1;
		TRACE("PCI SATA capability found at offset 0x%x\n", cap_ofs);
		satacr0 = pci->read_pci_config(device, cap_ofs, 4);
		satacr1 = pci->read_pci_config(device, cap_ofs + 4, 4);
		TRACE("satacr0 = 0x%08x, satacr1 = 0x%08x\n", satacr0, satacr1);
	}
#endif

	return 0.0;
}


static status_t
ahci_register_device(device_node_handle parent)
{
	TRACE("ahci_register_device\n");
	return B_OK;
}


static status_t
ahci_init_driver(device_node_handle node, void *user_cookie, void **_cookie)
{
	TRACE("ahci_init_driver\n");
	return B_OK;
}


static status_t
ahci_uninit_driver(void *cookie)
{
	TRACE("ahci_uninit_driver\n");
	return B_OK;
}


static void
ahci_device_removed(device_node_handle node, void *cookie)
{
	TRACE("ahci_device_removed\n");
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


driver_module_info ahci_driver = {
	{
		"ahci/device_v1",
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


module_info *modules[] = {
	(module_info *)&ahci_driver,
	NULL
};
