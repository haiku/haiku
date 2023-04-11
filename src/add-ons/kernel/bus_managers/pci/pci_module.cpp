/*
 * Copyright 2005-2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2003, Marcus Overhagen. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <PCI.h>
#include "pci_msi.h"

#include "pci_private.h"
#include "pci_info.h"
#include "pci.h"

#define CHECK_RET(err) {status_t _err = (err); if (_err < B_OK) return _err;}


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


static status_t
ResolveBDF(uint8 virtualBus, uint8 device, uint8 function, PCIDev*& dev)
{
	uint8 bus;
	uint8 domain;
	status_t result = gPCI->ResolveVirtualBus(virtualBus, &domain, &bus);
	if (result != B_OK)
		return result;

	dev = gPCI->FindDevice(domain, bus, device, function);
	if (dev == NULL)
		return B_ERROR;

	return B_OK;
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
	.read_io_8 = pci_read_io_8,
	.write_io_8 = pci_write_io_8,
	.read_io_16 = pci_read_io_16,
	.write_io_16 = pci_write_io_16,
	.read_io_32 = pci_read_io_32,
	.write_io_32 = pci_write_io_32,
	.get_nth_pci_info = pci_get_nth_pci_info,
	.read_pci_config = pci_read_config,
	.write_pci_config = pci_write_config,
	.ram_address = pci_ram_address,
	.find_pci_capability = pci_find_capability,
	.reserve_device = pci_reserve_device,
	.unreserve_device = pci_unreserve_device,
	.update_interrupt_line = pci_update_interrupt_line,
	.find_pci_extended_capability = pci_find_extended_capability,
	.get_powerstate = pci_get_powerstate,
	.set_powerstate = pci_set_powerstate,

	.get_msi_count = [](uint8 bus, uint8 device, uint8 function) {
		PCIDev* dev;
		if (ResolveBDF(bus, device, function, dev) < B_OK)
			return (uint8)0;
		return gPCI->GetMSICount(dev);
	},
	.configure_msi = [](uint8 bus, uint8 device, uint8 function, uint8 count, uint8 *startVector) {
		PCIDev* dev;
		CHECK_RET(ResolveBDF(bus, device, function, dev));
		return gPCI->ConfigureMSI(dev, count, startVector);
	},
	.unconfigure_msi = [](uint8 bus, uint8 device, uint8 function) {
		PCIDev* dev;
		CHECK_RET(ResolveBDF(bus, device, function, dev));
		return gPCI->UnconfigureMSI(dev);
	},
	.enable_msi = [](uint8 bus, uint8 device, uint8 function) {
		PCIDev* dev;
		CHECK_RET(ResolveBDF(bus, device, function, dev));
		return gPCI->EnableMSI(dev);
	},
	.disable_msi = [](uint8 bus, uint8 device, uint8 function) {
		PCIDev* dev;
		CHECK_RET(ResolveBDF(bus, device, function, dev));
		return gPCI->DisableMSI(dev);
	},
	.get_msix_count = [](uint8 bus, uint8 device, uint8 function) {
		PCIDev* dev;
		if (ResolveBDF(bus, device, function, dev) < B_OK)
			return (uint8)0;
		return gPCI->GetMSIXCount(dev);
	},
	.configure_msix = [](uint8 bus, uint8 device, uint8 function, uint8 count, uint8 *startVector) {
		PCIDev* dev;
		CHECK_RET(ResolveBDF(bus, device, function, dev));
		return gPCI->ConfigureMSIX(dev, count, startVector);
	},
	.enable_msix = [](uint8 bus, uint8 device, uint8 function) {
		PCIDev* dev;
		CHECK_RET(ResolveBDF(bus, device, function, dev));
		return gPCI->EnableMSIX(dev);
	}
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
	NULL
};
