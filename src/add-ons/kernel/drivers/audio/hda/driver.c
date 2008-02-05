/*
 * Copyright 2007-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar Adema, ithamar AT unet DOT nl
 */


#include "driver.h"


int32 api_version = B_CUR_DRIVER_API_VERSION;

hda_controller gCards[MAXCARDS];
uint32 gNumCards;
pci_module_info* gPci;


status_t
init_hardware(void)
{
	pci_info info;
	long i;

	if (get_module(B_PCI_MODULE_NAME, (module_info**)&gPci) != B_OK)
		return ENODEV;

	for (i = 0; gPci->get_nth_pci_info(i, &info) == B_OK; i++) {
		if (info.class_base == PCI_multimedia
			&& info.class_sub == PCI_hd_audio) {
			put_module(B_PCI_MODULE_NAME);
			return B_OK;
		}
	}

	put_module(B_PCI_MODULE_NAME);
	return ENODEV;
}


status_t
init_driver(void)
{
	char path[B_PATH_NAME_LENGTH];
	pci_info info;
	long i;

	if (get_module(B_PCI_MODULE_NAME, (module_info**)&gPci) != B_OK)
		return ENODEV;

	gNumCards = 0;

	for (i = 0; gPci->get_nth_pci_info(i, &info) == B_OK; i++) {
		if (info.class_base == PCI_multimedia
			&& info.class_sub == PCI_hd_audio) {
			gCards[gNumCards].pci_info = info;
			gCards[gNumCards].opened = 0;
			sprintf(path, DEVFS_PATH_FORMAT, gNumCards);
			gCards[gNumCards++].devfs_path = strdup(path);

			dprintf("HDA: Detected controller @ PCI:%d:%d:%d, IRQ:%d, type %04x/%04x\n",
				info.bus, info.device, info.function,
				info.u.h0.interrupt_line,
				info.vendor_id, info.device_id);
		}
	}

	if (gNumCards == 0) {
		put_module(B_PCI_MODULE_NAME);
		return ENODEV;
	}

	return B_OK;
}


void
uninit_driver(void)
{
	long i;
	dprintf("IRA: %s\n", __func__);

	for (i = 0; i < gNumCards; i++) {
		free((void*)gCards[i].devfs_path);
		gCards[i].devfs_path = NULL;
	}
	
	put_module(B_PCI_MODULE_NAME);
}


const char**
publish_devices(void)
{
	static const char* devs[MAXCARDS+1];
	long i;

	dprintf("IRA: %s\n", __func__);
	for (i = 0; i < gNumCards; i++)
		devs[i] = gCards[i].devfs_path;

	devs[i] = NULL;

	return devs;
}


device_hooks*
find_device(const char* name)
{
	dprintf("IRA: %s\n", __func__);
	return &gDriverHooks;
}
