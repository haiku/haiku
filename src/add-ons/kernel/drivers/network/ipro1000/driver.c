/* Intel PRO/1000 Family Driver
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
#include "mempool.h"

int32 api_version = B_CUR_DRIVER_API_VERSION;

pci_module_info *gPci;

char* gDevNameList[MAX_CARDS + 1];
pci_info *gDevList[MAX_CARDS];

const char *
identify_device(const pci_info *info)
{
	if (info->vendor_id != 0x8086)
		return 0;
	switch (info->device_id) {
		case 0x1000: return "82542";
		case 0x1001: return "82543GC FIBER";
		case 0x1004: return "82543GC COPPER";
		case 0x1008: return "82544EI COPPER";
		case 0x1009: return "82544EI FIBER";
		case 0x100C: return "82544GC COPPER";
		case 0x100D: return "82544GC LOM";
		case 0x100E: return "82540EM";
		case 0x100F: return "82545EM COPPER";
		case 0x1010: return "82546EB COPPER";
		case 0x1011: return "82545EM FIBER";
		case 0x1012: return "82546EB FIBER";
		case 0x1013: return "82541EI";
		case 0x1014: return "unknown 1014";
		case 0x1015: return "82540EM LOM";
		case 0x1016: return "82540EP LOM";
		case 0x1017: return "82540EP";
		case 0x1018: return "82541EI MOBILE";
		case 0x1019: return "82547EI";
		case 0x101A: return "unknown 101A";
		case 0x101D: return "82546EB QUAD COPPER";
		case 0x101E: return "82540EP LP";
		case 0x1026: return "82545GM COPPER";
		case 0x1027: return "82545GM FIBER";
		case 0x1028: return "82545GM SERDES";
		case 0x1075: return "82547GI";
		case 0x1076: return "82541GI";
		case 0x1077: return "82541GI MOBILE";
		case 0x1078: return "82541ER";
		case 0x1079: return "82546GB COPPER";
		case 0x107A: return "82546GB FIBER";
		case 0x107B: return "82546GB SERDES";
		default: return 0;
	}
}

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
		if (identify_device(&info)) {
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
	load_driver_symbols("ipro1000");
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
		const char *info = identify_device(item);
		if (info) {
			char name[64];
			sprintf(name, "net/ipro1000/%d", cards);
			TRACE("/dev/%s is a %s\n", name, info);
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

	if (!cards)
		goto err;

	if (mempool_init(cards * 768) != 0) {
		TRACE("mempool init failed\n");
		goto err;
	}
	
	return B_OK;

err:
	put_module(B_PCI_MODULE_NAME);
	return B_ERROR;
}


void
uninit_driver(void)
{
	int32 i;

	TRACE("uninit_driver()\n");
	
	mempool_exit();
	
	for (i = 0; gDevNameList[i] != NULL; i++) {
		free(gDevList[i]);
		free(gDevNameList[i]);
	}
	
	put_module(B_PCI_MODULE_NAME);
}


device_hooks
gDeviceHooks = {
	ipro1000_open,
	ipro1000_close,
	ipro1000_free,
	ipro1000_control,
	ipro1000_read,
	ipro1000_write,
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
