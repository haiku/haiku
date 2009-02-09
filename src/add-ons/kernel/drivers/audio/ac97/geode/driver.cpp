/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *	Jérôme Duval (korli@users.berlios.de)
 */


#include "driver.h"


int32 api_version = B_CUR_DRIVER_API_VERSION;

geode_controller gCards[MAX_CARDS];
uint32 gNumCards;
pci_module_info* gPci;


extern "C" status_t
init_hardware(void)
{
	pci_info info;
	long i;

	if (get_module(B_PCI_MODULE_NAME, (module_info**)&gPci) != B_OK)
		return ENODEV;

	for (i = 0; gPci->get_nth_pci_info(i, &info) == B_OK; i++) {
		if ((info.vendor_id == AMD_VENDOR_ID
			&& info.device_id == AMD_CS5536_AUDIO_DEVICE_ID)
			|| (info.vendor_id == NS_VENDOR_ID
				&& info.device_id == NS_CS5535_AUDIO_DEVICE_ID)) {
			put_module(B_PCI_MODULE_NAME);
			return B_OK;
		}
	}

	put_module(B_PCI_MODULE_NAME);
	return ENODEV;
}


extern "C" status_t
init_driver(void)
{
	char path[B_PATH_NAME_LENGTH];
	pci_info info;
	long i;

	if (get_module(B_PCI_MODULE_NAME, (module_info**)&gPci) != B_OK)
		return ENODEV;

	gNumCards = 0;

	for (i = 0; gPci->get_nth_pci_info(i, &info) == B_OK
			&& gNumCards < MAX_CARDS; i++) {
		if ((info.vendor_id == AMD_VENDOR_ID
			&& info.device_id == AMD_CS5536_AUDIO_DEVICE_ID)
			|| (info.vendor_id == NS_VENDOR_ID
				&& info.device_id == NS_CS5535_AUDIO_DEVICE_ID)) {
			memset(&gCards[gNumCards], 0, sizeof(geode_controller));
			gCards[gNumCards].pci_info = info;
			gCards[gNumCards].opened = 0;
			sprintf(path, DEVFS_PATH_FORMAT, gNumCards);
			gCards[gNumCards++].devfs_path = strdup(path);

			dprintf("geode: detected controller @ PCI:%d:%d:%d, IRQ:%d, type %04x/%04x\n",
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


extern "C" void
uninit_driver(void)
{
	for (uint32 i = 0; i < gNumCards; i++) {
		free((void*)gCards[i].devfs_path);
		gCards[i].devfs_path = NULL;
	}
	
	put_module(B_PCI_MODULE_NAME);
}


extern "C" const char**
publish_devices(void)
{
	static const char* devs[MAX_CARDS + 1];
	uint32 i;

	for (i = 0; i < gNumCards; i++) {
		devs[i] = gCards[i].devfs_path;
	}

	devs[i] = NULL;

	return devs;
}


extern "C" device_hooks*
find_device(const char* name)
{
	return &gDriverHooks;
}
