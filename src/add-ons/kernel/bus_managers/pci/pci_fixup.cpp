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
	switch (deviceId) {
		case 0x2360:
		case 0x2361:
		case 0x2362:
		case 0x2363:
		case 0x2366:
			break;
		default:
			return;
	}

	dprintf("jmicron_fixup_ahci: domain %u, bus %u, device %u, function %u, deviceId 0x%04x\n",
		domain, bus, device, function, deviceId);

	if (function == 0) {
		dprintf("jmicron_fixup_ahci: 0x40: 0x%08lx\n", pci->ReadPciConfig(domain, bus, device, function, 0x40, 4));
		dprintf("jmicron_fixup_ahci: 0xdc: 0x%08lx\n", pci->ReadPciConfig(domain, bus, device, function, 0xdc, 4));
		uint32 val = pci->ReadPciConfig(domain, bus, device, function, 0xdc, 4);
		if (!(val & (1 << 30))) {
			uint8 irq = pci->ReadPciConfig(domain, bus, device, function, 0x3c, 1);
			dprintf("jmicron_fixup_ahci: enabling split device mode\n");
			val &= ~(1 << 24);
			val |= (1 << 25) | (1 << 30);
			pci->WritePciConfig(domain, bus, device, function, 0xdc, 4, val);
			val = pci->ReadPciConfig(domain, bus, device, function, 0x40, 4);
			val &= ~(1 << 16);
			val |= (1 << 1) | (1 << 17) | (1 << 22);
			pci->WritePciConfig(domain, bus, device, function, 0x40, 4, val);
			pci->WritePciConfig(domain, bus, device, function, 0x3c, 1, irq);
		}
		dprintf("jmicron_fixup_ahci: 0x40: 0x%08lx\n", pci->ReadPciConfig(domain, bus, device, function, 0x40, 4));
		dprintf("jmicron_fixup_ahci: 0xdc: 0x%08lx\n", pci->ReadPciConfig(domain, bus, device, function, 0xdc, 4));
	}
}


static void
intel_fixup_ahci(PCI *pci, int domain, uint8 bus, uint8 device, uint8 function, uint16 deviceId)
{
	switch (deviceId) {
		case 0x2825: // ICH8 Desktop when in IDE emulation mode
			dprintf("intel_fixup_ahci: WARNING found ICH8 device id 0x2825");
			return;
		case 0x2926: // ICH9 Desktop when in IDE emulation mode
			dprintf("intel_fixup_ahci: WARNING found ICH9 device id 0x2926");
			return;

		case 0x27c0: // ICH7 Desktop non-AHCI and non-RAID mode
		case 0x27c4: // ICH7 Mobile non-AHCI and non-RAID mode
		case 0x2820: // ICH8 Desktop non-AHCI and non-RAID mode
		case 0x2828: // ICH8 Mobile non-AHCI and non-RAID mode
		case 0x2920: // ICH9 Desktop non-AHCI and non-RAID mode (4 ports)
		case 0x2921: // ICH9 Desktop non-AHCI and non-RAID mode (2 ports)
			break;
		default:
			return;
	}

	dprintf("intel_fixup_ahci: domain %u, bus %u, device %u, function %u, deviceId 0x%04x\n",
		domain, bus, device, function, deviceId);

	dprintf("intel_fixup_ahci: 0x24: 0x%08lx\n", pci->ReadPciConfig(domain, bus, device, function, 0x24, 4));
	dprintf("intel_fixup_ahci: 0x90: 0x%02lx\n", pci->ReadPciConfig(domain, bus, device, function, 0x90, 1));

	uint8 map = pci->ReadPciConfig(domain, bus, device, function, 0x90, 1);
	if ((map >> 6) == 0) {
		uint32 bar5 = pci->ReadPciConfig(domain, bus, device, function, 0x24, 4);
		uint16 pcicmd = pci->ReadPciConfig(domain, bus, device, function, PCI_command, 2);

		dprintf("intel_fixup_ahci: switching from IDE to AHCI mode\n");

		pci->WritePciConfig(domain, bus, device, function, PCI_command, 2, 
			pcicmd & ~(PCI_command_io | PCI_command_memory));

		pci->WritePciConfig(domain, bus, device, function, 0x24, 4, 0xffffffff);
		dprintf("intel_fixup_ahci: ide-bar5 bits-1: 0x%08lx\n", pci->ReadPciConfig(domain, bus, device, function, 0x24, 4));
		pci->WritePciConfig(domain, bus, device, function, 0x24, 4, 0);
		dprintf("intel_fixup_ahci: ide-bar5 bits-0: 0x%08lx\n", pci->ReadPciConfig(domain, bus, device, function, 0x24, 4));

		map &= ~0x03;
		map |= 0x40;
		pci->WritePciConfig(domain, bus, device, function, 0x90, 1, map);

		pci->WritePciConfig(domain, bus, device, function, 0x24, 4, 0xffffffff);
		dprintf("intel_fixup_ahci: ahci-bar5 bits-1: 0x%08lx\n", pci->ReadPciConfig(domain, bus, device, function, 0x24, 4));
		pci->WritePciConfig(domain, bus, device, function, 0x24, 4, 0);
		dprintf("intel_fixup_ahci: ahci-bar5 bits-0: 0x%08lx\n", pci->ReadPciConfig(domain, bus, device, function, 0x24, 4));

		if (deviceId == 0x27c0 || deviceId == 0x27c4) // restore on ICH7
			pci->WritePciConfig(domain, bus, device, function, 0x24, 4, bar5);

		pci->WritePciConfig(domain, bus, device, function, PCI_command, 2, pcicmd);
	}

	dprintf("intel_fixup_ahci: 0x24: 0x%08lx\n", pci->ReadPciConfig(domain, bus, device, function, 0x24, 4));
	dprintf("intel_fixup_ahci: 0x90: 0x%02lx\n", pci->ReadPciConfig(domain, bus, device, function, 0x90, 1));
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

