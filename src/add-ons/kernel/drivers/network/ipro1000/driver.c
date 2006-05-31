/* Intel PRO/1000 Family Driver
 * Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies, and that both the
 * copyright notice and this permission notice appear in supporting documentation.
 *
 * Marcus Overhagen makes no representations about the suitability of this software
 * for any purpose. It is provided "as is" without express or implied warranty.
 *
 * MARCUS OVERHAGEN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL MARCUS
 * OVERHAGEN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <KernelExport.h>
#include <Errors.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "timer.h"
#include "device.h"
#include "driver.h"
#include "mempool.h"

int32 api_version = B_CUR_DRIVER_API_VERSION;

pci_module_info *gPci;

char* gDevNameList[MAX_CARDS + 1];
pci_info *gDevList[MAX_CARDS];

static const char *
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
		case 0x107C: return "82541PI";
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

	INIT_DEBUGOUT("init_hardware()");

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
	
	dprintf("ipro1000: " INFO "\n");
	
	item = (pci_info *)malloc(sizeof(pci_info));
	if (!item)
		return B_NO_MEMORY;
	
	if (get_module(B_PCI_MODULE_NAME, (module_info **)&gPci) < B_OK) {
		free(item);
		return B_ERROR;
	}
	
	for (cards = 0, index = 0; gPci->get_nth_pci_info(index++, item) == B_OK; ) {
		const char *info = identify_device(item);
		if (info) {
			char name[64];
			sprintf(name, "net/ipro1000/%d", cards);
			dprintf("ipro1000: /dev/%s is a %s\n", name, info);
			gDevList[cards] = item;
			gDevNameList[cards] = strdup(name);
			gDevNameList[cards + 1] = NULL;
			cards++;
			item = (pci_info *)malloc(sizeof(pci_info));
			if (!item)
				goto err_outofmem;
			if (cards == MAX_CARDS)
				break;
		}
	}

	free(item);

	if (!cards)
		goto err_cards;
		
	if (initialize_timer() != B_OK) {
		ERROROUT("timer init failed");
		goto err_timer;
	}

	if (mempool_init(cards * 768) != B_OK) {
		ERROROUT("mempool init failed");
		goto err_mempool;
	}
	
	return B_OK;

err_mempool:
	terminate_timer();
err_timer:
err_cards:
err_outofmem:
	for (index = 0; index < cards; index++) {
		free(gDevList[index]);
		free(gDevNameList[index]);
	}
	put_module(B_PCI_MODULE_NAME);
	return B_ERROR;
}


void
uninit_driver(void)
{
	int32 i;

	INIT_DEBUGOUT("uninit_driver()");

	terminate_timer();
	
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
