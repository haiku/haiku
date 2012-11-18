/*
 * Copyright 2007, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "pci.h"
#include "pci_fixup.h"

#include <arch/int.h>

#include <KernelExport.h>


/*
 * We need to force controllers that have both SATA and PATA controllers to use
 * the split mode with the SATA controller at function 0 and the PATA
 * controller at function 1. This way the SATA controller will be picked up by
 * the AHCI driver and the IDE controller by the generic IDE driver.
 *
 * TODO(bga): This does not work when the SATA controller is configured for IDE
 * mode but this seems to be a problem with the device manager (it tries to load
 * the IDE driver for the AHCI controller for some reason).
 */
static void
jmicron_fixup_ahci(PCI *pci, int domain, uint8 bus, uint8 device,
	uint8 function, uint16 deviceId)
{
	// We only care about function 0.
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

	// Read controller control register (0x40).
	uint32 val = pci->ReadConfig(domain, bus, device, function, 0x40, 4);
	dprintf("jmicron_fixup_ahci: Register 0x40 : 0x%08" B_PRIx32 "\n", val);

	// Clear bits.
	val &= ~(1 << 1);
	val &= ~(1 << 9);
	val &= ~(1 << 13);
	val &= ~(1 << 15);
	val &= ~(1 << 16);
	val &= ~(1 << 17);
	val &= ~(1 << 18);
	val &= ~(1 << 19);
	val &= ~(1 << 22);

	//Set bits.
	val |= (1 << 0);
	val |= (1 << 4);
	val |= (1 << 5);
	val |= (1 << 7);
	val |= (1 << 8);
	val |= (1 << 12);
	val |= (1 << 14);
	val |= (1 << 23);

	dprintf("jmicron_fixup_ahci: Register 0x40 : 0x%08" B_PRIx32 "\n", val);
	pci->WriteConfig(domain, bus, device, function, 0x40, 4, val);

	// Read IRQ from controller at function 0 and assign this IRQ to the
	// controller at function 1.
	uint8 irq = pci->ReadConfig(domain, bus, device, function, 0x3c, 1);
	dprintf("jmicron_fixup_ahci: Assigning IRQ %d at device "
		"function 1.\n", irq);
	pci->WriteConfig(domain, bus, device, 1, 0x3c, 1, irq);
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

	dprintf("intel_fixup_ahci: 0x24: 0x%08" B_PRIx32 "\n",
		pci->ReadConfig(domain, bus, device, function, 0x24, 4));
	dprintf("intel_fixup_ahci: 0x90: 0x%02" B_PRIx32 "\n",
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
		dprintf("intel_fixup_ahci: ide-bar5 bits-1: 0x%08" B_PRIx32 "\n",
			pci->ReadConfig(domain, bus, device, function, 0x24, 4));
		pci->WriteConfig(domain, bus, device, function, 0x24, 4, 0);
		dprintf("intel_fixup_ahci: ide-bar5 bits-0: 0x%08" B_PRIx32 "\n",
			pci->ReadConfig(domain, bus, device, function, 0x24, 4));

		map &= ~0x03;
		map |= 0x40;
		pci->WriteConfig(domain, bus, device, function, 0x90, 1, map);

		pci->WriteConfig(domain, bus, device, function, 0x24, 4, 0xffffffff);
		dprintf("intel_fixup_ahci: ahci-bar5 bits-1: 0x%08" B_PRIx32 "\n",
			pci->ReadConfig(domain, bus, device, function, 0x24, 4));
		pci->WriteConfig(domain, bus, device, function, 0x24, 4, 0);
		dprintf("intel_fixup_ahci: ahci-bar5 bits-0: 0x%08" B_PRIx32 "\n",
			pci->ReadConfig(domain, bus, device, function, 0x24, 4));

		if (deviceId == 0x27c0 || deviceId == 0x27c4) // restore on ICH7
			pci->WriteConfig(domain, bus, device, function, 0x24, 4, bar5);

		pci->WriteConfig(domain, bus, device, function, PCI_command, 2, pcicmd);
	}

	dprintf("intel_fixup_ahci: 0x24: 0x%08" B_PRIx32 "\n",
		pci->ReadConfig(domain, bus, device, function, 0x24, 4));
	dprintf("intel_fixup_ahci: 0x90: 0x%02" B_PRIx32 "\n",
		pci->ReadConfig(domain, bus, device, function, 0x90, 1));
}


static void
ati_fixup_ixp(PCI *pci, int domain, uint8 bus, uint8 device, uint8 function, uint16 deviceId)
{
#if defined(__INTEL__) || defined(__x86_64__)
	/* ATI Technologies Inc, IXP chipset:
	 * This chipset seems broken, at least on my laptop I must force 
	 * the timer IRQ trigger mode, else no interrupt comes in.
	 * mmu_man.
	 */
	// XXX: should I use host or isa bridges for detection ??
	switch (deviceId) {
		// Host bridges
		case 0x5950:	// RS480 Host Bridge
		case 0x5830:
			break;
		// ISA bridges
		case 0x4377:	// IXP SB400 PCI-ISA Bridge 
		default:
			return;
	}
	dprintf("ati_fixup_ixp: domain %u, bus %u, device %u, function %u, deviceId 0x%04x\n",
		domain, bus, device, function, deviceId);

	dprintf("ati_fixup_ixp: found IXP chipset, forcing IRQ 0 as level triggered.\n");
	// XXX: maybe use pic_*() ?
	arch_int_configure_io_interrupt(0, B_LEVEL_TRIGGERED);

#endif
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

		case 0x1002:
			ati_fixup_ixp(pci, domain, bus, device, function, deviceId);
			break;
	}
}

