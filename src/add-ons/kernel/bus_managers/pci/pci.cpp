/*
 * Copyright 2006, Marcus Overhagen. All rights reserved.
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

static PCI *sPCI;

// #pragma mark bus manager exports


status_t
pci_controller_add(pci_controller *controller, void *cookie)
{
	return sPCI->AddController(controller, cookie);
}


long
pci_get_nth_pci_info(long index, pci_info *outInfo)
{
	return sPCI->GetNthPciInfo(index, outInfo);
}


uint32
pci_read_config(uint8 virt_bus, uint8 device, uint8 function, uint8 offset, uint8 size)
{
	uint8 bus;
	int domain;
	uint32 value;
	
	if (sPCI->GetVirtBus(virt_bus, &domain, &bus) != B_OK)
		return 0xffffffff;
		
	if (sPCI->ReadPciConfig(domain, bus, device, function, offset, size, &value) != B_OK)
		return 0xffffffff;
		
	return value;
}


void
pci_write_config(uint8 virt_bus, uint8 device, uint8 function, uint8 offset, uint8 size, uint32 value)
{
	uint8 bus;
	int domain;
	
	if (sPCI->GetVirtBus(virt_bus, &domain, &bus) != B_OK)
		return;
		
	sPCI->WritePciConfig(domain, bus, device, function, offset, size, value);
}


// #pragma mark bus manager init/uninit

status_t
pci_init(void)
{
	sPCI = new PCI;

	if (pci_io_init() != B_OK) {
		TRACE(("PCI: pci_io_init failed\n"));
		return B_ERROR;
	}

	if (pci_controller_init() != B_OK) {
		TRACE(("PCI: pci_controller_init failed\n"));
		return B_ERROR;
	}

	sPCI->InitDomainData();
	sPCI->InitBus();

	return B_OK;
}


void
pci_uninit(void)
{
	delete sPCI;
}


// #pragma mark PCI class

PCI::PCI()
 :	fRootBus(0)
 ,	fDomainCount(0)
{
}


void
PCI::InitBus()
{
	PCIBus **ppnext = &fRootBus;
	
	for (int i = 0; i < fDomainCount; i++) {
		PCIBus *bus = new PCIBus;
		bus->next = NULL;
		bus->parent = NULL;
		bus->child = NULL;
		bus->domain = i;
		bus->bus = 0;
		*ppnext = bus;
		ppnext = &bus->next;
	}
	
	if (fRootBus) {
		DiscoverBus(fRootBus);
		RefreshDeviceInfo(fRootBus);
	}
}


PCI::~PCI()
{
}


status_t
PCI::AddVirtBus(int domain, uint8 bus, uint8 *virt_bus)
{
	if (MAX_PCI_DOMAINS != 8)
		panic("PCI::AddVirtBus only 8 controllers supported");
		
	if (domain > 7)
		panic("PCI::AddVirtBus domain %d too large");

	if (bus > 31)
		panic("PCI::AddVirtBus bus %d too large");
		
	*virt_bus = (domain << 5) | bus;
	return B_OK;
}


status_t
PCI::GetVirtBus(uint8 virt_bus, int *domain, uint8 *bus)
{
	// XXX if you modify this, also change pci_info.cpp print_info_basic() !!
	*domain = virt_bus >> 5;
	*bus = virt_bus & 0x1f;
	return B_OK;
}

status_t
PCI::AddController(pci_controller *controller, void *controller_cookie)
{
	if (fDomainCount == MAX_PCI_DOMAINS)
		return B_ERROR;
	
	fDomainData[fDomainCount].controller = controller;
	fDomainData[fDomainCount].controller_cookie = controller_cookie;

	// initialized later to avoid call back into controller at this point
	fDomainData[fDomainCount].max_bus_devices = -1;
	
	fDomainCount++;
	return B_OK;
}

void
PCI::InitDomainData()
{
	for (int i = 0; i < fDomainCount; i++) {
		int32 count;
		status_t status;
		
		status = (*fDomainData[i].controller->get_max_bus_devices)(fDomainData[i].controller_cookie, &count);
		fDomainData[i].max_bus_devices = (status == B_OK) ? count : 0;
	}
}


domain_data *
PCI::GetDomainData(int domain)
{
	if (domain < 0 || domain >= fDomainCount)
		return NULL;

	return &fDomainData[domain];
}


status_t
PCI::GetNthPciInfo(long index, pci_info *outInfo)
{
	long curindex = 0;
	if (!fRootBus)
		return B_ERROR;
	return GetNthPciInfo(fRootBus, &curindex, index, outInfo);
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
	
	if (bus->next)
		return GetNthPciInfo(bus->next, curindex, wantindex, outInfo);
		
	return B_ERROR;
}


void
PCI::DiscoverBus(PCIBus *bus)
{
	TRACE(("PCI: DiscoverBus, domain %u, bus %u\n", bus->domain, bus->bus));
	
	int max_bus_devices = GetDomainData(bus->domain)->max_bus_devices;

	for (int dev = 0; dev < max_bus_devices; dev++) {
		uint16 vendor_id = ReadPciConfig(bus->domain, bus->bus, dev, 0, PCI_vendor_id, 2);
		if (vendor_id == 0xffff)
			continue;
		
		uint8 type = ReadPciConfig(bus->domain, bus->bus, dev, 0, PCI_header_type, 1);
		int nfunc = (type & PCI_multifunction) ? 8 : 1;
		for (int func = 0; func < nfunc; func++)
			DiscoverDevice(bus, dev, func);
	}
	
	if (bus->next)
		DiscoverBus(bus->next);
}


void
PCI::DiscoverDevice(PCIBus *bus, uint8 dev, uint8 func)
{
	TRACE(("PCI: DiscoverDevice, domain %u, bus %u, dev %u, func %u\n", bus->domain, bus->bus, dev, func));

	uint16 device_id = ReadPciConfig(bus->domain, bus->bus, dev, func, PCI_device_id, 2);
	if (device_id == 0xffff)
		return;

	PCIDev *newdev = CreateDevice(bus, dev, func);

	uint8 base_class = ReadPciConfig(bus->domain, bus->bus, dev, func, PCI_class_base, 1);
	uint8 sub_class	 = ReadPciConfig(bus->domain, bus->bus, dev, func, PCI_class_sub, 1);
	if (base_class == PCI_bridge && sub_class == PCI_pci) {
		uint8 secondary_bus = ReadPciConfig(bus->domain, bus->bus, dev, func, PCI_secondary_bus, 1);
		PCIBus *newbus = CreateBus(newdev, bus->domain, secondary_bus);
		DiscoverBus(newbus);
	}
}


PCIBus *
PCI::CreateBus(PCIDev *parent, int domain, uint8 bus)
{
	PCIBus *newbus = new PCIBus;
	newbus->next = NULL;
	newbus->parent = parent;
	newbus->child = NULL;
	newbus->domain = domain;
	newbus->bus = bus;
	
	// append
	parent->child = newbus;
	
	return newbus;
}

	
PCIDev *
PCI::CreateDevice(PCIBus *parent, uint8 dev, uint8 func)
{
	TRACE(("PCI: CreateDevice, domain %u, bus %u, dev %u, func %u:\n", parent->domain, parent->bus, dev, func));
	
	PCIDev *newdev = new PCIDev;
	newdev->next = NULL;
	newdev->parent = parent;
	newdev->child = NULL;
	newdev->domain = parent->domain;
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
	uint32 oldvalue = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, offset, 4);
	WritePciConfig(dev->domain, dev->bus, dev->dev, dev->func, offset, 4, 0xffffffff);
	uint32 newvalue = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, offset, 4);
	WritePciConfig(dev->domain, dev->bus, dev->dev, dev->func, offset, 4, oldvalue);
	
	*address = oldvalue & PCI_address_memory_32_mask;
	if (size)
		*size = BarSize(newvalue, PCI_address_memory_32_mask);
	if (flags)
		*flags = newvalue & 0xf;
}


void
PCI::GetRomBarInfo(PCIDev *dev, uint8 offset, uint32 *address, uint32 *size, uint8 *flags)
{
	uint32 oldvalue = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, offset, 4);
	WritePciConfig(dev->domain, dev->bus, dev->dev, dev->func, offset, 4, 0xfffffffe); // LSB must be 0
	uint32 newvalue = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, offset, 4);
	WritePciConfig(dev->domain, dev->bus, dev->dev, dev->func, offset, 4, oldvalue);
	
	*address = oldvalue & PCI_rom_address_mask;
	if (size)
		*size = BarSize(newvalue, PCI_rom_address_mask);
	if (flags)
		*flags = newvalue & 0xf;
}


void
PCI::ReadPciBasicInfo(PCIDev *dev)
{
	uint8 virt_bus;
	
	if (AddVirtBus(dev->domain, dev->bus, &virt_bus) != B_OK) {
		dprintf("PCI: AddVirtBus failed, domain %u, bus %u\n", dev->domain, dev->bus);
		return;
	}
	
	dev->info.vendor_id = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_vendor_id, 2);
	dev->info.device_id = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_device_id, 2);
	dev->info.bus = virt_bus;
	dev->info.device = dev->dev;
	dev->info.function = dev->func;
	dev->info.revision = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_revision, 1);
	dev->info.class_api = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_class_api, 1);
	dev->info.class_sub = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_class_sub, 1);
	dev->info.class_base = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_class_base, 1);
	dev->info.line_size = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_line_size, 1);
	dev->info.latency = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_latency, 1);
	// BeOS does not mask off the multifunction bit, developer must use (header_type & PCI_header_type_mask)
	dev->info.header_type = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_header_type, 1);
	dev->info.bist = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_bist, 1);
	dev->info.reserved = 0;
}


void
PCI::ReadPciHeaderInfo(PCIDev *dev)
{
	switch (dev->info.header_type & PCI_header_type_mask) {
		case PCI_header_type_generic:
		{
			// disable PCI device address decoding (io and memory) while BARs are modified
			uint16 pcicmd = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_command, 2);
			WritePciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_command, 2, pcicmd & ~(PCI_command_io | PCI_command_memory));

			// get BAR size infos			
			GetRomBarInfo(dev, PCI_rom_base, &dev->info.u.h0.rom_base_pci, &dev->info.u.h0.rom_size);
			for (int i = 0; i < 6; i++) {
				GetBarInfo(dev, PCI_base_registers + 4*i,
					&dev->info.u.h0.base_registers_pci[i],
					&dev->info.u.h0.base_register_sizes[i],
					&dev->info.u.h0.base_register_flags[i]);
			}
			
			// restore PCI device address decoding
			WritePciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_command, 2, pcicmd);

			dev->info.u.h0.rom_base = (ulong)pci_ram_address((void *)dev->info.u.h0.rom_base_pci);
			for (int i = 0; i < 6; i++) {
				dev->info.u.h0.base_registers[i] = (ulong)pci_ram_address((void *)dev->info.u.h0.base_registers_pci[i]);
			}
			
			dev->info.u.h0.cardbus_cis = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_cardbus_cis, 4);
			dev->info.u.h0.subsystem_id = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_subsystem_id, 2);
			dev->info.u.h0.subsystem_vendor_id = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_subsystem_vendor_id, 2);
			dev->info.u.h0.interrupt_line = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_interrupt_line, 1);
			dev->info.u.h0.interrupt_pin = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_interrupt_pin, 1);
			dev->info.u.h0.min_grant = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_min_grant, 1);
			dev->info.u.h0.max_latency = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_max_latency, 1);
			break;
		}

		case PCI_header_type_PCI_to_PCI_bridge:
		{
			// disable PCI device address decoding (io and memory) while BARs are modified
			uint16 pcicmd = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_command, 2);
			WritePciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_command, 2, pcicmd & ~(PCI_command_io | PCI_command_memory));

			GetRomBarInfo(dev, PCI_bridge_rom_base, &dev->info.u.h1.rom_base_pci);
			for (int i = 0; i < 2; i++) {
				GetBarInfo(dev, PCI_base_registers + 4*i,
					&dev->info.u.h1.base_registers_pci[i],
					&dev->info.u.h1.base_register_sizes[i],
					&dev->info.u.h1.base_register_flags[i]);
			}

			// restore PCI device address decoding
			WritePciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_command, 2, pcicmd);

			dev->info.u.h1.rom_base = (ulong)pci_ram_address((void *)dev->info.u.h1.rom_base_pci);
			for (int i = 0; i < 2; i++) {
				dev->info.u.h1.base_registers[i] = (ulong)pci_ram_address((void *)dev->info.u.h1.base_registers_pci[i]);
			}

			dev->info.u.h1.primary_bus = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_primary_bus, 1);
			dev->info.u.h1.secondary_bus = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_secondary_bus, 1);
			dev->info.u.h1.subordinate_bus = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_subordinate_bus, 1);
			dev->info.u.h1.secondary_latency = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_secondary_latency, 1);
			dev->info.u.h1.io_base = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_io_base, 1);
			dev->info.u.h1.io_limit = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_io_limit, 1);
			dev->info.u.h1.secondary_status = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_secondary_status, 2);
			dev->info.u.h1.memory_base = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_memory_base, 2);
			dev->info.u.h1.memory_limit = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_memory_limit, 2);
			dev->info.u.h1.prefetchable_memory_base = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_prefetchable_memory_base, 2);
			dev->info.u.h1.prefetchable_memory_limit = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_prefetchable_memory_limit, 2);
			dev->info.u.h1.prefetchable_memory_base_upper32 = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_prefetchable_memory_base_upper32, 4);
			dev->info.u.h1.prefetchable_memory_limit_upper32 = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_prefetchable_memory_limit_upper32, 4);
			dev->info.u.h1.io_base_upper16 = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_io_base_upper16, 2);
			dev->info.u.h1.io_limit_upper16 = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_io_limit_upper16, 2);
			dev->info.u.h1.interrupt_line = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_interrupt_line, 1);
			dev->info.u.h1.interrupt_pin = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_interrupt_pin, 1);
			dev->info.u.h1.bridge_control = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_bridge_control, 2);	
			dev->info.u.h1.subsystem_id = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_sub_device_id_1, 2);
			dev->info.u.h1.subsystem_vendor_id = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_sub_vendor_id_1, 2);
			break;
		}
		
		case PCI_header_type_cardbus:
		{
			// for testing only, not final:
			dev->info.u.h2.subsystem_id = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_sub_device_id_2, 2);
			dev->info.u.h2.subsystem_vendor_id = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_sub_vendor_id_2, 2);
			dev->info.u.h2.primary_bus = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_primary_bus_2, 1);
			dev->info.u.h2.secondary_bus = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_secondary_bus_2, 1);
			dev->info.u.h2.subordinate_bus = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_subordinate_bus_2, 1);
			dev->info.u.h2.secondary_latency = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_secondary_latency_2, 1);
			dev->info.u.h2.reserved = 0;
			dev->info.u.h2.memory_base = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_memory_base0_2, 4);
			dev->info.u.h2.memory_limit = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_memory_limit0_2, 4);
			dev->info.u.h2.memory_base_upper32 = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_memory_base1_2, 4);
			dev->info.u.h2.memory_limit_upper32 = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_memory_limit1_2, 4);
			dev->info.u.h2.io_base = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_io_base0_2, 4);
			dev->info.u.h2.io_limit = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_io_limit0_2, 4);
			dev->info.u.h2.io_base_upper32 = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_io_base1_2, 4);
			dev->info.u.h2.io_limit_upper32 = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_io_limit1_2, 4);
			dev->info.u.h2.secondary_status = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_secondary_status_2, 2);
			dev->info.u.h2.bridge_control = ReadPciConfig(dev->domain, dev->bus, dev->dev, dev->func, PCI_bridge_control_2, 1);
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
	
	if (bus->next)
		RefreshDeviceInfo(bus->next);
}


status_t
PCI::ReadPciConfig(int domain, uint8 bus, uint8 device, uint8 function,
				   uint8 offset, uint8 size, uint32 *value)
{
	domain_data *info;
	info = GetDomainData(domain);
	if (!info)
		return B_ERROR;
	
	if (device > (info->max_bus_devices - 1) 
		|| function > 7 
		|| (size != 1 && size != 2 && size != 4)
		|| (size == 2 && (offset & 3) == 3) 
		|| (size == 4 && (offset & 3) != 0)) {
		dprintf("PCI: can't read config for domain %d, bus %u, device %u, function %u, offset %u, size %u\n",
			 domain, bus, device, function, offset, size);
		return B_ERROR;
	}

	status_t status;
	status = (*info->controller->read_pci_config)(info->controller_cookie,
												  bus, device, function,
												  offset, size, value);
	return status;
}


uint32
PCI::ReadPciConfig(int domain, uint8 bus, uint8 device, uint8 function,
				   uint8 offset, uint8 size)
{
	uint32 value;
	
	if (ReadPciConfig(domain, bus, device, function, offset, size, &value) != B_OK)
		return 0xffffffff;
		
	return value;
}


status_t
PCI::WritePciConfig(int domain, uint8 bus, uint8 device, uint8 function,
					uint8 offset, uint8 size, uint32 value)
{
	domain_data *info;
	info = GetDomainData(domain);
	if (!info)
		return B_ERROR;
	
	if (device > (info->max_bus_devices - 1) 
		|| function > 7 
		|| (size != 1 && size != 2 && size != 4)
		|| (size == 2 && (offset & 3) == 3) 
		|| (size == 4 && (offset & 3) != 0)) {
		dprintf("PCI: can't write config for domain %d, bus %u, device %u, function %u, offset %u, size %u\n",
			 domain, bus, device, function, offset, size);
		return B_ERROR;
	}

	status_t status;
	status = (*info->controller->write_pci_config)(info->controller_cookie,
												   bus, device, function,
												   offset, size, value);
	return status;
}

