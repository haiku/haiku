/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "pci.h"
#include "pci_fixup.h"

#include <KernelExport.h>

/* The Jmicron AHCI controller has a mode that combines IDE and AHCI
 * functionality into a single PCI device at function 0. This happens when the
 * controller is set in the BIOS to "basic" or "IDE" mode (but not in "RAID" or
 * "AHCI" mode). To avoid needing two drivers to handle a single PCI device, we
 * switch to the multifunction (split device) AHCI mode. This will set PCI
 * device at function 0 to AHCI, and PCI device at function 1 to IDE controller.
 */
static void
jmicron_fixup_ahci(PCI *pci, int domain, uint8 bus, uint8 device,
	uint8 function, uint16 deviceId)
{
	// We only care about devices with function 0.
	if (function != 0)
		return;

	// And only devices with combined SATA/PATA.
	switch (deviceId) {
		case 0x2361: // 1 SATA, 1 PATA
		case 0x2363: // 2 SATA, 1 PATA
		case 0x2366: // 2 SATA, 2 PATA
			break;
		default:
			return;
	}

	dprintf("jmicron_fixup_ahci: domain %u, bus %u, device %u, function %u, "
		"deviceId 0x%04x\n", domain, bus, device, function, deviceId);

	uint32 val = pci->ReadConfig(domain, bus, device, function, 0xdc, 4);
	if (!(val & (1 << 30))) {
		// IDE controller at function 1 is configured in IDE mode (as opposed
		// to AHCI or RAID). So we want to handle it.

		dprintf("jmicron_fixup_ahci: PATA controller in IDE mode.\n");

		// TODO(bga): It seems that with recent BIOS revisions no special code
		// is needed here. We still want to handle IRQ assignment as seen
		// below.

		// Read IRQ from controller at function 0 and assign this IRQ to the
		// controller at function 1.
		uint8 irq = pci->ReadConfig(domain, bus, device, function, 0x3c, 1);
		pci->WriteConfig(domain, bus, device, 1, 0x3c, 1, irq);
	} else {
		// TODO(bga): If the PATA controller is set to AHCI mode, the IDE
		// driver will try to pick it up and will fail either because there is
		// no assigned IRQ or, if we assign an IRQ, because it errors-out when
		// detecting devices (probably because of the AHCI mode). Then the AHCI
		// driver picks the device up but fail to find the attached devices.
		// Maybe fixing this would be as simple as changing the device class to
		// Serial ATA Controller instead of IDE Controller (even in AHCI mode
		// it reports being a standard IDE controller)?
	}
}


static void
intel_fixup_ahci(PCI *pci, int domain, uint8 bus, uint8 device, uint8 function,
	uint16 deviceId)
{
	// TODO(bga): disabled until the PCI manager can assign new resources.
	return;

	switch (deviceId) {
		case 0x2825: // ICH8 Desktop when in IDE emulation mode
			dprintf("intel_fixup_ahci: WARNING found ICH8 device id 0x2825\n");
			return;
		case 0x2926: // ICH9 Desktop when in IDE emulation mode
			dprintf("intel_fixup_ahci: WARNING found ICH9 device id 0x2926\n");
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

	dprintf("intel_fixup_ahci: domain %u, bus %u, device %u, function %u, "
		"deviceId 0x%04x\n", domain, bus, device, function, deviceId);

	dprintf("intel_fixup_ahci: 0x24: 0x%08lx\n",
		pci->ReadConfig(domain, bus, device, function, 0x24, 4));
	dprintf("intel_fixup_ahci: 0x90: 0x%02lx\n",
		pci->ReadConfig(domain, bus, device, function, 0x90, 1));

	uint8 map = pci->ReadConfig(domain, bus, device, function, 0x90, 1);
	if ((map >> 6) == 0) {
		uint32 bar5 = pci->ReadConfig(domain, bus, device, function, 0x24, 4);
		uint16 pcicmd = pci->ReadConfig(domain, bus, device, function,
			PCI_command, 2);

		dprintf("intel_fixup_ahci: switching from IDE to AHCI mode\n");

		pci->WriteConfig(domain, bus, device, function, PCI_command, 2,
			pcicmd & ~(PCI_command_io | PCI_command_memory));

		pci->WriteConfig(domain, bus, device, function, 0x24, 4, 0xffffffff);
		dprintf("intel_fixup_ahci: ide-bar5 bits-1: 0x%08lx\n",
			pci->ReadConfig(domain, bus, device, function, 0x24, 4));
		pci->WriteConfig(domain, bus, device, function, 0x24, 4, 0);
		dprintf("intel_fixup_ahci: ide-bar5 bits-0: 0x%08lx\n",
			pci->ReadConfig(domain, bus, device, function, 0x24, 4));

		map &= ~0x03;
		map |= 0x40;
		pci->WriteConfig(domain, bus, device, function, 0x90, 1, map);

		pci->WriteConfig(domain, bus, device, function, 0x24, 4, 0xffffffff);
		dprintf("intel_fixup_ahci: ahci-bar5 bits-1: 0x%08lx\n",
			pci->ReadConfig(domain, bus, device, function, 0x24, 4));
		pci->WriteConfig(domain, bus, device, function, 0x24, 4, 0);
		dprintf("intel_fixup_ahci: ahci-bar5 bits-0: 0x%08lx\n",
			pci->ReadConfig(domain, bus, device, function, 0x24, 4));

		if (deviceId == 0x27c0 || deviceId == 0x27c4) // restore on ICH7
			pci->WriteConfig(domain, bus, device, function, 0x24, 4, bar5);

		pci->WriteConfig(domain, bus, device, function, PCI_command, 2, pcicmd);
	}

	dprintf("intel_fixup_ahci: 0x24: 0x%08lx\n",
		pci->ReadConfig(domain, bus, device, function, 0x24, 4));
	dprintf("intel_fixup_ahci: 0x90: 0x%02lx\n",
		pci->ReadConfig(domain, bus, device, function, 0x90, 1));
}


void
pci_fixup_device(PCI *pci, int domain, uint8 bus, uint8 device, uint8 function)
{
	uint16 vendorId = pci->ReadConfig(domain, bus, device, function,
		PCI_vendor_id, 2);
	uint16 deviceId = pci->ReadConfig(domain, bus, device, function,
		PCI_device_id, 2);

//	dprintf("pci_fixup_device: domain %u, bus %u, device %u, function %u\n",
//		domain, bus, device, function);

	switch (vendorId) {
		case 0x197b:
			jmicron_fixup_ahci(pci, domain, bus, device, function, deviceId);
			break;

		case 0x8086:
			intel_fixup_ahci(pci, domain, bus, device, function, deviceId);
			break;
	}
}

