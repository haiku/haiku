/* Realtek RTL8169 Family Driver
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

//#define DEBUG

#include "debug.h"
#include "device.h"
#include "driver.h"
#include "setup.h"
#include "timer.h"

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

	item = (pci_info *)malloc(sizeof(pci_info));
	if (!item)
		return B_NO_MEMORY;
	
	if (get_module(B_PCI_MODULE_NAME, (module_info **)&gPci) < B_OK) {
		free(item);
		return B_ERROR;
	}
	
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
				goto err_outofmem;
			if (cards == MAX_CARDS)
				break;
		}
	}
	
	TRACE("found %d cards\n", cards);

	free(item);
	
	if (!cards)
		goto err_cards;
	
	if (initialize_timer() != B_OK) {
		ERROR("timer init failed\n");
		goto err_timer;
	}
	
	return B_OK;
	
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

	TRACE("uninit_driver()\n");
	
	terminate_timer();

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
