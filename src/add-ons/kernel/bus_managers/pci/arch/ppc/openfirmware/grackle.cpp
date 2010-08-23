/*
 * Copyright 2010 Andreas Färber <andreas.faerber@web.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>

#include <platform/openfirmware/devices.h>
#include <platform/openfirmware/openfirmware.h>
#include <platform/openfirmware/pci.h>

#include "pci_controller.h"
#include "pci_io.h"
#include "pci_openfirmware_priv.h"


static status_t
grackle_read_pci_config(void *cookie, uint8 bus, uint8 device, uint8 function,
	uint8 offset, uint8 size, uint32 *value)
{
	return B_ERROR;
}


static status_t
grackle_write_pci_config(void *cookie, uint8 bus, uint8 device, uint8 function,
	uint8 offset, uint8 size, uint32 value)
{
	return B_ERROR;
}


static status_t
grackle_get_max_bus_devices(void *cookie, int32 *count)
{
	*count = 0;
	return B_OK;
}


static status_t
grackle_read_pci_irq(void *cookie, uint8 bus, uint8 device, uint8 function,
	uint8 pin, uint8 *irq)
{
	return B_ERROR;
}


static status_t
grackle_write_pci_irq(void *cookie, uint8 bus, uint8 device, uint8 function,
	uint8 pin, uint8 irq)
{
	return B_ERROR;
}


static pci_controller sGracklePCIController = {
	grackle_read_pci_config,
	grackle_write_pci_config,
	grackle_get_max_bus_devices,
	grackle_read_pci_irq,
	grackle_write_pci_irq,
};


status_t
ppc_openfirmware_probe_grackle(int deviceNode,
	const StringArrayPropertyValue &compatibleValue)
{
	if (!compatibleValue.ContainsElement("grackle"))
		return B_ERROR;

	panic("ppc_openfirmware_probe_grackle(): not implemented yet\n");

	status_t error = pci_controller_add(&sGracklePCIController, NULL);
	return error;
}
