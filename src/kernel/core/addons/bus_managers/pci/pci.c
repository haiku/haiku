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
#include <smp.h> /* for spinlocks */
#include <arch/cpu.h>
#include <arch/int.h>

#include <string.h>
#include <stdio.h>

#include <PCI.h>
#include <bus.h>

/* Change to 1 to see more debugging :) */
#define THE_FULL_MONTY 0

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

static spinlock_t pci_config_lock = 0;  /* lock for config space access */
static int        pci_mode = 1;         /* The pci config mechanism we're using.
                                         * NB defaults to 1 as this is more common, but
                                         * checked at runtime
                                         */
static int        bus_max_devices = 32; /* max devices that any bus can support
                                         * Yes, if we're using pci_mode == 2 then
                                         * this is only 16, instead of the 32 we
                                         * have with pci_mode == 1
                                         */
static region_id  pci_region;           /* pci_bios region we map */
static void *     pci_bios_ptr= NULL;   /* virtual address of memory we map */

struct pci_config *pci_list;

/* Config space locking!
 * We need to make sure we only have one access at a time into the config space,
 * so we'll use a spinlock and also disbale interrupts so if we're smp we
 * won't have problems.
 */

#define PCI_LOCK_CONFIG(status) \
{ \
	status = disable_interrupts(); \
	acquire_spinlock(&pci_config_lock); \
}

#define PCI_UNLOCK_CONFIG(cpu_status) \
{ \
	release_spinlock(&pci_config_lock); \
	restore_interrupts(cpu_status); \
}

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

#define CONFIG_ADDR_1(bus, device, func, reg) \
	(0x80000000 | (bus << 16) | (device << 11) | (func << 8) | (reg & ~3))

#define CONFIG_ADDR_2(dev, reg) \
	(uint16)(0xC00 | (dev << 8) | reg)
	
static uint32 read_pci_config(uchar bus, uchar device, uchar function, uchar reg, uchar size)
{
	int cpu_status;
	uint32 val = 0;

	PCI_LOCK_CONFIG(cpu_status);

	if (pci_mode == 1) {
		/* write request details */
		out32(CONFIG_ADDR_1(bus, device, function, reg), CONFIG_REQ_PORT);	
		/* Now read data back from the data port...
		 * offset for 1 byte can be 1,2 or 3
		 * offset for 2 bytes can be 1 or 2
		 */
		switch (size) {
			case 1:
				val = in8 (CONFIG_DATA_PORT + (reg & 3));
				break;
			case 2:
				val = in16(CONFIG_DATA_PORT + (reg & 2));
				break;
			case 4:
				val = in32(CONFIG_DATA_PORT + reg);
				break;
			default:
				dprintf("ERROR: mech #1: read_pci_config: called for %d bytes!!\n", size);
		}
	} else if (pci_mode == 2) {
		if (!(device & 0x10)) {
			out8((uint8)(0xF0 | (function << 1)), 0xCF8);
			out8(bus, 0xCFA);
		
			switch (size) {
				case 1:
					val = in8 (CONFIG_ADDR_2(device, reg));
					break;
				case 2:
					val = in16(CONFIG_ADDR_2(device, reg));
					break;
				case 4:
					val = in32(CONFIG_ADDR_2(device, reg));
					break;
				default:
					dprintf("ERROR: mech #2: read_pci_config: called for %d bytes!!\n", size);
			}
			out8(0, 0xCF8);
		} else 
			val = EINVAL;
	} else
		dprintf("PCI: Config Mechanism %d isn't known!\n", pci_mode);

	PCI_UNLOCK_CONFIG(cpu_status);
	return val;
}

static void write_pci_config(uchar bus, uchar device, uchar function, uchar reg, 
                             uchar size, uint32 value)
{
	int cpu_status;
	
	PCI_LOCK_CONFIG(cpu_status);
	
	if (pci_mode == 1) {
		/* write request details */
		out32(CONFIG_ADDR_1(bus, device, function, reg), CONFIG_REQ_PORT);	
		/* Now read data back from the data port...
		 * offset for 1 byte can be 1,2 or 3
		 * offset for 2 bytes can be 1 or 2
		 */
		switch (size) {
			case 1:
				out8 (value, CONFIG_DATA_PORT + (reg & 3));
				break;
			case 2:
				out16(value, CONFIG_DATA_PORT + (reg & 2));
				break;
			case 4:
				out32(value, CONFIG_DATA_PORT + reg);
				break;
			default:
				dprintf("ERROR: write_pci_config: called for %d bytes!!\n", size);
		}
	} else if (pci_mode == 2) {
		if (!(device & 0x10)) {
			out8((uint8)(0xF0 | (function << 1)), 0xCF8);
			out8(bus, 0xCFA);
		
			switch (size) {
				case 1:
					out8 (value, CONFIG_ADDR_2(device, reg));
					break;
				case 2:
					out16(value, CONFIG_ADDR_2(device, reg));
					break;
				case 4:
					out32(value, CONFIG_ADDR_2(device, reg));
					break;
				default:
					dprintf("ERROR: write_pci_config: called for %d bytes!!\n", size);
			}
			out8(0, 0xCF8);
		}
	} else
		dprintf("PCI: Config Mechanism %d isn't known!\n", pci_mode);

	PCI_UNLOCK_CONFIG(cpu_status);
	
	return;
}

/* pci_get_capability
 * Try to get the offset and value of the specified capability from the
 * devices capability list.
 * returns 0 if unable, 1 if succesful
 */
static int pci_get_capability(uint8 bus, uint8 dev, uint8 func, uint8 cap, 
                              uint8 *offs)
{
	uint16 status = read_pci_config(bus, dev, func, PCI_status, 2);
	uint8 hdr_type, reg_data;
	uint8 ofs;
	
	if (!(status & PCI_status_capabilities))
		return 0;
		
	hdr_type = read_pci_config(bus, dev, func, PCI_header_type, 1);
	switch(hdr_type) {
		case PCI_header_type_generic:
			ofs = PCI_capabilities_ptr;
			break;
		case PCI_header_type_cardbus:
			ofs = PCI_capabilities_ptr_2;
			break;
		default:
			return 0;
	}
	
	ofs = read_pci_config(bus, dev, func, ofs, 1);
	while (ofs != 0) {
		reg_data = read_pci_config(bus, dev, func, ofs, 2);
		if ((reg_data & 0xff) == cap) {
			if (offs)
				*offs = ofs;
			return 1;
		}
		ofs = (reg_data >> 8) & 0xff;
	}
	return 0;
}

//static int pci_set_power_state

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

/* Intel specific 
 *
 * See http://www.microsoft.com/HWDEV/busbios/PCIIRQ.htm
 *
 * Intel boards have a PCI IRQ Routing table that contains details of
 * how things get routed, information we need :(
 */

struct linkmap {
	uint8  pin;
	uint16 possible_irq;
} _PACKED;

struct pir_slot {
	uint8  bus;
	uint8  devfunc;
	struct linkmap linkmap[4];
	uint8  slot;
	uint8  reserved;
} _PACKED;

#define PIR_HEADER "$PIR"

struct pir_header {
	char   signature[4];     /* always '$PIR' */
	uint16 version;
	uint16 tbl_sz;
	uint8  router_bus;
	uint8  router_devfunc;
	uint16 exclusive_irq;
	uint32 compat_vend;
	uint32 miniport;
	uint8  rsvd[11];
	uint8  cksum;
} _PACKED;

struct pir_table {
	struct pir_header hdr;
	struct pir_slot slot[1];
} _PACKED;

#define PIR_DEVICE(devfunc)     (((devfunc) >> 3) & 0x1f)
#define PIR_FUNCTION(devfunc)   ((devfunc) & 0x07)

/* find_pir_table
 * No real magic, just scan through the memory until we find it, 
 * or not!
 *
 * When we check the size of the table, we need to satisfy that
 *  size >= size of a pir_table with no pir_slots
 *  size <= size of a pir_table with 32 pir_slots
 * If we find a table, and it's a suitable size we'll check that the
 * checksum is OK. This is done by simply adding up all the bytes and
 * checking that the final value == 0. If it is we return the pointer to the table.
 *
 * Returns NULL if we fail to find a suitable table.
 */
static struct pir_table *find_pir_table(void)
{
	uint32 mem_addr = (uint32)pci_bios_ptr;
	int range = 0x10000;
	
	for (; range > 0; range -= 16, mem_addr += 16) {
		if (memcmp((void*)mem_addr, PIR_HEADER, 4) == 0) {
			uint16 size = ((struct pir_header*)mem_addr)->tbl_sz;
			if ((size >= (sizeof(struct pir_header))) &&
			    (size <= (sizeof(struct pir_header) + sizeof(struct pir_slot) * 32))) {
				uint16 i;
				uint8 cksum = 0;
				
				for (i = 0; i < size; i++)
					cksum += ((uint8*)mem_addr)[i];
				if (cksum == 0)
					return (struct pir_table *)mem_addr;
			}
		}
	}
	return NULL;
}

/* print_pir_table
 * Print out the table to debug output
 */
#if THE_FULL_MONTY
static void print_pir_table(struct pir_table *tbl)
{
	int i, j;
	int entries = (tbl->hdr.tbl_sz - sizeof(struct pir_header)) / sizeof(struct pir_slot);

	for (i=0; i < entries; i++) {
		dprintf("PIR slot %d: bus %d, device %d\n",
		        tbl->slot[i].slot, tbl->slot[i].bus,
		        PIR_DEVICE(tbl->slot[i].devfunc));
		for (j=0; j < 4; j++)
			dprintf("\tINT%c: pin %d possible_irq's %04x\n",
			        'A'+j, tbl->slot[i].linkmap[j].pin,
			        tbl->slot[i].linkmap[j].possible_irq);
	}
	dprintf("*** end of table\n");
}
#endif

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
				uint8 offset;
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

				if (pci_get_capability(bus, dev, func, PCI_cap_id_agp, &offset))
					dprintf("Device @ %d:%d:%d has AGP capability\n", bus, dev, func);
				if (pci_get_capability(bus, dev, func, PCI_cap_id_pm, &offset))
					dprintf("Device @ %d:%d:%d has Power Management capability\n", bus, dev, func);
				
				/* basic header */
				pcii->bus = bus;
				pcii->device = dev;
				pcii->function = func;
				pcii->vendor_id = vendor_id;
				pcii->device_id = device_id;

				pcii->revision =    read_pci_config(bus, dev, func, PCI_revision, 1);
				pcii->class_api =   read_pci_config(bus, dev, func, PCI_class_api, 1);
				pcii->class_sub =   read_pci_config(bus, dev, func, PCI_class_sub, 1);
				pcii->class_base =  read_pci_config(bus, dev, func, PCI_class_base, 1);

				pcii->header_type = read_pci_config(bus, dev, func, PCI_header_type, 1);
				pcii->header_type &= PCI_header_type_mask;			        		

				npcid->info = pcii;
				/* Add the device to the list */
				insque(npcid, &pci_dev_list);
			}
		}
	}
}

static void fill_pci_structure(pci_info *pcii)
{
	uint8 bus = pcii->bus, dev = pcii->device, func = pcii->function;
	uint8 int_line = 0, int_pin = 0;
	
	pcii->latency =     read_pci_config(bus, dev, func, PCI_latency, 1);
	pcii->bist =        read_pci_config(bus, dev, func, PCI_bist, 1);
	pcii->line_size =   read_pci_config(bus, dev, func, PCI_line_size, 1);
				
	int_line =          read_pci_config(bus, dev, func, PCI_interrupt_line, 1);
	int_pin =           read_pci_config(bus, dev, func, PCI_interrupt_pin, 1);
	int_pin &= PCI_pin_mask;

	/* device type specific headers based on header_type declared */
	if (pcii->header_type == PCI_header_type_generic) {
		/* header type 0 */
		pcii->u.h0.cardbus_cis =         read_pci_config(bus, dev, func, PCI_cardbus_cis, 4);
		pcii->u.h0.subsystem_id =        read_pci_config(bus, dev, func, PCI_subsystem_id, 2);
		pcii->u.h0.subsystem_vendor_id = read_pci_config(bus, dev, func, PCI_subsystem_vendor_id, 2);
		pcii->u.h0.rom_base_pci =        read_pci_config(bus, dev, func, PCI_rom_base, 4);
		pcii->u.h0.interrupt_line =      int_line;
		pcii->u.h0.interrupt_pin =       int_pin;			
	} else if (pcii->header_type == PCI_header_type_PCI_to_PCI_bridge) {
		/* header_type 1 */
		/* PCI-PCI bridge - may be used for AGP */				
		pcii->u.h1.rom_base_pci =        read_pci_config(bus, dev, func, PCI_bridge_rom_base, 4);
		pcii->u.h1.primary_bus =         read_pci_config(bus, dev, func, PCI_primary_bus, 1);
		pcii->u.h1.secondary_bus =       read_pci_config(bus, dev, func, PCI_secondary_bus, 1);
		pcii->u.h1.secondary_latency =   read_pci_config(bus, dev, func, PCI_secondary_latency, 1);
		pcii->u.h1.secondary_status =    read_pci_config(bus, dev, func, PCI_secondary_status, 2);
		pcii->u.h1.subordinate_bus =     read_pci_config(bus, dev, func, PCI_subordinate_bus, 1);
		pcii->u.h1.memory_base =         read_pci_config(bus, dev, func, PCI_memory_base, 2);
		pcii->u.h1.memory_limit =        read_pci_config(bus, dev, func, PCI_memory_limit, 2);
		pcii->u.h1.prefetchable_memory_base =  read_pci_config(bus, dev, func, PCI_prefetchable_memory_base, 2);
		pcii->u.h1.prefetchable_memory_limit = read_pci_config(bus, dev, func, PCI_prefetchable_memory_limit, 2);
		pcii->u.h1.bridge_control =      read_pci_config(bus, dev, func, PCI_bridge_control, 1);
		pcii->u.h1.subsystem_vendor_id = read_pci_config(bus, dev, func, PCI_sub_vendor_id_1, 2);
		pcii->u.h1.subsystem_id        = read_pci_config(bus, dev, func, PCI_sub_device_id_1, 2);		
		pcii->u.h1.interrupt_line =      int_line;
		pcii->u.h1.interrupt_pin =       int_pin;
					
	} else if (pcii->header_type == PCI_header_type_cardbus) {
		pcii->u.h2.subsystem_vendor_id = read_pci_config(bus, dev, func, PCI_sub_vendor_id_2, 2);
		pcii->u.h2.subsystem_id =        read_pci_config(bus, dev, func, PCI_sub_device_id_2, 2);		
		
	}else if (pcii->header_type == PCI_multifunction) {
		dprintf("PCI: multifunction device detected\n");
		/* ??? */
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

	fill_pci_structure(fpd->info);
	
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

static void pci_module_init(void)
{
	struct pir_table *pirt = NULL;
	
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

	pci_region = vm_map_physical_memory(vm_get_kernel_aspace_id(),
	                                    "pci_bios", &pci_bios_ptr,
	                                    REGION_ADDR_ANY_ADDRESS,
	                                    0x10000,
	                                    LOCK_RO | LOCK_KERNEL,
	                                    (addr)0xf0000);
	

	pirt = find_pir_table();
	if (pirt) {
		dprintf("PCI IRQ Routing table found\n");
#if THE_FULL_MONTY
		print_pir_table(pirt);
#endif
	}
	
	/* Get our initial list of devices */
	scan_pci();
}

static void pci_module_uninit(void)
{
	vm_delete_region(vm_get_kernel_aspace_id(), pci_region);
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
			pci_module_uninit();
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

