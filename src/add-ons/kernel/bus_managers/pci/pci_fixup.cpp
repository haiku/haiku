/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

#include "pci.h"
#include "pci_fixup.h"

#include <KernelExport.h>

/* The Jmicron AHCI controller has a mode that combines IDE and AHCI functionality
 * into a single PCI device at function 0. This happens when the controller is set
 * in the BIOS to "basic" or "IDE" mode (but not in "RAID" or "AHCI" mode).
 * To avoid needing two drivers to handle a single PCI device, we switch to the 
 * multifunction (split device) AHCI mode. This will set PCI device at function 0 
 * to AHCI, and PCI device at function 1 to IDE controller.
 */
static void
jmicron_fixup_ahci(PCI *pci, int domain, uint8 bus, uint8 device, uint8 function, uint16 deviceId)
{
	if (deviceId != 0x2360 && deviceId != 0x2361 && deviceId != 0x2362 && deviceId != 0x2363 && deviceId != 0x2366)
		return;

	dprintf("jmicron_fixup_ahci: domain %u, bus %u, device %u, function %u, deviceId 0x%04x\n",
		domain, bus, device, function, deviceId);

	if (function == 0) {
		dprintf("0x40: 0x%08lx\n", pci->ReadPciConfig(domain, bus, device, function, 0x40, 4));
		dprintf("0xdc: 0x%08lx\n", pci->ReadPciConfig(domain, bus, device, function, 0xdc, 4));
		uint32 val = pci->ReadPciConfig(domain, bus, device, function, 0xdc, 4);
		if (!(val & (1 << 30))) {
			dprintf("jmicron_fixup_ahci: enabling split device mode\n");
			val &= ~(1 << 24);
			val |= (1 << 25) | (1 << 30);
			pci->WritePciConfig(domain, bus, device, function, 0xdc, 4, val);
			val = pci->ReadPciConfig(domain, bus, device, function, 0x40, 4);
			val &= ~(1 << 16);
			val |= (1 << 1) | (1 << 17) | (1 << 22);
			pci->WritePciConfig(domain, bus, device, function, 0x40, 4, val);
		}
		dprintf("0x40: 0x%08lx\n", pci->ReadPciConfig(domain, bus, device, function, 0x40, 4));
		dprintf("0xdc: 0x%08lx\n", pci->ReadPciConfig(domain, bus, device, function, 0xdc, 4));
	}
}


static void
intel_fixup_ahci(PCI *pci, int domain, uint8 bus, uint8 device, uint8 function, uint16 deviceId)
{
}


void
pci_fixup_device(PCI *pci, int domain, uint8 bus, uint8 device, uint8 function)
{
	uint16 vendorId = pci->ReadPciConfig(domain, bus, device, function, PCI_vendor_id, 2);
	uint16 deviceId = pci->ReadPciConfig(domain, bus, device, function, PCI_device_id, 2);

	switch (vendorId) {
		case 0x197b:
			jmicron_fixup_ahci(pci, domain, bus, device, function, deviceId);
			break;

		case 0x8086:
			intel_fixup_ahci(pci, domain, bus, device, function, deviceId);
			break;
	}
}

