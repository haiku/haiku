#include <KernelExport.h>
#include <PCI.h>
#include "pci_info.h"
#include "pci.h"


#define USE_PCI_HEADER 0

#if USE_PCI_HEADER
  #include "pcihdr.h"
#endif

void
print_bridge_info(pci_info *info)
{
	dprintf("PCI:   primary_bus %02x, secondary_bus %02x, subordinate_bus %02x, secondary_latency %02x\n",
			info->u.h1.primary_bus, info->u.h1.secondary_bus, info->u.h1.subordinate_bus, info->u.h1.secondary_latency);
	dprintf("PCI:   io_base %04x%02x, io_limit %04x%02x\n",
			info->u.h1.io_base_upper16, info->u.h1.io_base, info->u.h1.io_limit_upper16, info->u.h1.io_limit);
	dprintf("PCI:   memory_base %04x, memory_limit %04x\n",
			info->u.h1.memory_base, info->u.h1.memory_limit);
	dprintf("PCI:   prefetchable memory base %08x%04x, limit %08x%04x\n",
		info->u.h1.prefetchable_memory_base_upper32, info->u.h1.prefetchable_memory_base,
		info->u.h1.prefetchable_memory_limit_upper32, info->u.h1.prefetchable_memory_limit);
	dprintf("PCI:   bridge_control %04x, secondary_status %04x\n",
			info->u.h1.bridge_control, info->u.h1.secondary_status);
	dprintf("PCI:   interrupt_line %02x, interrupt_pin %02x\n",
			info->u.h1.interrupt_line, info->u.h1.interrupt_pin);
	dprintf("PCI:   ROM base host %08x, pci %08x, size ??\n",
			info->u.h1.rom_base, info->u.h1.rom_base_pci);
	for (int i = 0; i < 2; i++)
		dprintf("PCI:   base reg %d: host %08x, pci %08x, size %08x, flags %02x\n",
			i, info->u.h1.base_registers[i], info->u.h1.base_registers_pci[i],
			info->u.h1.base_register_sizes[i], info->u.h1.base_register_flags[i]);
}

void
print_generic_info(pci_info *info)
{
	dprintf("PCI:   ROM base host %08x, pci %08x, size %08x\n",
			info->u.h0.rom_base, info->u.h0.rom_base_pci, info->u.h0.rom_size);
	dprintf("PCI:   cardbus_CIS %08x, subsystem_id %04x, subsystem_vendor_id %04x\n",
			info->u.h0.cardbus_cis, info->u.h0.subsystem_id, info->u.h0.subsystem_vendor_id);
	dprintf("PCI:   interrupt_line %02x, interrupt_pin %02x, min_grant %02x, max_latency %02x\n",
			info->u.h0.interrupt_line, info->u.h0.interrupt_pin, info->u.h0.min_grant, info->u.h0.max_latency);
	for (int i = 0; i < 6; i++)
		dprintf("PCI:   base reg %d: host %08x, pci %08x, size %08x, flags %02x\n",
			i, info->u.h0.base_registers[i], info->u.h0.base_registers_pci[i],
			info->u.h0.base_register_sizes[i], info->u.h0.base_register_flags[i]);
}

void
print_info_basic(pci_info *info)
{
	dprintf("PCI: bus %2d, device %2d, function %2d: vendor %04x, device %04x, revision %02x\n",
			info->bus, info->device, info->function, info->vendor_id, info->device_id, info->revision);
	dprintf("PCI:   class_base %02x, class_function %02x, class_api %02x\n",
			info->class_base, info->class_sub, info->class_api);
	dprintf("PCI:   line_size %02x, latency %02x, header_type %02x, BIST %02x\n",
			info->line_size, info->latency, info->header_type, info->bist);
	switch (info->header_type) {
		case 0:
			print_generic_info(info);
			break;
		case 1:
			print_bridge_info(info);
			break;
		default:
			dprintf("PCI:   unknown header type\n");
	}
};

void
pci_print_info()
{
	pci_info info;
	for (long index = 0; B_OK == pci_get_nth_pci_info(index, &info); index++) {
		print_info_basic(&info);
	}
}
