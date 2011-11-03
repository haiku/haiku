/*
 * Copyright 2003-2006, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <KernelExport.h>
#define __HAIKU_PCI_BUS_MANAGER_TESTING 1
#include <PCI.h>
#include <string.h>
#include "pci_info.h"
#include "pci_private.h"
#include "pci.h"

#define PCI_VERBOSE	1
#define USE_PCI_HEADER 1

#if USE_PCI_HEADER
#	include "pcihdr.h"
#	include "pci-utils.h"
#endif

const char *get_capability_name(uint8 cap_id);


static void
print_pci2pci_bridge_info(const pci_info *info, bool verbose)
{
	TRACE(("PCI:   subsystem_id %04x, subsystem_vendor_id %04x\n",
			info->u.h1.subsystem_id, info->u.h1.subsystem_vendor_id));
	TRACE(("PCI:   primary_bus %02x, secondary_bus %02x, subordinate_bus %02x, secondary_latency %02x\n",
			info->u.h1.primary_bus, info->u.h1.secondary_bus, info->u.h1.subordinate_bus, info->u.h1.secondary_latency));
	TRACE(("PCI:   io_base_upper_16  %04x, io_base  %02x\n",
			info->u.h1.io_base_upper16, info->u.h1.io_base));
	TRACE(("PCI:   io_limit_upper_16 %04x, io_limit %02x\n",
			info->u.h1.io_limit_upper16, info->u.h1.io_limit));
	TRACE(("PCI:   memory_base %04x, memory_limit %04x\n",
			info->u.h1.memory_base, info->u.h1.memory_limit));
	TRACE(("PCI:   prefetchable_memory_base_upper32  %08lx, prefetchable_memory_base  %04x\n",
		info->u.h1.prefetchable_memory_base_upper32, info->u.h1.prefetchable_memory_base));
	TRACE(("PCI:   prefetchable_memory_limit_upper32 %08lx, prefetchable_memory_limit %04x\n",
		info->u.h1.prefetchable_memory_limit_upper32, info->u.h1.prefetchable_memory_limit));
	TRACE(("PCI:   bridge_control %04x, secondary_status %04x\n",
			info->u.h1.bridge_control, info->u.h1.secondary_status));
	TRACE(("PCI:   interrupt_line %02x, interrupt_pin %02x\n",
			info->u.h1.interrupt_line, info->u.h1.interrupt_pin));
	TRACE(("PCI:   ROM base host %08lx, pci %08lx, size ??\n",
			info->u.h1.rom_base, info->u.h1.rom_base_pci));
	for (int i = 0; i < 2; i++)
		TRACE(("PCI:   base reg %d: host %08lx, pci %08lx, size %08lx, flags %02x\n",
			i, info->u.h1.base_registers[i], info->u.h1.base_registers_pci[i],
			info->u.h1.base_register_sizes[i], info->u.h1.base_register_flags[i]));
}


static void
print_pci2cardbus_bridge_info(const pci_info *info, bool verbose)
{
	TRACE(("PCI:   subsystem_id %04x, subsystem_vendor_id %04x\n",
			info->u.h2.subsystem_id, info->u.h2.subsystem_vendor_id));
	TRACE(("PCI:   primary_bus %02x, secondary_bus %02x, subordinate_bus %02x, secondary_latency %02x\n",
			info->u.h2.primary_bus, info->u.h2.secondary_bus, info->u.h2.subordinate_bus, info->u.h2.secondary_latency));
	TRACE(("PCI:   bridge_control %04x, secondary_status %04x\n",
			info->u.h2.bridge_control, info->u.h2.secondary_status));
	TRACE(("PCI:   memory_base_upper32  %08lx, memory_base  %08lx\n",
		info->u.h2.memory_base_upper32, info->u.h2.memory_base));
	TRACE(("PCI:   memory_limit_upper32 %08lx, memory_limit %08lx\n",
		info->u.h2.memory_limit_upper32, info->u.h2.memory_limit));
	TRACE(("PCI:   io_base_upper32  %08lx, io_base  %08lx\n",
		info->u.h2.io_base_upper32, info->u.h2.io_base));
	TRACE(("PCI:   io_limit_upper32 %08lx, io_limit %08lx\n",
		info->u.h2.io_limit_upper32, info->u.h2.io_limit));
}


static void
print_generic_info(const pci_info *info, bool verbose)
{
	TRACE(("PCI:   ROM base host %08lx, pci %08lx, size %08lx\n",
			info->u.h0.rom_base, info->u.h0.rom_base_pci, info->u.h0.rom_size));
	TRACE(("PCI:   cardbus_CIS %08lx, subsystem_id %04x, subsystem_vendor_id %04x\n",
			info->u.h0.cardbus_cis, info->u.h0.subsystem_id, info->u.h0.subsystem_vendor_id));
	TRACE(("PCI:   interrupt_line %02x, interrupt_pin %02x, min_grant %02x, max_latency %02x\n",
			info->u.h0.interrupt_line, info->u.h0.interrupt_pin, info->u.h0.min_grant, info->u.h0.max_latency));
	for (int i = 0; i < 6; i++)
		TRACE(("PCI:   base reg %d: host %08lx, pci %08lx, size %08lx, flags %02x\n",
			i, info->u.h0.base_registers[i], info->u.h0.base_registers_pci[i],
			info->u.h0.base_register_sizes[i], info->u.h0.base_register_flags[i]));
}


static void
print_capabilities(const pci_info *info)
{
	uint16	status;
	uint8	cap_ptr;
	uint8	cap_id;
	int		i;

	TRACE(("PCI:   Capabilities: "));

	status = pci_read_config(info->bus, info->device, info->function, PCI_status, 2);
	if (!(status & PCI_status_capabilities)) {
		TRACE(("(not supported)\n"));
		return;
	}

	switch (info->header_type & PCI_header_type_mask) {
		case PCI_header_type_generic:
		case PCI_header_type_PCI_to_PCI_bridge:
			cap_ptr = pci_read_config(info->bus, info->device, info->function, PCI_capabilities_ptr, 1);
			break;
		case PCI_header_type_cardbus:
			cap_ptr = pci_read_config(info->bus, info->device, info->function, PCI_capabilities_ptr_2, 1);
			break;
		default:
			TRACE(("(unknown header type)\n"));
			return;
	}

	cap_ptr &= ~3;
	if (!cap_ptr) {
		TRACE(("(empty list)\n"));
		return;
	}

	for (i = 0; i < 48; i++) {
		const char *name;
		cap_id  = pci_read_config(info->bus, info->device, info->function, cap_ptr, 1);
		cap_ptr = pci_read_config(info->bus, info->device, info->function, cap_ptr + 1, 1);
		cap_ptr &= ~3;
		if (i) {
			TRACE((", "));
		}
		name = get_capability_name(cap_id);
		if (name) {
			TRACE(("%s", name));
		} else {
			TRACE(("0x%02x", cap_id));
		}
		if (!cap_ptr)
			break;
	}
	TRACE(("\n"));
}


static void
print_info_basic(const pci_info *info, bool verbose)
{
	int domain;
	uint8 bus;

	__pci_resolve_virtual_bus(info->bus, &domain, &bus);

	TRACE(("PCI: [dom %d, bus %2d] bus %3d, device %2d, function %2d: vendor %04x, device %04x, revision %02x\n",
			domain, bus, info->bus /* virtual bus*/,
			info->device, info->function, info->vendor_id, info->device_id, info->revision));
	TRACE(("PCI:   class_base %02x, class_function %02x, class_api %02x\n",
			info->class_base, info->class_sub, info->class_api));

	if (verbose) {
#if USE_PCI_HEADER
		const char *venShort;
		const char *venFull;
		get_vendor_info(info->vendor_id, &venShort, &venFull);
		if (!venShort && !venFull) {
			TRACE(("PCI:   vendor %04x: Unknown\n", info->vendor_id));
		} else if (venShort && venFull) {
			TRACE(("PCI:   vendor %04x: %s - %s\n", info->vendor_id, venShort, venFull));
		} else {
			TRACE(("PCI:   vendor %04x: %s\n", info->vendor_id, venShort ? venShort : venFull));
		}
		const char *devShort;
		const char *devFull;
		get_device_info(info->vendor_id, info->device_id, info->u.h0.subsystem_vendor_id, info->u.h0.subsystem_id,
			&devShort, &devFull);
		if (!devShort && !devFull) {
			TRACE(("PCI:   device %04x: Unknown\n", info->device_id));
		} else if (devShort && devFull) {
			TRACE(("PCI:   device %04x: %s (%s)\n", info->device_id, devShort, devFull));
		} else {
			TRACE(("PCI:   device %04x: %s\n", info->device_id, devShort ? devShort : devFull));
		}
		char classInfo[64];
		get_class_info(info->class_base, info->class_sub, info->class_api, classInfo, sizeof(classInfo));
		TRACE(("PCI:   info: %s\n", classInfo));
#endif
	}
	TRACE(("PCI:   line_size %02x, latency %02x, header_type %02x, BIST %02x\n",
			info->line_size, info->latency, info->header_type, info->bist));

	switch (info->header_type & PCI_header_type_mask) {
		case PCI_header_type_generic:
			print_generic_info(info, verbose);
			break;
		case PCI_header_type_PCI_to_PCI_bridge:
			print_pci2pci_bridge_info(info, verbose);
			break;
		case PCI_header_type_cardbus:
			print_pci2cardbus_bridge_info(info, verbose);
			break;
		default:
			TRACE(("PCI:   unknown header type\n"));
	}

	print_capabilities(info);
}


void
pci_print_info()
{
	pci_info info;
	for (long index = 0; B_OK == pci_get_nth_pci_info(index, &info); index++) {
		print_info_basic(&info, PCI_VERBOSE);
	}
}


const char *
get_capability_name(uint8 cap_id)
{
	switch (cap_id) {
		case PCI_cap_id_reserved:
			return "reserved";
		case PCI_cap_id_pm:
			return "PM";
		case PCI_cap_id_agp:
			return "AGP";
		case PCI_cap_id_vpd:
			return "VPD";
		case PCI_cap_id_slotid:
			return "SlotID";
		case PCI_cap_id_msi:
			return "MSI";
		case PCI_cap_id_chswp:
			return "CompactPCIHotSwap";
		case PCI_cap_id_pcix:
			return "PCI-X";
		case PCI_cap_id_ldt:
			return "ldt";
		case PCI_cap_id_vendspec:
			return "vendspec";
		case PCI_cap_id_debugport:
			return "DebugPort";
		case PCI_cap_id_cpci_rsrcctl:
			return "cpci_rsrcctl";
		case PCI_cap_id_hotplug:
			return "HotPlug";
		case PCI_cap_id_subvendor:
			return "subvendor";
		case PCI_cap_id_agp8x:
			return "AGP8x";
		case PCI_cap_id_secure_dev:
			return "Secure Device";
		case PCI_cap_id_pcie:
			return "PCIe";
		case PCI_cap_id_msix:
			return "MSI-X";
		case PCI_cap_id_sata:
			return "SATA";
		case PCI_cap_id_pciaf:
			return "AdvancedFeatures";
		default:
			return NULL;
	}
}

