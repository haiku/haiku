/*
** Copyright 2001-2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <kernel.h>
#include <console.h>
#include <debug.h>
#include <memheap.h>
#include <int.h>
#include <lock.h>
#include <vm.h>
#include <devfs.h>
#include <khash.h>
#include <Errors.h>

#include <arch/cpu.h>
#include <arch/int.h>

#include <string.h>
#include <stdio.h>

#include <PCI.h>
#include <bus.h>
#include <pci_bus.h>

#include "pci_p.h" // private includes

struct found_pci_device {
	struct found_pci_device *next;
	struct found_pci_device *prev;
	pci_info *info;
};

static struct found_pci_device pci_dev_list;

struct pci_config {
	struct pci_config *next;
	char *full_path;
	struct pci_cfg *cfg;
};

/* The pci_mode we're using, defaults to 1 as this is more common */
static int pci_mode = 1;

/* If we have configuration mechanism one we have devices 0 - 32 per bus, if
 * we only have configuration mechanism two we have devices 0 - 16
 * XXX - set this when we determine which configuration mechanism we have
 */
static int bus_max_devices = 32;

struct pci_config *pci_list;

/* PCI has 2 Configuration Mechanisms. We need to decide which one the
 * PCI Host Bridge is speaking and then speak to it correctly. This is decided
 * in set_pci_mechanism() where the pci_mode value is set to the appropriate
 * value and the bus_max_devices value is set correctly.
 *
 * Mechanism 1
 * ===========
 * Mechanism 1 is the more common one found on modern computers, so presently
 * that's the one that has been tested and added.
 *
 * This has 2 ranges, one for addressing and for data. Basically we write in the details of
 * the configuration information we want (bus, device and function) and then either
 * read or write from the data port.
 *
 * Apparently most modern hardware no longer has Configuration Type #2.
 *
 * XXX - add code for Mechanism Two
 *
 *
 */

#define CONFIG_REQ_PORT    0xCF8
#define CONFIG_DATA_PORT   0xCFC

#define CONFIG_REQ(bus, device, func, offs) (0x80000000 | (bus << 16) | \
                                            (((device << 3) | (func & 0x07)) << 8) | \
                                            (offs & ~3))

static uint32 read_pci_config(uchar bus, uchar device, uchar function, uchar offset, uchar size)
{
	if (pci_mode == 1) {
		/* write request details */
		out32(CONFIG_REQ(bus, device, function, offset), 0xCF8);	
		/* Now read data back from the data port...
		 * offset for 1 byte can be 1,2 or 3
		 * offset for 2 bytes can be 1 or 2
		 */
		switch (size) {
			case 1:
				return in8 (CONFIG_DATA_PORT + (offset & 3));
			case 2:
				return in16(CONFIG_DATA_PORT + (offset & 2));
			case 4:
				return in32(CONFIG_DATA_PORT + offset);
			default:
				dprintf("read_pci_config: called for %d bytes!!\n", size);
		}
	} else if (pci_mode == 2) {
		dprintf("PCI: Config Mechanism 2 not yet supported!\n");
	} else
		dprintf("PCI: Config Mechanism %d isn't known!\n", pci_mode);

	return 0;
}

static void write_pci_config(uchar bus, uchar device, uchar function, uchar offset, 
                               uchar size, uint32 value)
{
	if (pci_mode == 1) {
		/* write request details */
		out32(CONFIG_REQ(bus, device, function, offset), 0xCF8);	
		/* Now read data back from the data port...
		 * offset for 1 byte can be 1,2 or 3
		 * offset for 2 bytes can be 1 or 2
		 */
		switch (size) {
			case 1:
				out8 (value, CONFIG_DATA_PORT + (offset & 3));
			case 2:
				out16(value, CONFIG_DATA_PORT + (offset & 2));
			case 4:
				out32(value, CONFIG_DATA_PORT + offset);
			default:
				dprintf("read_pci_config: called for %d bytes!!\n", size);
		}
	} else if (pci_mode == 2) {
		dprintf("PCI: Config Mechanism 2 not yet supported\n");
	} else
		dprintf("PCI: Config Mechanism %d isn't known!\n", pci_mode);
	
	return;
}

/* set_pci_mechanism()
 * Try to determine which configuration mechanism the PCI Host Bridge
 * wants to deal with.
 * XXX - we really should add code to detect and use a PCI BIOS if one
 *       exists, and this code then becomes the fallback. For now we'll
 *       just use this.
 */
static int set_pci_mechanism(void)
{
	uint32 ckval = 0x80000000;
	/* Start by looking for the older and more limited mechanism 2
	 * as the test will probably work for mechanism 1 as well.
	 *
	 * This code copied/adapted from OpenBSD
	 */
#define PCI_MODE2_ENABLE  0x0cf8
#define PCI_MODE2_FORWARD 0x0cfa
	out8(0, PCI_MODE2_ENABLE);
	out8(0, PCI_MODE2_FORWARD);
	if (in8(PCI_MODE2_ENABLE) == 0 && 
	    in8(PCI_MODE2_FORWARD) == 0) {
		dprintf("PCI_Mechanism 2 test passed\n");
		bus_max_devices = 16;
		pci_mode = 2;
		return 0;
	}

	/* If we get here, the first test (for mechanism 2) failed, so there
	 * is a good chance this one will pass. Basically enable then disable and
	 * make sure we have the same values.
	 */
#define PCI_MODE1_ADDRESS 0x0cf8
	out32(ckval, PCI_MODE1_ADDRESS);
	if (in32(PCI_MODE1_ADDRESS) == ckval) {
		out32(0, PCI_MODE1_ADDRESS);
		if (in32(PCI_MODE1_ADDRESS) == 0) {
			dprintf("PCI_Mechanism 1 test passed\n");
			bus_max_devices = 32;
			pci_mode = 1;
			return 0;
		}
	}

	dprintf("PCI: Failed to find a valid PCI Configuration Mechanism!\n"
	        "PCI: disabled\n");
	pci_mode = 0;

	return -1;
} 
		
/* check_pci()
 * Basically PCI bus #0 "should" contain a PCI Host Bridge.
 * This can be identified by the 8 bit pci class base of 0x06.
 *
 * XXX - this is pretty simplistic and needs improvement. In particular
 *       some Intel & Compaq bridges don't have the correct class base set.
 *       Need to review these. For the time being if this sanity check
 *       fails it won't be a hanging offense, but it will generate a
 *       message asking for the info to be sent to the kernel list so we
 *       can refine this code. :) Assuming anyone ever looks at the 
 *       debug output!
 *
 * returns  0 if PCI seems to be OK
 *         -1 if PCI fails the test
 */ 
static int check_pci(void)
{
	int dev = 0;

	/* Scan through the first 16 devices on bus 0 looking for 
	 * a PCI Host Bridge
	 */	
	for (dev = 0; dev < bus_max_devices; dev++) {
		uint8 val = read_pci_config(0, dev, 0, PCI_class_base, 1);
		if (val == 0x06)
			return 0;
	}
	/* Bit wordy, but it needs to be :( */
	dprintf("*** PCI Warning! ***\n"
	        "The PCI sanity check appears to have failed on your system.\n"
	        "This is probably due to the test being used, so please email\n"
	        "\topen-beos-kernel-devel@lists.sourceforge.net\n"
	        "Your assistance will help improve this test :)\n"
	        "***\n"
	        "PCI will attempt to continue normally.\n");
	return -1;
}
	
	
static void scan_pci(void)
{
	int bus, dev, func; 	
	
	/* We can have up to 255 busses */
	for(bus = 0; bus < 255; bus++) {
		/* Each bus can have up to 'bus_max_devices' devices on it */
		for(dev = 0; dev <= bus_max_devices; dev++) {
			/* Each device can have up to 8 functions */
			uint16 sv_vendor_id = 0, sv_device_id = 0;
			
			for (func = 0; func < 8; func++) {
				pci_info *pcii = NULL;
				struct found_pci_device *npcid = NULL;
				uint16 vendor_id = read_pci_config(bus, dev, func, 0, 2);
				uint16 device_id;
				uint8 int_line = 0, int_pin = 0;
				/* If we get 0xffff then there is no device here. As there can't
				 * be any gaps in function allocation this tells us that we
				 * can move onto the next device/bus
				 */
				if (vendor_id == 0xffff)
					break;

				device_id = read_pci_config(bus, dev, func, PCI_device_id, 2);

				/* Is this a new device? As we're scanning the functions here, many devices
				 * will supply identical information for all 8 accesses! Try to catch this and
				 * simply move past duplicates. We need to continue scanning in case we miss
				 * a different device at the end.
				 * XXX - is this correct????
				 */
				if (vendor_id == sv_vendor_id && sv_device_id == device_id) {
					/* It's a duplicate */
					continue;
				}
				sv_vendor_id = vendor_id;
				sv_device_id = device_id;
				
				/* At present we will add a device to our list if we get here,
				 * but we may want to review if we need to add 8 version of the
				 * same device if only the functions differ?
				 */
				if ((pcii = (pci_info*)kmalloc(sizeof(pci_info))) == NULL) {
					dprintf("Failed to get memory for a pic_info structure in scan_pci\n");
					return;
				}
				if ((npcid = (struct found_pci_device*)kmalloc(sizeof(struct found_pci_device))) == NULL) {
					kfree(pcii);
					dprintf("scan_pci: failed to kmalloc memory for found_pci_device structure\n");
					return;
				}

				/* basic header */
				pcii->vendor_id = vendor_id;
				pcii->device_id = device_id;
				pcii->bus = bus;
				pcii->device = dev;
				pcii->function = func;
				pcii->revision =    read_pci_config(bus, dev, func, PCI_revision, 1);
				pcii->class_api =   read_pci_config(bus, dev, func, PCI_class_api, 1);
				pcii->class_sub =   read_pci_config(bus, dev, func, PCI_class_sub, 1);
				pcii->class_base =  read_pci_config(bus, dev, func, PCI_class_base, 1);
				pcii->line_size =   read_pci_config(bus, dev, func, PCI_line_size, 1);
				pcii->latency =     read_pci_config(bus, dev, func, PCI_latency, 1);
				pcii->header_type = read_pci_config(bus, dev, func, PCI_header_type, 1);			        
				pcii->bist =        read_pci_config(bus, dev, func, PCI_bist, 1);
				
				int_line =          read_pci_config(bus, dev, func, PCI_interrupt_line, 1);
				int_pin =           read_pci_config(bus, dev, func, PCI_interrupt_pin, 1);
				
				/* device type specific headers based on header_type declared */
				if (pcii->header_type == 0) {
					/* header type 0 */
					pcii->u.h0.cardbus_cis =         read_pci_config(bus, dev, func, PCI_cardbus_cis, 4);
					pcii->u.h0.subsystem_id =        read_pci_config(bus, dev, func, PCI_subsystem_id, 2);
					pcii->u.h0.subsystem_vendor_id = read_pci_config(bus, dev, func, PCI_subsystem_vendor_id, 2);
					pcii->u.h0.rom_base_pci =        read_pci_config(bus, dev, func, PCI_rom_base, 4);
					pcii->u.h0.interrupt_line =      int_line;
					pcii->u.h0.interrupt_pin =       int_pin;					
				} else if (pcii->header_type == 1) {
					/* header_type 1 */
					/* PCI-PCI bridge - may be used for AGP */
					pcii->u.h1.rom_base_pci =        read_pci_config(bus, dev, func, PCI_bridge_rom_base, 4);
					pcii->u.h1.primary_bus =         read_pci_config(bus, dev, func, PCI_primary_bus, 1);
					pcii->u.h1.secondary_bus =       read_pci_config(bus, dev, func, PCI_secondary_bus, 1);
					pcii->u.h1.secondary_latency =   read_pci_config(bus, dev, func, PCI_secondary_latency, 1);
					pcii->u.h1.secondary_status =    read_pci_config(bus, dev, func, PCI_secondary_status, 2);
					pcii->u.h1.subordinate_bus =     read_pci_config(bus, dev, func, PCI_subordinate_bus, 1);
					pcii->u.h1.io_base =             read_pci_config(bus, dev, func, PCI_io_base, 1);
					pcii->u.h1.io_limit =            read_pci_config(bus, dev, func, PCI_io_limit, 1);
					pcii->u.h1.memory_base =         read_pci_config(bus, dev, func, PCI_memory_base, 2);
					pcii->u.h1.memory_limit =        read_pci_config(bus, dev, func, PCI_memory_limit, 2);
					pcii->u.h1.prefetchable_memory_base =  read_pci_config(bus, dev, func, PCI_prefetchable_memory_base, 2);
					pcii->u.h1.prefetchable_memory_limit = read_pci_config(bus, dev, func, PCI_prefetchable_memory_limit, 2);
					pcii->u.h1.bridge_control =      read_pci_config(bus, dev, func, PCI_bridge_control, 1);
					pcii->u.h1.interrupt_line =      int_line;
					pcii->u.h1.interrupt_pin =       int_pin;
				} else if (pcii->header_type == 0x80) {
					/* ??? */
				}

				npcid->info = pcii;
				/* Add the device to the list */
				insque(npcid, &pci_dev_list);
			}
		/* next device */	
		}
	/* next bus */
	}
}

/* XXX - check the return values from this function. */
static long get_nth_pci_info(long index, pci_info *copyto)
{
	/* We copy the index and then decrement it.
	 * We also set the start found_pci_device pointer to
	 * the first device found and inserted.
	 * XXX - see discussion below.
	 */
	long iter = index;
	struct found_pci_device *fpd = pci_dev_list.prev;

	/* iterate through the list until we have iter == 0
	 * NB we go in "reverse" as the devices are inserted into the
	 * list at the start, so going "forward" would give us the last
	 * device first.
	 * XXX - do we want to reverse this decision and go "forwards"?
	 *       this should reduce the number of tries we make to find
	 *       a "device" as the system devices are the first ones found
	 *       and therefore the last ones to be returned.
	 * XXX - if we do decide to reverse the order we need to be very
	 *       careful about which function # gets found first and returned.
	 */
	while (iter-- > 0 && fpd && fpd != &pci_dev_list)
		fpd = fpd->prev;

	/* 2 cases we bail here.
	 * 1) we don't have enough devices to fulfill the request 
	 *    e.g. user asked for dev #31 but we only have 30
	 * 2) The found_device_structure has a NULL pointer for
	 *    info, which would cause a segfault when we try to memcpy!
	 */
	if (iter > 0 || !fpd->info)
		return B_DEV_ID_ERROR;
	
	memcpy(copyto, fpd->info, sizeof(pci_info));
	return 0;
}

/* I/O routines */
static uint8 pci_read_io_8(int mapped_io_addr)
{
	return in8(mapped_io_addr);
}

static void pci_write_io_8(int mapped_io_addr, uint8 value)
{
	out8(value, mapped_io_addr);
}

static uint16 pci_read_io_16(int mapped_io_addr)
{
	return in16(mapped_io_addr);
}

static void pci_write_io_16(int mapped_io_addr, uint16 value)
{
	out16(value, mapped_io_addr);
}

static uint32 pci_read_io_32(int mapped_io_addr )
{
	return in32(mapped_io_addr);
}

static void pci_write_io_32(int mapped_io_addr, uint32 value)
{
	out32(value, mapped_io_addr);
}

static int pci_open(const char *name, uint32 flags, void * *_cookie)
{
	struct pci_config *c = (struct pci_config *)kmalloc(sizeof(struct pci_config));

	/* name points at the devfs_vnode so this is safe as we have to have a shorter
	 * lifetime than the vnode and the devfs_vnode 
	 */
	c->full_path = (char*)name;
	dprintf("pci_open: entry on '%s'\n", c->full_path);

	*_cookie = c;

	return 0;
}

static int pci_freecookie(void * cookie)
{
	kfree(cookie);
	return 0;
}

static int pci_close(void * cookie)
{
	return 0;
}

static ssize_t pci_read(void *cookie, off_t pos, void *buf, size_t *len)
{
	*len = 0;
	return EPERM;
}

static ssize_t pci_write(void * cookie, off_t pos, const void *buf, size_t *len)
{
	return EPERM;
}

static int pci_ioctl(void * _cookie, uint32 op, void *buf, size_t len)
{
	struct pci_config *cookie = _cookie;
	int err = 0;

	switch(op) {
		case PCI_GET_CFG:
			if(len < sizeof(struct pci_cfg)) {
				err= -1;
				goto err;
			}

			err = user_memcpy(buf, cookie->cfg, sizeof(struct pci_cfg));
			break;
		case PCI_DUMP_CFG:
			dump_pci_config(cookie->cfg);
			break;
		default:
			err = EINVAL;
			goto err;
	}

err:
	return err;
}

device_hooks pci_hooks = {
	&pci_open,
	&pci_close,
	&pci_freecookie,
	&pci_ioctl,
	&pci_read,
	&pci_write,
	NULL, /* select */
	NULL, /* deselect */
	NULL, /* readv */
	NULL  /* writev */
};

static int pci_create_config_structs()
{
	int bus, unit, function;
	struct pci_cfg *cfg = NULL;
	struct pci_config *config;
	char char_buf[SYS_MAX_PATH_LEN];

	dprintf("pcifs_create_vnode_tree: entry\n");

	for(bus = 0; bus < 256; bus++) {
		char bus_txt[4];
		sprintf(bus_txt, "%d", bus);
		for(unit = 0; unit < 32; unit++) {
			char unit_txt[3];
			sprintf(unit_txt, "%d", unit);
			for(function = 0; function < 8; function++) {
				char func_txt[2];
				sprintf(func_txt, "%d", function);

				if(cfg == NULL)
					cfg = kmalloc(sizeof(struct pci_cfg));
				if(pci_probe(bus, unit, function, cfg) < 0) {
					// not possible for a unit to have a hole in functions
					// if we dont find one in this unit, there are no more
					break;
				}

				config = kmalloc(sizeof(struct pci_config));
				if(!config)
					panic("pci_create_config_structs: error allocating pci config struct\n");

				sprintf(char_buf, "bus/pci/%s/%s/%s/ctrl", bus_txt, unit_txt, func_txt);
				dprintf("created node '%s'\n", char_buf);

				config->full_path = kmalloc(strlen(char_buf)+1);
				strcpy(config->full_path, char_buf);

				config->cfg = cfg;
				config->next = NULL;

				config->next = pci_list;
				pci_list = config;

				cfg = NULL;
			}
		}
	}

	if(cfg != NULL)
		kfree(cfg);

	return 0;
}

int pci_bus_init(kernel_args *ka)
{
	struct pci_config *c;

	pci_list = NULL;

	dprintf("PCI: pci_bus_init()\n");

	pci_create_config_structs();

	// create device nodes
	for(c = pci_list; c; c = c->next) {
		devfs_publish_device(c->full_path, c, &pci_hooks);
	}

	// register with the bus manager
	bus_register_bus("/dev/bus/pci");

	return 0;
}

static void pci_module_init(void)
{
	/* init the double linked list */
	pci_dev_list.next = pci_dev_list.prev = &pci_dev_list;

	/* Determine how we communicate with the PCI Host Bridge */
	if (set_pci_mechanism() == -1) {
		return;
	}
	
	/* Check PCI is valid and we can use it. 
	 * Presently we don't do anything with this other than run it and
	 * inform the user if we pass/fail. If we fail we print a big message
	 * asking the user to get in touch to let us know about their system so
	 * we can refine the test.
	 */
	check_pci();
	/* Get our initial list of devices */
	scan_pci();
}

static int std_ops(int32 op, ...)
{
	switch(op) {
		case B_MODULE_INIT:
			dprintf( "PCI: init\n" );
			pci_module_init();
			break;
		case B_MODULE_UNINIT:
			dprintf( "PCI: uninit\n" );
			break;
		default:
			return EINVAL;
	}
	return B_OK;
}

struct pci_module_info pci_module = {
	{
		{
			B_PCI_MODULE_NAME,
			B_KEEP_LOADED,
			std_ops
		},
		NULL//		&pci_rescan
	},
	
	&pci_read_io_8,
	&pci_write_io_8,
	&pci_read_io_16,
	&pci_write_io_16,
	&pci_read_io_32,
	&pci_write_io_32,
	&get_nth_pci_info,
	&read_pci_config,
	&write_pci_config,
	NULL,//	&ram_address
};

module_info *modules[] = {
	(module_info *)&pci_module,
	NULL
};

