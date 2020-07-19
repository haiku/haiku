/*
 * Copyright 2007-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar Adema, ithamar AT unet DOT nl
 */


#include "driver.h"


int32 api_version = B_CUR_DRIVER_API_VERSION;

hda_controller gCards[MAX_CARDS];
uint32 gNumCards;
pci_module_info* gPci;
pci_x86_module_info* gPCIx86Module;


extern "C" status_t
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
		if (info.class_base == PCI_multimedia
			&& info.class_sub == PCI_hd_audio) {
#ifdef __HAIKU__
			if ((*gPci->reserve_device)(info.bus, info.device, info.function,
				"hda", &gCards[gNumCards]) < B_OK) {
				dprintf("HDA: Failed to reserve PCI:%d:%d:%d\n",
					info.bus, info.device, info.function);
				continue;
			}
#endif
			memset(&gCards[gNumCards], 0, sizeof(hda_controller));
			gCards[gNumCards].pci_info = info;
			gCards[gNumCards].opened = 0;
			sprintf(path, DEVFS_PATH_FORMAT, gNumCards);
			gCards[gNumCards++].devfs_path = strdup(path);

			dprintf("HDA: Detected controller @ PCI:%d:%d:%d, IRQ:%d, "
				"type %04x/%04x (%04x/%04x)\n",
				info.bus, info.device, info.function,
				info.u.h0.interrupt_line, info.vendor_id, info.device_id,
				info.u.h0.subsystem_vendor_id, info.u.h0.subsystem_id);
		}
	}

	if (gNumCards == 0) {
		put_module(B_PCI_MODULE_NAME);
		return ENODEV;
	}

	if (get_module(B_PCI_X86_MODULE_NAME, (module_info**)&gPCIx86Module)
			!= B_OK) {
		gPCIx86Module = NULL;
	}

	return B_OK;
}


extern "C" void
uninit_driver(void)
{
	for (uint32 i = 0; i < gNumCards; i++) {
#ifdef __HAIKU__
		(*gPci->unreserve_device)(gCards[i].pci_info.bus,
			gCards[i].pci_info.device, gCards[i].pci_info.function, "hda",
			&gCards[i]);
#endif
		free((void*)gCards[i].devfs_path);
		gCards[i].devfs_path = NULL;
	}

	put_module(B_PCI_MODULE_NAME);
	if (gPCIx86Module != NULL) {
		put_module(B_PCI_X86_MODULE_NAME);
		gPCIx86Module = NULL;
	}
}


extern "C" const char**
publish_devices(void)
{
	static const char* devs[MAX_CARDS + 1];
	uint32 i;

	for (i = 0; i < gNumCards; i++)
		devs[i] = gCards[i].devfs_path;

	devs[i] = NULL;

	return devs;
}


extern "C" device_hooks*
find_device(const char* name)
{
	return &gDriverHooks;
}
