/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2003, Marcus Overhagen. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <PCI.h>
#include <PCI_x86.h>
#include "pci_msi.h"

#include "pci_private.h"
#include "pci_info.h"
#include "pci.h"


device_manager_info *gDeviceManager;

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
pci_arch_module_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			module_info *dummy;
			status_t result = get_module(B_PCI_MODULE_NAME, &dummy);
			if (result != B_OK)
				return result;

			return B_OK;
		}

		case B_MODULE_UNINIT:
			put_module(B_PCI_MODULE_NAME);
			return B_OK;
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
	&pci_ram_address,
	&pci_find_capability,
	&pci_reserve_device,
	&pci_unreserve_device,
	&pci_update_interrupt_line,
	&pci_find_extended_capability,
	&pci_get_powerstate,
	&pci_set_powerstate
};


static pci_x86_module_info sPCIArchModule = {
	{
		B_PCI_X86_MODULE_NAME,
		0,
		pci_arch_module_std_ops
	},

	&pci_get_msi_count,
	&pci_configure_msi,
	&pci_unconfigure_msi,
	&pci_enable_msi,
	&pci_disable_msi,
	&pci_get_msix_count,
	&pci_configure_msix,
	&pci_enable_msix
};


module_dependency module_dependencies[] = {
	{B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&gDeviceManager},
	{}
};

driver_module_info gPCILegacyDriverModule = {
	{
		PCI_LEGACY_DRIVER_MODULE_NAME,
		0,
		NULL,
	},
	NULL
};

module_info *modules[] = {
	(module_info *)&sOldPCIModule,
	(module_info *)&gPCIRootModule,
	(module_info *)&gPCIDeviceModule,
	(module_info *)&gPCILegacyDriverModule,
	(module_info *)&sPCIArchModule,
	NULL
};
