/* Copyright (c) 2003-2011
 * Stefano Ceccherini <stefano.ceccherini@gmail.com>. All rights reserved.
 */
#include "debug.h"
#include <Debug.h>

#include <KernelExport.h>
#include <Errors.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "wb840.h"
#include "device.h"
#include "driver.h"

#define MAX_CARDS 4

int32 api_version = B_CUR_DRIVER_API_VERSION;

pci_module_info* gPci;
char* gDevNameList[MAX_CARDS + 1];
pci_info* gDevList[MAX_CARDS];


static bool
probe(pci_info* item)
{
	if ((item->vendor_id == WB_VENDORID && item->device_id == WB_DEVICEID_840F)
			|| (item->vendor_id == CP_VENDORID && item->device_id == CP_DEVICEID_RL100))	
		return true;
	return false;	
}


status_t
init_hardware(void)
{
	LOG((DEVICE_NAME ": init_hardware\n"));
	return B_OK;
}


status_t
init_driver(void)
{
	struct pci_info* item = NULL;
	int index = 0;
	int card_found = 0;
	char devName[64];
	status_t status;

	LOG((DEVICE_NAME ": init_driver\n"));
	
#ifdef DEBUG	
	set_dprintf_enabled(true);
#endif
	
	status = get_module(B_PCI_MODULE_NAME, (module_info**)&gPci);
	if (status < B_OK)
		return status;
	
	item = (pci_info*)malloc(sizeof(pci_info));
	if (item == NULL) {
		put_module(B_PCI_MODULE_NAME);
		return B_NO_MEMORY;
	}
	
	while (gPci->get_nth_pci_info(index, item) == B_OK) {
		if (probe(item)) {			
			gPci->write_pci_config(item->bus, item->device, item->function,
				PCI_command, 2, PCI_command_master | gPci->read_pci_config(
					item->bus, item->device, item->function,
					PCI_command, 2));
			gDevList[card_found++] = item;
			
			dprintf(DEVICE_NAME ": revision = %x\n", item->revision);

			item = (pci_info *)malloc(sizeof(pci_info));
		}
		index++;
	}		
	free(item);

	gDevList[card_found] = NULL;
		
	if (card_found == 0) {
		put_module(B_PCI_MODULE_NAME);		
		return ENODEV;
	}
	
	for (index = 0; index < card_found; index++) {
		sprintf(devName, DEVICE_NAME "/%d", index);
		LOG((DEVICE_NAME ":enabled %s\n", devName));
		gDevNameList[index] = strdup(devName);
	}
	
	gDevNameList[index] = NULL;
		
	return B_OK;
}


void
uninit_driver(void)
{
	int32 i = 0;
	
	LOG((DEVICE_NAME ": uninit_driver()\n"));	
	while(gDevNameList[i] != NULL) {
		free(gDevList[i]);
		free(gDevNameList[i]);
		i++;
	}
	
	put_module(B_PCI_MODULE_NAME);
}


const char**
publish_devices()
{
	return (const char**)gDevNameList;
}


device_hooks*
find_device(const char* name)
{
	int32 i;
	char* item;

	LOG((DEVICE_NAME ": find_device()\n"));
	// Find device name
	for (i = 0; (item = gDevNameList[i]); i++) {
		if (!strcmp(name, item)) {
			return &gDeviceHooks;
		}
	}
	return NULL; // Device not found
}
