/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <KernelExport.h>
#include <kernel.h>
#include <vfs.h>
#include <debug.h>
#include <lock.h>

#include <string.h>

#include <pci_bus.h>

#include "pci_p.h" // private includes

/* While this will now print details of all the PCI devices it finds, for a full
 * description of the devices you'll need to add the information. As the header is
 * 275k I haven't just included it while we're still working on a floppy image,
 * but rather it's inclusion is optional.
 *
 * If you want to include the header for more information, you'll need to
 *   1) get a copy of pcihdr.h from http://www.yourvote.com/pci/ and modify it
 *   2) copy it into the pci directory (ie here)
 *   3) change the define of USE_PCIHDR from 0 to 1
 *   4) rebuild
 */
#define USE_PCIHDR 0
#if USE_PCIHDR
#	include "pcihdr.h"
#endif

int pci_scan_all();

struct pci_fs {
	mount_id id;
	mutex lock;
	void *covered_vnode;
	void *redir_vnode;
	int root_vnode;              /* just a placeholder to return a pointer to */
};


static unsigned int
pci_read_data(uint8 bus, uint8 unit, uint8 function, int reg, int bytes)
{
	struct pci_config_address addr;
	addr.enable = 1;
	addr.reserved = 0;
	addr.bus = bus;
	addr.unit = unit;
	addr.function = function;
	addr.reg = reg & 0xfc;

	out32(*(unsigned int *)&addr, CONFIG_ADDRESS);
	switch(bytes) {
		case 1:
			return in8(CONFIG_DATA + (reg & 3));
		case 2:
			return in16(CONFIG_DATA + (reg & 3));
		case 4:
			return in32(CONFIG_DATA + (reg & 3));
		default:
			return 0;
	}
}


static void
pci_write_data(uint8 bus, uint8 unit, uint8 function, int reg, uint32 data, int bytes)
{
	struct pci_config_address addr;
	addr.enable = 1;
	addr.reserved = 0;
	addr.bus = bus;
	addr.unit = unit;
	addr.function = function;
	addr.reg = reg & 0xfc;

	out32(*(unsigned int *)&addr, CONFIG_ADDRESS);
	switch(bytes) {
		case 1:
			out8(data, CONFIG_DATA + (reg & 3));
			break;
		case 2:
			out16(data, CONFIG_DATA + (reg & 3));
			break;
		case 4:
			out32(data, CONFIG_DATA + (reg & 3));
			break;
	}
	return;
}


static int
pci_read_config(uint8 bus, uint8 unit, uint8 func, struct pci_cfg *cfg)
{
	union {
		struct pci_cfg cfg;
		uint32 word[4];
	} u;
	int i;

	for (i = 0; i<4; i++) {
		u.word[i] = pci_read_data(bus, unit, func, 4*i, 4);
	}
	if (u.cfg.vendor_id == 0xffff)
		return -1;

	// move over to the passed in config structure
	memcpy(cfg, &u.cfg, sizeof(struct pci_cfg));
	cfg->bus = bus;
	cfg->unit = unit;
	cfg->func = func;

	switch (cfg->header_type & 0x7f) {
		case 0: { // normal device
			uint32 v;
			for(i=0; i<6; i++) {
				v = pci_read_data(bus, unit, func, i*4 + 0x10, 4);
				if(v) {
					int v2;
					pci_write_data(bus, unit, func, i*4 + 0x10, 0xffffffff, 4);
					v2 = pci_read_data(bus, unit, func, i*4 + 0x10, 4) & 0xfffffff0;
					pci_write_data(bus, unit, func, i*4 + 0x10, v, 4);
					v2 = 1 + ~v2;
					if(v & 1) {
						cfg->base[i] = v & 0xfff0;
						cfg->size[i] = v2 & 0xffff;
					} else {
						cfg->base[i] = v & 0xfffffff0;
						cfg->size[i] = v2;
					}
				} else {
					cfg->base[i] = 0;
					cfg->size[i] = 0;
				}
			}
			v = pci_read_data(bus, unit, func, 0x3c, 1);
			cfg->irq = (v == 0xff ? 0 : v);
			break;
		}
		case 1: //  PCI <-> PCI bridge
			break;
		default:
			break;
	}
	return 0;
}


static const char *
pci_class_to_string(uint8 base_class)
{
	switch (base_class) {
		case 0x00: return "legacy";
		case 0x01: return "mass storage";
		case 0x02: return "network";
		case 0x03: return "video";
		case 0x04: return "multimedia";
		case 0x05: return "memory";
		case 0x06: return "bridge";
		case 0x07: return "comms";
		case 0x08: return "system";
		case 0x09: return "input";
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


static const char *
pci_subclass_to_string(uint8 base_class, uint8 sub_class)
{
	switch(base_class) {
		case 0: // legacy
			return "unknown";
		case 1: // mass storage
			switch(sub_class) {
				case 0: return "scsi";
				case 1: return "ide";
				case 2: return "floppy";
				case 3: return "ipi";
				default: return "unknown";
			}
		case 2: // network
			switch(sub_class) {
				case 0: return "ethernet";
				case 1: return "token_ring";
				case 2: return "fddi";
				default: return "unknown";
			}
		case 3: // video
			switch(sub_class) {
				case 0: return "vga";
				case 1: return "xga";
				default: return "unknown";
			}
		case 4: // multimedia
			return "unknown";
		case 5: // memory
			switch(sub_class) {
				case 1: return "ram";
				default: return "unknown";
			}
		case 6: // bridge
			switch(sub_class) {
				case 0: return "host";
				case 1: return "isa";
				case 2: return "eisa";
				case 3: return "mca";
				case 4: return "pcipci";
				case 5: return "pcmcia";
				case 0x80: return "other";
				default: return "unknown";
			}
		default: return "unknown";
	}
}


void
dump_pci_config(struct pci_cfg *cfg)
{
	int i;

	dprintf("dump_pci_config: dumping cfg structure at %p\n", cfg);

	dprintf("\tbus: %d, unit: %d, function: %d\n", cfg->bus, cfg->unit, cfg->func);

	dprintf("\tvendor id: %d\n", cfg->vendor_id);
	dprintf("\tdevice id: %d\n", cfg->device_id);
	dprintf("\tcommand: %d\n", cfg->command);
	dprintf("\tstatus: %d\n", cfg->status);

	dprintf("\trevision id: %d\n", cfg->revision_id);
	dprintf("\tinterface: %d\n", cfg->interface);
	dprintf("\tsub class: %d '%s'\n", cfg->sub_class, pci_subclass_to_string(cfg->base_class, cfg->sub_class));

	dprintf("\tcache line size: %d\n", cfg->cache_line_size);
	dprintf("\tlatency timer: %d\n", cfg->latency_timer);
	dprintf("\theader type: %d\n", cfg->header_type);
	dprintf("\tbist: %d\n", cfg->bist);

	dprintf("\tirq: %d\n", cfg->irq);

	for (i = 0; i < 6; i++) {
		dprintf("\tbase[%d] = 0x%lx\n", i, cfg->base[i]);
		dprintf("\tsize[%d] = 0x%lx\n", i, cfg->size[i]);
	}
}


static void
dump_short_pci_config(struct pci_cfg *cfg)
{
#if !USE_PCIHDR
	dprintf("PCI: vendor id %d, device id %d ", cfg->vendor_id, cfg->device_id);
	dprintf(", base class: %d '%s'\n", cfg->base_class, pci_class_to_string(cfg->base_class));
#else
	int i,j;
	for (i = 0; i < PCI_VENTABLE_LEN; i++) {
		if (PciVenTable[i].VenId == cfg->vendor_id) {
			dprintf("PCI: %s: ", PciVenTable[i].VenFull);
			for (j=0; j < PCI_DEVTABLE_LEN; j++) {
				if (PciDevTable[j].VenId == cfg->vendor_id &&
				    PciDevTable[j].DevId == cfg->device_id)
					dprintf("%s %s", PciDevTable[j].Chip, PciDevTable[j].ChipDesc);
			}
			dprintf("\n");
		}
	}
#endif
}


int
pci_probe(uint8 bus, uint8 unit, uint8 function, struct pci_cfg *cfg)
{
	if (function > 0) {
		struct pci_cfg cfg1;

		// read info about the first unit
		if (pci_probe(bus, unit, 0, &cfg1) < 0)
			return -1;

		if (!(cfg1.header_type & 0x80)) {
			// no multiple functions for this dev
			return -1;
		}
	}

	if (pci_read_data(bus, unit, function, 0, 2) == 0xffff)
		return -1;

	if (pci_read_config(bus, unit, function, cfg) < 0)
		return -1;

	dump_short_pci_config(cfg);

	return 0;
}

#if 0

int
pci_scan_all()
{
	int bus;
	int unit;
	int function;

	dprintf("pci_scan_all: entry\n");

	for (bus = 0; bus < 255; bus++) {
		for (unit = 0; unit < 32; unit++) {
			for (function = 0; function < 8; function++) {
				struct pci_cfg cfg;

				if (pci_read_data(bus, unit, function, 0, 2) == 0xffff)
					break;

				if (pci_read_config(bus, unit, function, &cfg) < 0)
					break;

				dump_short_pci_config(&cfg);

				if (!(cfg.header_type & 0x80)) {
					// no multiple functions
					break;
				}
			}
		}
	}
	return 0;
}
#endif

