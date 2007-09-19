/*
 * Copyright 2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Eric Petit <eric.petit@lapsus.org>
 */


#include "driver.h"

#include <KernelExport.h>
#include <PCI.h>
#include <OS.h>

#include <stdlib.h>
#include <string.h>


DeviceData *gPd;
pci_module_info *gPciBus;


int32 api_version = B_CUR_DRIVER_API_VERSION;


status_t
init_hardware(void)
{
	status_t ret = B_ERROR;
	pci_info pcii;
	int i;

	TRACE("init_hardware\n");

	if (get_module(B_PCI_MODULE_NAME, (module_info **)&gPciBus) != B_OK)
		return B_ERROR;

	for (i = 0; (*gPciBus->get_nth_pci_info)(i, &pcii) == B_OK; i++) {
		if (pcii.vendor_id == PCI_VENDOR_ID_VMWARE &&
			pcii.device_id == PCI_DEVICE_ID_VMWARE_SVGA2) {
			ret = B_OK;
			break;
		}
	}

	put_module(B_PCI_MODULE_NAME);

	TRACE("init_hardware: %ld\n", ret);
	return ret;
}


status_t
init_driver(void)
{
	status_t ret = ENODEV;
	int i;

	TRACE("init_driver\n");

	if (get_module(B_PCI_MODULE_NAME, (module_info **)&gPciBus) != B_OK) {
		ret = B_ERROR;
		goto done;
	}

	if (!(gPd = calloc(1, sizeof(DeviceData)))) {
		put_module(B_PCI_MODULE_NAME);
		ret = B_ERROR;
		goto done;
	}

	/* Remember the PCI information */
	for (i = 0; (*gPciBus->get_nth_pci_info)(i, &gPd->pcii) == B_OK; i++) 
		if (gPd->pcii.vendor_id == PCI_VENDOR_ID_VMWARE &&
			gPd->pcii.device_id == PCI_DEVICE_ID_VMWARE_SVGA2) {
			ret = B_OK;
			break;
		}

	if (ret != B_OK) {
		free(gPd);
		put_module(B_PCI_MODULE_NAME);
		goto done;
	}

	/* Create a benaphore for exclusive access in OpenHook/FreeHook */
	INIT_BEN(gPd->kernel);

	/* The device name */
	gPd->names[0] = strdup("graphics/vmware");
	gPd->names[1] = NULL;

	/* Usual initializations */
	gPd->isOpen = 0;
	gPd->sharedArea = -1;
	gPd->si = NULL;

done:
	TRACE("init_driver: %ld\n", ret);
	return ret;
}


const char **
publish_devices(void)
{
	TRACE("publish_devices\n");
	return (const char **)gPd->names;
}


device_hooks *
find_device(const char *name)
{
	TRACE("find_device (%s)\n", name);
	if (gPd->names[0] && !strcmp(gPd->names[0], name))
		return &gGraphicsDeviceHooks;
	return NULL;
}


void
uninit_driver()
{
	TRACE("uninit_driver\n");
	DELETE_BEN(gPd->kernel);
	free(gPd->names[0]);
	free(gPd);
	put_module(B_PCI_MODULE_NAME);
}

