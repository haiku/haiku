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

/* This is normally turned off. Turning it on (1) will provide a more
 * verbose description of the PCI devices found, though most of the extra
 * information is only useful if you're working on this code.
 */
#define THE_FULL_MONTY 0

#define B_CONFIG_MANAGER_FOR_BUS_MODULE_NAME "bus_managers/config_manager/bus/v1"

static status_t cfdm_get_next_device_info(bus_type bus, uint64 *cookie,
                                          struct device_info *info, uint32 len)
{
	dprintf("get_next_device_info(bus = %d, cookie = %lld)\n",
	        bus, *cookie);
	return 0;
}

static char *decode_class_base(uint8 base)
{
	switch(base) {
		case 0x00: return "legacy";
		case 0x01: return "mass storage controller";
		case 0x02: return "network controller";
		case 0x03: return "display controller";
		case 0x04: return "multimedia device";
		case 0x05: return "memory controller";
		case 0x06: return "bridge controller";
		case 0x07: return "simple comms controller";
		case 0x08: return "base system peripheral";
		case 0x09: return "input device";
		case 0x0a: return "docking station";
		case 0x0b: return "processor";
		case 0x0c: return "serial bus controller";
		case 0x0d: return "wireless";
		case 0x0e: return "intelligent i/o ??";
		case 0x0f: return "satellite";
		case 0x10: return "encryption";
		case 0x11: return "signal processing";
		default: return "unknown";
	}
}

/* XXX - these are only listed as they're the ones that my system has!
 *       I'm committing mainly so I don't have to keep adjusting this file each
 *       time I commit a change. I'm NOT suggesting others add their codes here.
 *       The level of detail offered is useful if you're working on this, otherwise
 *       it's just nice to have. Adding all the data we need will add over 270k to
 *       the build!
 */
static char *decode_vendor(uint16 vendor)
{
	switch(vendor) {
		case 0x102b: return "Matrox";
		case 0x10b7: return "3Com";
		case 0x11ad: return "Lite-On";
		case 0x1385: return "NetGear";
		case 0x8086: return "Intel";
		default: return "unknown";
	}
}

static char *decode_device(uint16 dev)
{
	switch(dev) {
		case 0x0002: return "NGMC169B, 10/100 Ethernet (NetGear FA310TX)";
		case 0x051a: return "MGA 1064SG, Hurricane/Cyclone 64-bit graphics chip";
		case 0x7110: return "82371AB/EB/MB, PIIX4/4E/4M ISA Bridge";
		case 0x7111: return "82371AB/EB/MB, PIIX4/4E/4M IDE Controller";
		case 0x7112: return "82371AB/EB/MB, PIIX4/4E/4M USB Interface";
		case 0x7113: return "82371AB/EB/MB, PIIX4/4E/4M Power Management Controller";
		case 0x7190: return "82443BX/ZX, 440BX/ZX AGPset Host Bridge";	
		case 0x7191: return "82443BX/ZX, 440BX/ZX AGPset PCI-to-PCI bridge";
		case 0x7192: return "82443BX/ZX, 440BX/ZX chipset Host-to-PCI Bridge";
		case 0x9055: return "3C905B-TX, Fast Etherlink 10/100 PCI TX NIC";
		default: return "unknown";
	}
}

static void show_pci_details(struct pci_info *p, pci_module_info *pcim)
{
	dprintf("PCI device found:\n");
	dprintf("\tvendor id   : %02x [%s]\n", p->vendor_id, decode_vendor(p->vendor_id));
	dprintf("\tdevice id   : %02x [%s]\n", p->device_id, decode_device(p->device_id));

#if THE_FULL_MONTY
	dprintf("\tbus         : %d\n", p->bus);
	dprintf("\tdevice      : %d\n", p->device);
	dprintf("\tfunction    : %d\n", p->function);
	dprintf("\trevision    : %02x\n", p->revision);
	dprintf("\tclass_api   : %02x\n", p->class_api);
	dprintf("\tclass_sub   : %02x\n", p->class_sub);
#endif
	dprintf("\tclass_base  : %02x [%s]\n", p->class_base, decode_class_base(p->class_base));

#if THE_FULL_MONTY
	dprintf("\tstatus      : %04x\n", 
	        pcim->read_pci_config(p->bus, p->device, p->function, PCI_status, 2));
	dprintf("\tcommand     : %04x\n", 
	        pcim->read_pci_config(p->bus, p->device, p->function, PCI_command, 2));
	dprintf("\tbase address: %08lx\n", 
	        pcim->read_pci_config(p->bus, p->device, p->function, PCI_base_registers, 4));

	dprintf("\tline_size   : %02x\n", p->line_size);
	dprintf("\theader_type : %02x\n", p->header_type);
	dprintf("\tbist        : %02x\n", p->bist);
	
	if (p->header_type == 0) {
		dprintf("Header Type 0\n");
		dprintf("\tcardbus_cis         : %08lx\n", p->u.h0.cardbus_cis);
		dprintf("\tsubsystem_id        : %04x\n", p->u.h0.subsystem_id);
		dprintf("\tsubsystem_vendor_id : %04x [%s]\n", p->u.h0.subsystem_vendor_id, 
		        decode_vendor(p->u.h0.subsystem_vendor_id));
		dprintf("\trom_base_pci        : %08lx\n", p->u.h0.rom_base_pci);
		dprintf("\tinterrupt_line      : %02x\n", p->u.h0.interrupt_line);
		dprintf("\tinterrupt_pin       : %02x\n", p->u.h0.interrupt_pin);
		
	} else if (p->header_type == 1) {
		dprintf("Header Type 1 (PCI-PCI bridge)\n");
		dprintf("\trom_base_pci        : %08lx\n", p->u.h1.rom_base_pci);
		dprintf("\tinterrupt_line      : %02x\n", p->u.h1.interrupt_line);
		dprintf("\tinterrupt_pin       : %02x\n", p->u.h1.interrupt_pin);
		dprintf("\t2ndry status        : %04x\n", p->u.h1.secondary_status);
		dprintf("\tmemory_base         : %04x\n", p->u.h1.memory_base);
		dprintf("\tbridge control      : %02x\n", p->u.h1.bridge_control);
	}
#endif
}
				
static status_t test_me(void)
{
	pci_module_info *pcim;
	pci_info apci;
	long index = 0;
		
	if (get_module(B_PCI_MODULE_NAME, (module_info**)&pcim) != 0) {
		dprintf("config_manager: test_me: failed to load PCI module\n");
		return -1;
	}

	while (pcim->get_nth_pci_info(index++, &apci) == 0) {
		show_pci_details(&apci, pcim);
	}
			
	put_module(B_PCI_MODULE_NAME);
	return 0;
}
				
/* device_modules */
static int cfdm_std_ops(int32 op, ...)
{
	switch(op) {
		case B_MODULE_INIT:
			dprintf( "config_manager: device modules: init\n" );
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
