/*
 * Copyright 2009-2022, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */

#include "pci_controller.h"

#include <kernel/debug.h>


extern addr_t gPCIeBase;
extern uint8 gStartBusNumber;
extern uint8 gEndBusNumber;


#define PCIE_VADDR(base, bus, slot, func, reg) \
	((base) + ((((bus) & 0xff) << 20) | (((slot) & 0x1f) << 15) \
		| (((func) & 0x7) << 12) | ((reg) & 0xfff)))


static status_t
read_pci_config(void *cookie, uint8 bus, uint8 device, uint8 function,
					  uint16 offset, uint8 size, uint32 *value)
{
	if (!(bus >= gStartBusNumber && bus <= gEndBusNumber))
		return B_ERROR;

	status_t status = B_OK;

	addr_t address = PCIE_VADDR(gPCIeBase, bus - gStartBusNumber, device, function, offset);

	switch (size) {
		case 1:
			*value = *(uint8*)address;
			break;
		case 2:
			*value = *(uint16*)address;
			break;
		case 4:
			*value = *(uint32*)address;
			break;
		default:
			status = B_ERROR;
			break;
	}
	return status;
}


static status_t
write_pci_config(void *cookie, uint8 bus, uint8 device, uint8 function,
					   uint16 offset, uint8 size, uint32 value)
{
	if (!(bus >= gStartBusNumber && bus <= gEndBusNumber))
		return B_ERROR;

	status_t status = B_OK;

	addr_t address = PCIE_VADDR(gPCIeBase, bus - gStartBusNumber, device, function, offset);
	switch (size) {
		case 1:
			*(uint8*)address = value;
			break;
		case 2:
			*(uint16*)address = value;
			break;
		case 4:
			*(uint32*)address = value;
			break;
		default:
			status = B_ERROR;
			break;
	}

	return status;
}


static status_t
get_max_bus_devices(void *cookie, int32 *count)
{
	*count = 32;
	return B_OK;
}


static status_t
read_pci_irq(void *cookie, uint8 bus, uint8 device, uint8 function, uint8 pin,
	uint8 *irq)
{
	dprintf("read_pci_irq\n");
	return B_NOT_SUPPORTED;
}


static status_t
write_pci_irq(void *cookie, uint8 bus, uint8 device, uint8 function, uint8 pin,
	uint8 irq)
{
	dprintf("write_pci_irq\n");
	return B_NOT_SUPPORTED;
}


pci_controller pci_controller_ecam =
{
	read_pci_config,
	write_pci_config,
	get_max_bus_devices,
	read_pci_irq,
	write_pci_irq,
};
