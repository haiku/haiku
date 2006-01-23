/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2003, Marcus Overhagen. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>
#define __HAIKU_PCI_BUS_MANAGER_TESTING 1
#include <PCI.h>

#include "util/kernel_cpp.h"
#include "pci_priv.h"
#include "pci.h"


bool gIrqRouterAvailable = false;
spinlock gConfigLock = 0;

static PCI *sPCI;


status_t
pci_init(void)
{
	if (pci_io_init() != B_OK) {
		TRACE(("PCI: pci_io_init failed\n"));
		return B_ERROR;
	}

	if (pci_config_init() != B_OK) {
		TRACE(("PCI: pci_config_init failed\n"));
		return B_ERROR;
	}

	if (pci_irq_init() != B_OK)
		TRACE(("PCI: IRQ router not available\n"));
	else
		gIrqRouterAvailable = true;

	sPCI = new PCI;

	return B_OK;
}


void
pci_uninit(void)
{
	delete sPCI;
}


long
pci_get_nth_pci_info(long index, pci_info *outInfo)
{
	return sPCI->GetNthPciInfo(index, outInfo);
}


PCI::PCI()
{
	fRootBus.parent = 0;
	fRootBus.bus = 0;
	fRootBus.child = 0;
	
	DiscoverBus(&fRootBus);

	RefreshDeviceInfo(&fRootBus);
}


PCI::~PCI()
{
}


status_t
PCI::GetNthPciInfo(long index, pci_info *outInfo)
{
	long curindex = 0;
	return GetNthPciInfo(&fRootBus, &curindex, index, outInfo);
}


status_t
PCI::GetNthPciInfo(PCIBus *bus, long *curindex, long wantindex, pci_info *outInfo)
{
	// maps tree structure to linear indexed view
	PCIDev *dev = bus->child;
	while (dev) {
		if (*curindex == wantindex) {
			*outInfo = dev->info;
			return B_OK;
		}
		*curindex += 1;
		if (dev->child && B_OK == GetNthPciInfo(dev->child, curindex, wantindex, outInfo))
			return B_OK;
		dev = dev->next;
	}
	return B_ERROR;
}


void
PCI::DiscoverBus(PCIBus *bus)
{
	TRACE(("PCI: DiscoverBus, bus %u\n", bus->bus));

	for (int dev = 0; dev < gMaxBusDevices; dev++) {
		uint16 vendor_id = pci_read_config(bus->bus, dev, 0, PCI_vendor_id, 2);
		if (vendor_id == 0xffff)
			continue;
		
		uint8 type = pci_read_config(bus->bus, dev, 0, PCI_header_type, 1);
		int nfunc = (type & PCI_multifunction) ? 8 : 1;
		for (int func = 0; func < nfunc; func++)
			DiscoverDevice(bus, dev, func);
	}
}


void
PCI::DiscoverDevice(PCIBus *bus, uint8 dev, uint8 func)
{
	TRACE(("PCI: DiscoverDevice, bus %u, dev %u, func %u\n", bus->bus, dev, func));

	uint16 device_id = pci_read_config(bus->bus, dev, func, PCI_device_id, 2);
	if (device_id == 0xffff)
		return;

	PCIDev *newdev = CreateDevice(bus, dev, func);

	uint8 base_class = pci_read_config(bus->bus, dev, func, PCI_class_base, 1);
	uint8 sub_class	 = pci_read_config(bus->bus, dev, func, PCI_class_sub, 1);
	if (base_class == PCI_bridge && sub_class == PCI_pci) {
		uint8 secondary_bus = pci_read_config(bus->bus, dev, func, PCI_secondary_bus, 1);
		PCIBus *newbus = CreateBus(newdev, secondary_bus);
		DiscoverBus(newbus);
	}
}


PCIBus *
PCI::CreateBus(PCIDev *parent, uint8 bus)
{
	PCIBus *newbus = new PCIBus;
	newbus->parent = parent;
	newbus->bus = bus;
	newbus->child = 0;
	
	// append
	parent->child = newbus;
	
	return newbus;
}

	
PCIDev *
PCI::CreateDevice(PCIBus *parent, uint8 dev, uint8 func)
{
	TRACE(("PCI: CreateDevice, bus %u, dev %u, func %u:\n", parent->bus, dev, func));
	
	PCIDev *newdev = new PCIDev;
	newdev->next = 0;
	newdev->parent = parent;
	newdev->child = 0;
	newdev->bus = parent->bus;
	newdev->dev = dev;
	newdev->func = func;

	ReadPciBasicInfo(newdev);

	TRACE(("PCI: vendor 0x%04x, device 0x%04x, class_base 0x%02x, class_sub 0x%02x\n",
		newdev->info.vendor_id, newdev->info.device_id, newdev->info.class_base, newdev->info.class_sub));

	// append
	if (parent->child == 0) {
		parent->child = newdev;
	} else {
		PCIDev *sub = parent->child;
		while (sub->next)
			sub = sub->next;
		sub->next = newdev;
	}
	
	return newdev;
}


uint32
PCI::BarSize(uint32 bits, uint32 mask)
{
	bits &= mask;
	if (!bits)
		return 0;
	uint32 size = 1;
	while (!(bits & size))
		size <<= 1;
	return size;
}


void
PCI::GetBarInfo(PCIDev *dev, uint8 offset, uint32 *address, uint32 *size, uint8 *flags)
{
	uint32 oldvalue = pci_read_config(dev->bus, dev->dev, dev->func, offset, 4);
	pci_write_config(dev->bus, dev->dev, dev->func, offset, 4, 0xffffffff);
	uint32 newvalue = pci_read_config(dev->bus, dev->dev, dev->func, offset, 4);
	pci_write_config(dev->bus, dev->dev, dev->func, offset, 4, oldvalue);
	
	*address = oldvalue & PCI_address_memory_32_mask;
	if (size)
		*size = BarSize(newvalue, PCI_address_memory_32_mask);
	if (flags)
		*flags = newvalue & 0xf;
}


void
PCI::GetRomBarInfo(PCIDev *dev, uint8 offset, uint32 *address, uint32 *size, uint8 *flags)
{
	uint32 oldvalue = pci_read_config(dev->bus, dev->dev, dev->func, offset, 4);
	pci_write_config(dev->bus, dev->dev, dev->func, offset, 4, 0xfffffffe); // LSB must be 0
	uint32 newvalue = pci_read_config(dev->bus, dev->dev, dev->func, offset, 4);
	pci_write_config(dev->bus, dev->dev, dev->func, offset, 4, oldvalue);
	
	*address = oldvalue & PCI_rom_address_mask;
	if (size)
		*size = BarSize(newvalue, PCI_rom_address_mask);
	if (flags)
		*flags = newvalue & 0xf;
}


void
PCI::ReadPciBasicInfo(PCIDev *dev)
{
	dev->info.vendor_id = pci_read_config(dev->bus, dev->dev, dev->func, PCI_vendor_id, 2);
	dev->info.device_id = pci_read_config(dev->bus, dev->dev, dev->func, PCI_device_id, 2);
	dev->info.bus = dev->bus;
	dev->info.device = dev->dev;
	dev->info.function = dev->func;
	dev->info.revision = pci_read_config(dev->bus, dev->dev, dev->func, PCI_revision, 1);
	dev->info.class_api = pci_read_config(dev->bus, dev->dev, dev->func, PCI_class_api, 1);
	dev->info.class_sub = pci_read_config(dev->bus, dev->dev, dev->func, PCI_class_sub, 1);
	dev->info.class_base = pci_read_config(dev->bus, dev->dev, dev->func, PCI_class_base, 1);
	dev->info.line_size = pci_read_config(dev->bus, dev->dev, dev->func, PCI_line_size, 1);
	dev->info.latency = pci_read_config(dev->bus, dev->dev, dev->func, PCI_latency, 1);
	// BeOS does not mask off the multifunction bit, developer must use (header_type & PCI_header_type_mask)
	dev->info.header_type = pci_read_config(dev->bus, dev->dev, dev->func, PCI_header_type, 1);
	dev->info.bist = pci_read_config(dev->bus, dev->dev, dev->func, PCI_bist, 1);
	dev->info.reserved = 0;
}


void
PCI::ReadPciHeaderInfo(PCIDev *dev)
{
	switch (dev->info.header_type & PCI_header_type_mask) {
		case PCI_header_type_generic:
		{
			// disable PCI device address decoding (io and memory) while BARs are modified
			uint16 pcicmd = pci_read_config(dev->bus, dev->dev, dev->func, PCI_command, 2);
			pci_write_config(dev->bus, dev->dev, dev->func, PCI_command, 2, pcicmd & ~(PCI_command_io | PCI_command_memory));

			// get BAR size infos			
			GetRomBarInfo(dev, PCI_rom_base, &dev->info.u.h0.rom_base_pci, &dev->info.u.h0.rom_size);
			for (int i = 0; i < 6; i++) {
				GetBarInfo(dev, PCI_base_registers + 4*i,
					&dev->info.u.h0.base_registers_pci[i],
					&dev->info.u.h0.base_register_sizes[i],
					&dev->info.u.h0.base_register_flags[i]);
			}
			
			// restore PCI device address decoding
			pci_write_config(dev->bus, dev->dev, dev->func, PCI_command, 2, pcicmd);

			dev->info.u.h0.rom_base = (ulong)pci_ram_address((void *)dev->info.u.h0.rom_base_pci);
			for (int i = 0; i < 6; i++) {
				dev->info.u.h0.base_registers[i] = (ulong)pci_ram_address((void *)dev->info.u.h0.base_registers_pci[i]);
			}
			
			dev->info.u.h0.cardbus_cis = pci_read_config(dev->bus, dev->dev, dev->func, PCI_cardbus_cis, 4);
			dev->info.u.h0.subsystem_id = pci_read_config(dev->bus, dev->dev, dev->func, PCI_subsystem_id, 2);
			dev->info.u.h0.subsystem_vendor_id = pci_read_config(dev->bus, dev->dev, dev->func, PCI_subsystem_vendor_id, 2);
			dev->info.u.h0.interrupt_line = pci_read_config(dev->bus, dev->dev, dev->func, PCI_interrupt_line, 1);
			dev->info.u.h0.interrupt_pin = pci_read_config(dev->bus, dev->dev, dev->func, PCI_interrupt_pin, 1);
			dev->info.u.h0.min_grant = pci_read_config(dev->bus, dev->dev, dev->func, PCI_min_grant, 1);
			dev->info.u.h0.max_latency = pci_read_config(dev->bus, dev->dev, dev->func, PCI_max_latency, 1);
			break;
		}

		case PCI_header_type_PCI_to_PCI_bridge:
		{
			// disable PCI device address decoding (io and memory) while BARs are modified
			uint16 pcicmd = pci_read_config(dev->bus, dev->dev, dev->func, PCI_command, 2);
			pci_write_config(dev->bus, dev->dev, dev->func, PCI_command, 2, pcicmd & ~(PCI_command_io | PCI_command_memory));

			GetRomBarInfo(dev, PCI_bridge_rom_base, &dev->info.u.h1.rom_base_pci);
			for (int i = 0; i < 2; i++) {
				GetBarInfo(dev, PCI_base_registers + 4*i,
					&dev->info.u.h1.base_registers_pci[i],
					&dev->info.u.h1.base_register_sizes[i],
					&dev->info.u.h1.base_register_flags[i]);
			}

			// restore PCI device address decoding
			pci_write_config(dev->bus, dev->dev, dev->func, PCI_command, 2, pcicmd);

			dev->info.u.h1.rom_base = (ulong)pci_ram_address((void *)dev->info.u.h1.rom_base_pci);
			for (int i = 0; i < 2; i++) {
				dev->info.u.h1.base_registers[i] = (ulong)pci_ram_address((void *)dev->info.u.h1.base_registers_pci[i]);
			}

			dev->info.u.h1.primary_bus = pci_read_config(dev->bus, dev->dev, dev->func, PCI_primary_bus, 1);
			dev->info.u.h1.secondary_bus = pci_read_config(dev->bus, dev->dev, dev->func, PCI_secondary_bus, 1);
			dev->info.u.h1.subordinate_bus = pci_read_config(dev->bus, dev->dev, dev->func, PCI_subordinate_bus, 1);
			dev->info.u.h1.secondary_latency = pci_read_config(dev->bus, dev->dev, dev->func, PCI_secondary_latency, 1);
			dev->info.u.h1.io_base = pci_read_config(dev->bus, dev->dev, dev->func, PCI_io_base, 1);
			dev->info.u.h1.io_limit = pci_read_config(dev->bus, dev->dev, dev->func, PCI_io_limit, 1);
			dev->info.u.h1.secondary_status = pci_read_config(dev->bus, dev->dev, dev->func, PCI_secondary_status, 2);
			dev->info.u.h1.memory_base = pci_read_config(dev->bus, dev->dev, dev->func, PCI_memory_base, 2);
			dev->info.u.h1.memory_limit = pci_read_config(dev->bus, dev->dev, dev->func, PCI_memory_limit, 2);
			dev->info.u.h1.prefetchable_memory_base = pci_read_config(dev->bus, dev->dev, dev->func, PCI_prefetchable_memory_base, 2);
			dev->info.u.h1.prefetchable_memory_limit = pci_read_config(dev->bus, dev->dev, dev->func, PCI_prefetchable_memory_limit, 2);
			dev->info.u.h1.prefetchable_memory_base_upper32 = pci_read_config(dev->bus, dev->dev, dev->func, PCI_prefetchable_memory_base_upper32, 4);
			dev->info.u.h1.prefetchable_memory_limit_upper32 = pci_read_config(dev->bus, dev->dev, dev->func, PCI_prefetchable_memory_limit_upper32, 4);
			dev->info.u.h1.io_base_upper16 = pci_read_config(dev->bus, dev->dev, dev->func, PCI_io_base_upper16, 2);
			dev->info.u.h1.io_limit_upper16 = pci_read_config(dev->bus, dev->dev, dev->func, PCI_io_limit_upper16, 2);
			dev->info.u.h1.interrupt_line = pci_read_config(dev->bus, dev->dev, dev->func, PCI_interrupt_line, 1);
			dev->info.u.h1.interrupt_pin = pci_read_config(dev->bus, dev->dev, dev->func, PCI_interrupt_pin, 1);
			dev->info.u.h1.bridge_control = pci_read_config(dev->bus, dev->dev, dev->func, PCI_bridge_control, 2);	
			dev->info.u.h1.subsystem_id = pci_read_config(dev->bus, dev->dev, dev->func, PCI_sub_device_id_1, 2);
			dev->info.u.h1.subsystem_vendor_id = pci_read_config(dev->bus, dev->dev, dev->func, PCI_sub_vendor_id_1, 2);
			break;
		}
		
		case PCI_header_type_cardbus:
		{
			// for testing only, not final:
			dev->info.u.h2.subsystem_id = pci_read_config(dev->bus, dev->dev, dev->func, PCI_sub_device_id_2, 2);
			dev->info.u.h2.subsystem_vendor_id = pci_read_config(dev->bus, dev->dev, dev->func, PCI_sub_vendor_id_2, 2);
			dev->info.u.h2.primary_bus = pci_read_config(dev->bus, dev->dev, dev->func, PCI_primary_bus_2, 1);
			dev->info.u.h2.secondary_bus = pci_read_config(dev->bus, dev->dev, dev->func, PCI_secondary_bus_2, 1);
			dev->info.u.h2.subordinate_bus = pci_read_config(dev->bus, dev->dev, dev->func, PCI_subordinate_bus_2, 1);
			dev->info.u.h2.secondary_latency = pci_read_config(dev->bus, dev->dev, dev->func, PCI_secondary_latency_2, 1);
			dev->info.u.h2.reserved = 0;
			dev->info.u.h2.memory_base = pci_read_config(dev->bus, dev->dev, dev->func, PCI_memory_base0_2, 4);
			dev->info.u.h2.memory_limit = pci_read_config(dev->bus, dev->dev, dev->func, PCI_memory_limit0_2, 4);
			dev->info.u.h2.memory_base_upper32 = pci_read_config(dev->bus, dev->dev, dev->func, PCI_memory_base1_2, 4);
			dev->info.u.h2.memory_limit_upper32 = pci_read_config(dev->bus, dev->dev, dev->func, PCI_memory_limit1_2, 4);
			dev->info.u.h2.io_base = pci_read_config(dev->bus, dev->dev, dev->func, PCI_io_base0_2, 4);
			dev->info.u.h2.io_limit = pci_read_config(dev->bus, dev->dev, dev->func, PCI_io_limit0_2, 4);
			dev->info.u.h2.io_base_upper32 = pci_read_config(dev->bus, dev->dev, dev->func, PCI_io_base1_2, 4);
			dev->info.u.h2.io_limit_upper32 = pci_read_config(dev->bus, dev->dev, dev->func, PCI_io_limit1_2, 4);
			dev->info.u.h2.secondary_status = pci_read_config(dev->bus, dev->dev, dev->func, PCI_secondary_status_2, 2);
			dev->info.u.h2.bridge_control = pci_read_config(dev->bus, dev->dev, dev->func, PCI_bridge_control_2, 1);
			break;
		}

		default:
			TRACE(("PCI: Header type unknown (0x%02x)\n", dev->info.header_type));
			break;
	}
}


void
PCI::RefreshDeviceInfo(PCIBus *bus)
{
	for (PCIDev *dev = bus->child; dev; dev = dev->next) {
		ReadPciBasicInfo(dev);
		ReadPciHeaderInfo(dev);
		if (dev->child)
			RefreshDeviceInfo(dev->child);
	}
}
