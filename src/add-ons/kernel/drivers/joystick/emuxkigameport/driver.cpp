/*
 * Copyright 2008 Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander Coers		Alexander.Coers@gmx.de
 *		Fredrik Modéen 		fredrik@modeen.se
 *		Axel Dörfler		axeld@pinc-software.de
 */

#include "driver.h"

#include <stdio.h>
#include <string.h>

#include <KernelExport.h>
#include <PCI.h>

#define TRACE_DRIVER
#ifdef TRACE_DRIVER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

int32	api_version = B_CUR_DRIVER_API_VERSION;

int num_names = 0;
int num_cards = 0;

char *gDeviceNames[MAX_CARDS + 1];
gameport_info cards[MAX_CARDS];

/* setup_card used to initialize cards, structures or hardware */
static status_t
setup_card (gameport_info* card)
{
	char * name = card->name;
	uint32 command_reg = 0; 
	area_id area;
	int32 base,size;
  
  	dprintf (DRIVER_NAME ": setup_card() trying to init structures \n");
    /* enable PCI i/o  */
	command_reg = PCI_command_io || PCI_command_master ;
	
	(*pci->write_pci_config)(card->info.bus,card->info.device, 
		card->info.function, PCI_command, 2, 0);
	
	/* disable all i/o -regs and bus_mastering */
  	base = card->info.u.h0.base_registers[0];
  	size = card->info.u.h0.base_register_sizes[0];
  	
 	(*pci->write_pci_config) (card->info.bus,card->info.device, 
  		card->info.function, 0x10, 2, base);
  	
  	(*pci->write_pci_config) (card->info.bus,card->info.device, 
  		card->info.function, PCI_command, 2, command_reg);
  	
  	/* enable i/o- regs and possible bus_mastering */
	dprintf (DRIVER_NAME ": setup_card() enabled card with i/o regs from "
		"0x%04x to 0x%04x \n",base,base+size-1);
  
	if ((*gameport->create_device)((int32)base, &card->joy.driver) < B_OK) {
		dprintf (DRIVER_NAME ": setup_card() Gameport setup failed! Failed to "
			"load Generic Gameport Module \n");
  		return B_ERROR;
	}
	
	sprintf(card->joy.name1, "joystick/"DRIVER_NAME "/%x", base);
	gDeviceNames[num_names++] = card->joy.name1;
	gDeviceNames[num_names] = NULL;
	return B_OK;
}


extern "C" status_t
init_hardware (void)
{
	status_t err = ENODEV;
	pci_info info;
	int ix = 0;
	uint32 buffer;
	
	/* probe PCI bus */
	if (get_module (pci_name, (module_info **)&pci))
		return ENOSYS;
	while ((*pci->get_nth_pci_info)(ix, &info) == B_OK) {
		/* look for HARDWARE */
		if (info.vendor_id == VENDOR_ID_CREATIVE && 
			info.device_id == DEVICE_ID_CREATIVE_EMU10K1) {
			err = B_OK;
			
			dprintf (DRIVER_NAME ": init_hardware() found one, Revision %02x\n"
				,info.revision);
 			
 			if (!(info.u.h0.subsystem_id == 0x20 ||
 				info.u.h0.subsystem_id == 0xc400 || 
 					(info.u.h0.subsystem_id == 0x21 && info.revision < 6))) {
	    		buffer = (*pci->read_io_32)(info.u.h0.base_registers[0] + HCFG);
	    		buffer |= HCFG_JOYENABLE;
	    		(*pci->write_io_32)(info.u.h0.base_registers[0] + HCFG, buffer);
	   		}
			/* Some SB-Live cards need to the Joyenable Bit in the config 
				Register, others don´t */ 
		}
	ix++;
	}
	put_module (B_PCI_MODULE_NAME);
	return err;
}


extern "C" status_t 
init_driver (void)
{
	area_id	area;
	area_info	ainfo;
	pci_info info;
    int ix = 0;
	dprintf (DRIVER_NAME ": init_driver() " __DATE__ " " __TIME__ "\n");

	/* probe PCI bus */
	if (get_module (pci_name, (module_info **)&pci))
		return ENOSYS;
	
	if (get_module (gameport_name, (module_info **)&gameport)) {
		dprintf (DRIVER_NAME ": Failed to load Generic Gameport Module \n");
		put_module (pci_name);
		return ENOSYS;
	}
	
	while ((*pci->get_nth_pci_info)(ix, &info) == B_OK) {
		if (info.vendor_id == VENDOR_ID_CREATIVE && (
			info.device_id == SBLIVE_ID || 
			info.device_id == AUDIGY_ID ||
			info.device_id == SBLIVE_DELL_ID)) {
				
			if (num_cards == MAX_CARDS) {
				dprintf(DRIVER_NAME  ": Too many cards installed!\n");
				break;
			}
						
			memset(&cards[num_cards], 0, sizeof(gameport_info));
			cards[num_cards].info = info;
			if (setup_card(&cards[num_cards]))
				dprintf(DRIVER_NAME ": Setup of card %ld failed \n", 
					num_cards + 1);
			else
				num_cards++;
		}
		ix++;
	}
	
	if (!num_cards) {
		dprintf(DRIVER_NAME ": no cards \n");
		put_module(pci_name);
		return ENODEV;
    }
    return B_OK;
}


void
uninit_driver (void)
{
	int ix = 0;
	area_id area;
	dprintf (DRIVER_NAME ": uninit_driver()\n");
	for (ix = 0; ix < num_cards; ix++) {
		area = find_area (AREA_NAME);
		if (area >= 0)
			delete_area (area);
  		(*gameport->delete_device)(cards[ix].joy.driver);
	}
	memset(&cards, 0, sizeof(cards));
	put_module (gameport_name);
	put_module (pci_name);
	num_cards = 0;
}


extern "C" const char**
publish_devices()
{
	return (const char **)gDeviceNames;
}


static int
lookup_device_name (const char *name)
{
	int i;
	for (i = 0; gDeviceNames[i]; i++)
		if (!strcmp (gDeviceNames[i], name))
			return i;
	return -1;
}


static status_t
device_open(const char* name, uint32 flags, void** cookie)
{
	int ix;
	int offset = -1;

	*cookie = NULL;
	for (ix = 0; ix < num_cards; ix++) {
		if (!strcmp(name, cards[ix].joy.name1)) {
			offset = 0;
			break;
		}
	}
	
	if (offset < 0) {
		return ENODEV;
	}
			
   return (*gameport->open_hook)(cards[ix].joy.driver, flags, cookie);
}


static status_t
device_close(void * cookie)
{
	return (*gameport->close_hook)(cookie);
}


static status_t
device_free(void * cookie)
{
	return (*gameport->free_hook)(cookie);
}


static status_t
device_control(void * cookie, uint32 iop, void * data, size_t len)
{
	return (*gameport->control_hook)(cookie, iop, data, len);
}


static status_t
device_read(void * cookie, off_t pos, void * data, size_t * nread)
{	
	return (*gameport->read_hook)(cookie, pos, data, nread);
}


static status_t
device_write(void * cookie, off_t pos, const void * data, size_t * nwritten)
{
	(*pci->write_io_32) ((int)vaddr,0);	
	return (*gameport->write_hook)(cookie, pos, data, nwritten);
}

device_hooks gDeviceHooks = {
    device_open,
    device_close,
    device_free,
    device_control,
    device_read,
    device_write,
    NULL,		/* select */
    NULL,		/* deselect */
    NULL,		/* readv */
    NULL		/* writev */
};


device_hooks*
find_device(const char* name)
{
	if (lookup_device_name (name) >= 0) {
	     return &gDeviceHooks;
	}
	return NULL;
}
