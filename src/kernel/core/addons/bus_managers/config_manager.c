/* config_manager.c
 * 
 * Module that provides access to driver modules and busses
 */

#include <ktypes.h> 
#include <config_manager.h>
#include <PCI.h>
#include <ISA.h>
#include <bus_manager.h>
#include <errno.h>
#include <debug.h>
#include <string.h>
#include <memheap.h>

/* This is normally turned off. Turning it on (1) will provide a more
 * verbose description of the PCI devices found, though most of the extra
 * information is only useful if you're working on this code.
 */
#define THE_FULL_MONTY 0

static pci_module_info *pcim = NULL;

static void pci_scan_bus(uint8 bus);
static void pci_bridge(uint8 bus, uint8 dev, uint8 func);

#define B_CONFIG_MANAGER_FOR_BUS_MODULE_NAME "bus_managers/config_manager/bus/v1"

static status_t cfdm_get_next_device_info(bus_type bus, uint64 *cookie,
                                          struct device_info *info, uint32 len)
{
	dprintf("get_next_device_info(bus = %d, cookie = %lld)\n",
	        bus, *cookie);
	return 0;
}

static char *decode_class(uint8 base, uint8 sub_class)
{
	switch(base) {
		case PCI_early:
			switch (sub_class) {
				case PCI_early_not_vga:
					return "legacy (non VGA)";
				case PCI_early_vga:
					return "legacy VGA";
			}
		case 0x01:
			switch (sub_class) {
				case PCI_scsi:
					return "mass storage controller: scsi";
				case PCI_ide:
					return "mass storage controller: ide";
				case PCI_floppy:
					return "mass storage controller: floppy";
				case PCI_ipi:
					return "mass storage controller: ipi";
				case PCI_raid:
					return "mass storage controller: raid";
				case PCI_mass_storage_other:
					return "mass storage controller: other";
			}
		case 0x02:
			switch (sub_class) {
				case PCI_ethernet:
					return "network controller: ethernet";
				case PCI_token_ring:
					return "network controller: token ring";
				case PCI_fddi:
					return "network controller: fddi";
				case PCI_atm:
					return "network controller: atm";
				case PCI_network_other:
					return "network controller: other";
			}
		case 0x03:
			switch (sub_class) {
				case PCI_vga:
					return "display controller: vga";
				case PCI_xga:
					return "display controller: xga";
				case PCI_display_other:
					return "display controller: other";
			}					
		case 0x04:
			switch (sub_class) {
				case PCI_video:
					return "multimedia device: video";
				case PCI_audio:
					return "multimedia device: audio";
				case PCI_multimedia_other:
					return "multimedia device: other";
			}
		case 0x05: return "memory controller";
		case 0x06:
			switch (sub_class) {
				case PCI_host:
					return "bridge controller: host bridge";
				case PCI_isa:
					return "bridge controller: isa";
				case PCI_eisa:
					return "bridge controller: eisa";
				case PCI_microchannel:
					return "bridge controller: microchannel";
				case PCI_pci:
					return "bridge controller: PCI";
				case PCI_pcmcia:
					return "bridge controller: PC Card";
				case PCI_nubus:
					return "bridge controller: nubus";
				case PCI_cardbus:
					return "bridge controller: CardBus";
				case PCI_bridge_other:
					return "bridge controller: other";
			}
		case 0x07: return "simple comms controller";
		case 0x08: return "base system peripheral";
		case 0x09: return "input device";
		case 0x0a: return "docking station";
		case 0x0b: return "processor";
		case 0x0c:
			switch (sub_class) {
				case PCI_firewire:
					return "bus controller: IEEE1394 FireWire";
				case PCI_access:
					return "bus controller: ACCESS.bus";
				case PCI_ssa:
					return "bus controller: SSA";
				case PCI_usb:
					return "bus controller: USB";
				case PCI_fibre_channel:
					return "bus controller: fibre channel";
			}
		case 0x0d: return "wireless";
		case 0x0e: return "intelligent i/o ??";
		case 0x0f: return "satellite";
		case 0x10: return "encryption";
		case 0x11: return "signal processing";
		default: return "unknown";
	}
}

static void show_pci_details(struct pci_info *p, pci_module_info *pcim)
{
#ifdef THE_FULL_MONTY
	uint16 ss_vend, ss_dev;
#endif
	uint8 irq = 0;
	
	if (p->header_type == PCI_header_type_generic)
		irq = p->u.h0.interrupt_line;
	else
		irq = p->u.h1.interrupt_line;
	
	dprintf("0x%04x:0x%04x : ", p->vendor_id, p->device_id);

	if (irq > 0)
		dprintf("irq %d : ", irq);

	dprintf("location %d:%d:%d : ", p->bus, p->device, p->function);
	dprintf("class %02x:%02x:%02x", p->class_base, p->class_sub, 
	        p->class_api);
	dprintf(" => %s\n", decode_class(p->class_base, p->class_sub));

#if THE_FULL_MONTY
	dprintf("\trevision    : %02x\n", p->revision);
	dprintf("\tstatus      : %04x\n", 
	        pcim->read_pci_config(p->bus, p->device, p->function, PCI_status, 2));
	dprintf("\tcommand     : %04x\n", 
	        pcim->read_pci_config(p->bus, p->device, p->function, PCI_command, 2));
	dprintf("\tbase address: %08x\n", 
	        pcim->read_pci_config(p->bus, p->device, p->function, PCI_base_registers, 4));

	dprintf("\tline_size   : %02x\n", p->line_size);
	dprintf("\theader_type : %02x\n", p->header_type);
	dprintf("\tbist        : %02x\n", p->bist);
	
	if (p->header_type == PCI_header_type_generic) {
		dprintf("Header Type 0 (Generic)\n");
		dprintf("\tcardbus_cis         : %08lx\n", p->u.h0.cardbus_cis);
		dprintf("\trom_base_pci        : %08lx\n", p->u.h0.rom_base_pci);
		dprintf("\tinterrupt_line      : %02x\n", p->u.h0.interrupt_line);
		dprintf("\tinterrupt_pin       : %02x\n", p->u.h0.interrupt_pin);
		ss_vend = p->u.h0.subsystem_vendor_id;
		ss_dev = p->u.h0.subsystem_id;
	} else if (p->header_type == PCI_header_type_PCI_to_PCI_bridge) {
		dprintf("Header Type 1 (PCI-PCI bridge)\n");
		dprintf("\trom_base_pci        : %08lx\n", p->u.h1.rom_base_pci);
		dprintf("\tinterrupt_line      : %02x\n", p->u.h1.interrupt_line);
		dprintf("\tinterrupt_pin       : %02x\n", p->u.h1.interrupt_pin);
		dprintf("\t2ndry status        : %04x\n", p->u.h1.secondary_status);
		dprintf("\tprimary_bus         : %d\n", p->u.h1.primary_bus);
		dprintf("\tsecondary_bus       : %d\n", p->u.h1.secondary_bus);		
		dprintf("\tbridge control      : %02x\n", p->u.h1.bridge_control);
		ss_vend = p->u.h1.subsystem_vendor_id;
		ss_dev = p->u.h1.subsystem_id;
	}
	dprintf("\tsubsystem_id        : %04x\n", ss_dev);
	dprintf("\tsubsystem_vendor_id : %04x\n", ss_vend);
#endif
}

/* XXX - move these to a header file */
#define PCI_VENDOR_INTEL                 0x8086

#define PCI_PRODUCT_INTEL_82371AB_ISA    0x7110 /* PIIX4 ISA */
#define PCI_PRODUCT_INTEL_82371AB_IDE    0x7111 /* PIIX4 IDE */
#define PCI_PRODUCT_INTEL_82371AB_USB    0x7112 /* PIIX4 USB */
#define PCI_PRODUCT_INTEL_82371AB_PMC    0x7113 /* PIIX4 Power Management */

#define PCI_PRODUCT_INTEL_82443BX        0x7190
#define PCI_PRODUCT_INTEL_82443BX_AGP    0x7191
#define PCI_PRODUCT_INTEL_82443BX_NOAGP  0x7192


/* Borrowed from NetBSD.
 * Some Host bridges need to have fixes applied.
 */
static void fixup_host_bridge(uint8 bus, uint8 dev, uint8 func)
{
	uint16 vendor, device;
	
	vendor = pcim->read_pci_config(bus, dev, func, PCI_vendor_id, 2);
	device = pcim->read_pci_config(bus, dev, func, PCI_device_id, 2);
	
	switch (vendor) {
		case PCI_VENDOR_INTEL:
			switch (device) {
				case PCI_PRODUCT_INTEL_82443BX_AGP:
				case PCI_PRODUCT_INTEL_82443BX_NOAGP: {
					/* BIOS bug workaround
					 * While the datasheet indicates that the only valid setting
					 * for "Idle/Pipeline DRAM Leadoff Timing (IPLDT)" is 01,
					 * some BIOS's do not set these correctly, so we check and 
					 * correct if required.
					 */
					uint16 bcreg = pcim->read_pci_config(bus, dev, func, 0x76, 2);
					if ((bcreg & 0x0300) != 0x0100) {
						dprintf("Intel 82443BX Host Bridge: Fixing IPDLT setting\n");
						bcreg &= ~0x0300;
						bcreg |= 0x100;
						pcim->write_pci_config(bus, dev, func, 0x76, 2, bcreg);
					}
					break;
				}
			}
	}
}

/* Given a vendor/device pairing, do we need to scan through
 * the entire set of funtions? normally the return will be 0,
 * implying we don't need to, but for some it will be 1 which
 * means scan all functions.
 * This function may seem overkill but it prevents scanning
 * functions we don't need to and shoudl reduce the possibility of
 * duplicates being detected.
 */
static int pci_quirk_multifunction(uint16 vendor, uint16 device)
{
	switch (vendor) {
		case PCI_VENDOR_INTEL:
			switch (device) {
				case PCI_PRODUCT_INTEL_82371AB_ISA:
				case PCI_PRODUCT_INTEL_82371AB_IDE:
				case PCI_PRODUCT_INTEL_82371AB_USB:
				case PCI_PRODUCT_INTEL_82371AB_PMC:
					return 1;
			}
	}
	return 0;
}
	
/* pci_bridge()
 * The values passed in specify the "device" on the bus being searched
 * that has been identified as a PCI-PCI bridge. We now setup that bridge
 * and scan the bus it defines.
 * The bus is initially taken off-line, scanned and then put back on-line
 */
static void pci_bridge(uint8 bus, uint8 dev, uint8 func)
{
	uint16 command = 0;
	uint8 mybus = bus + 1;
	
	command = pcim->read_pci_config(bus, dev, func, PCI_command, 2);
	command &= ~ 0x03;
	pcim->write_pci_config(bus, dev, func, PCI_command, 2, command);

	/* Bus is now off line */
		
	pcim->write_pci_config(bus, dev, func, PCI_primary_bus, 1, bus);
	pcim->write_pci_config(bus, dev, func, PCI_secondary_bus, 1, mybus);
	pcim->write_pci_config(bus, dev, func, PCI_subordinate_bus, 1, 0xff);

	dprintf("PCI-PCI bridge at %d:%d:%d configured as bus %d\n", bus, dev, func, mybus);
	pci_scan_bus(mybus);

	/* Not strictly correct, but close enough... */
	command |= 0x03;
	pcim->write_pci_config(bus, dev, func, PCI_command, 2, command);
}

static void pci_device_probe(uint8 bus, uint8 dev, uint8 func) 
{
	uint8 base_class, sub_class;
	uint8 type;

	if (func > 0) {
		uint16 vend = pcim->read_pci_config(bus, dev, func, PCI_vendor_id, 2);
		if (vend == 0xffff)
			return;
	}
	
	type = pcim->read_pci_config(bus, dev, func, PCI_header_type, 1);
	type &= PCI_header_type_mask;
	base_class = pcim->read_pci_config(bus, dev, func, PCI_class_base, 1);
	sub_class  = pcim->read_pci_config(bus, dev, func, PCI_class_sub, 1);

	if (base_class == PCI_bridge) {
		if (sub_class == PCI_host)
			fixup_host_bridge(bus, dev, func);
		if (sub_class == PCI_pci) {
			pci_bridge(bus, dev, func);
			return;
		}
	}
	
	dprintf("device at %d:%d:%d => %s\n", bus, dev, func, 
	        decode_class(base_class, sub_class));	
}	

/* pci_bus()
 * Scan a bus for PCI devices. For each device that's a possible
 * we get the vendor_id. If it's 0xffff then it's not a valid
 * device so we move on.
 * Valid devices then have their header_type checked to detrmine how
 * many functions we need to check. However, some devices that support
 * multiple functions have a header_type of 0 (generic) so we also check
 * for these using pci_quirk_multifunction(). 
 * If it's a multifunction device we scan all 8 possible functions, 
 * otherwise we just probe the first one.
 */
static void pci_scan_bus(uint8 bus)
{
	uint8 dev = 0, func = 0;
	uint16 vend = 0;
	
	for (dev = 0; dev < 32; dev++) {
		vend = pcim->read_pci_config(bus, dev, 0, PCI_vendor_id, 2);
		if (vend != 0xffff) {
			uint16 device = pcim->read_pci_config(bus, dev, func, PCI_device_id, 2);
			uint8 type = pcim->read_pci_config(bus, dev, func, PCI_header_type, 1);
			uint8 nfunc = 8;
			
			type &= PCI_header_type_mask;
			if ((type & PCI_multifunction) == 0 &&
			    !pci_quirk_multifunction(vend, device))
				nfunc = 1;
				
			for (func = 0; func < nfunc; func++)
				pci_device_probe(bus, dev, func);
		}
	}
}

static status_t test_me(void)
{
	pci_info apci;
	long index = 0;
		
	while (pcim->get_nth_pci_info(index++, &apci) == 0) {
		show_pci_details(&apci, pcim);
	}
			
	return 0;
}
				
/* device_modules */
static int cfdm_std_ops(int32 op, ...)
{
	switch(op) {
		case B_MODULE_INIT:
			dprintf( "config_manager: device modules: init\n" );
			if (get_module(B_PCI_MODULE_NAME, (module_info**)&pcim) != 0) {
				dprintf("config_manager: failed to load PCI module\n");
				return -1;
			}
			pci_scan_bus(0);
			test_me();
			break;
		case B_MODULE_UNINIT:
			dprintf( "config_manager: device modules: uninit\n" );
			break;
		default:
			return EINVAL;
	}
	return B_OK;
}

/* bus_modules */
static int cfbm_std_ops(int32 op, ...)
{
	switch(op) {
		case B_MODULE_INIT:
			dprintf( "config_manager: bus modules: init\n" );
			break;
		case B_MODULE_UNINIT:
			dprintf( "config_manager: bus modules: uninit\n" );
			break;
		default:
			return EINVAL;
	}
	return B_OK;
}

/* cfdm = configuration_manager_device_modules */
struct config_manager_for_driver_module_info cfdm = {
	{
		B_CONFIG_MANAGER_FOR_DRIVER_MODULE_NAME,
		B_KEEP_LOADED,
		cfdm_std_ops
	},
	
	&cfdm_get_next_device_info, /* get_next_device_info */
	NULL, /* get_device_info_for */
	NULL, /* get_size_of_current_configuration_for */
	NULL, /* get_current_configuration_for */
	NULL, /* get_size_of_possible_configurations */
	NULL, /* get_possible_configurations_for */
	
	NULL, /* count_resource_descriptors_of_type */
	NULL, /* get_nth_resource_descriptor_of_type */
};

/* cfbm = configuration_manager_bus_modules */
struct config_manager_for_driver_module_info cfbm = {
	{
		B_CONFIG_MANAGER_FOR_BUS_MODULE_NAME,
		B_KEEP_LOADED,
		cfbm_std_ops
	},
	
	NULL, /* get_next_device_info */
	NULL, /* get_device_info_for */
	NULL, /* get_size_of_current_configuration_for */
	NULL, /* get_current_configuration_for */
	NULL, /* get_size_of_possible_configurations */
	NULL, /* get_possible_configurations_for */
	
	NULL, /* count_resource_descriptors_of_type */
	NULL, /* get_nth_resource_descriptor_of_type */
};

module_info *modules[] = {
	(module_info*)&cfdm,
	(module_info*)&cfbm,
	NULL
};
