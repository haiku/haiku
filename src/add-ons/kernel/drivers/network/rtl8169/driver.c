/* Realtek RTL8169 Family Driver
 * Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 */
#include <KernelExport.h>
#include <Errors.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//#define DEBUG

#include "debug.h"
#include "device.h"
#include "driver.h"
#include "setup.h"

int32 api_version = B_CUR_DRIVER_API_VERSION;

pci_module_info *gPci;

char* gDevNameList[MAX_CARDS + 1];
pci_info *gDevList[MAX_CARDS];

status_t
init_hardware(void)
{
	pci_module_info *pci;
	pci_info info;
	status_t res;
	int i;

	TRACE("init_hardware()\n");

	if (get_module(B_PCI_MODULE_NAME, (module_info **)&pci) < B_OK)
		return B_ERROR;
	for (res = B_ERROR, i = 0; pci->get_nth_pci_info(i, &info) == B_OK; i++) {
		if (info.vendor_id == 0x10ec && info.device_id == 0x8169) {
			res = B_OK;
			break;
		}
	}
	put_module(B_PCI_MODULE_NAME);

	return res;
}


status_t
init_driver(void)
{
	struct pci_info *item;
	int index;
	int cards;
	
#ifdef DEBUG	
	set_dprintf_enabled(true);
	load_driver_symbols("rtl8169");
#endif

	dprintf(INFO1"\n");
	dprintf(INFO2"\n");
	dprintf(INFO3"\n");

	item = (pci_info *)malloc(sizeof(pci_info));
	if (!item)
		return B_NO_MEMORY;
	
	if (get_module(B_PCI_MODULE_NAME, (module_info **)&gPci) < B_OK)
		return B_ERROR;
	
	for (cards = 0, index = 0; gPci->get_nth_pci_info(index++, item) == B_OK; ) {
		if (item->vendor_id == 0x10ec && item->device_id == 0x8169) {
			char name[64];
			sprintf(name, "net/rtl8169/%d", cards);
			gDevList[cards] = item;
			gDevNameList[cards] = strdup(name);
			gDevNameList[cards + 1] = NULL;
			cards++;
			item = (pci_info *)malloc(sizeof(pci_info));
			if (!item)
				return B_OK; // already found 1 card, but out of memory
			if (cards == MAX_CARDS)
				break;
		}
	}
	
	TRACE("found %d cards\n", cards);

	free(item);
	
	if (!cards) {
		put_module(B_PCI_MODULE_NAME);
		return B_ERROR;
	}

	return B_OK;
}


void
uninit_driver(void)
{
	int32 i;

	TRACE("uninit_driver()\n");
	
	for (i = 0; gDevNameList[i] != NULL; i++) {
		free(gDevList[i]);
		free(gDevNameList[i]);
	}
	
	put_module(B_PCI_MODULE_NAME);
}


device_hooks
gDeviceHooks = {
	rtl8169_open,
	rtl8169_close,
	rtl8169_free,
	rtl8169_control,
	rtl8169_read,
	rtl8169_write,
};


const char**
publish_devices()
{
	return (const char**)gDevNameList;
}


device_hooks*
find_device(const char* name)
{
	return &gDeviceHooks;
}
