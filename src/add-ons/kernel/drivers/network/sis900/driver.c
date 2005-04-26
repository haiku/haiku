/* Kernel driver for SiS 900 networking
 *
 * Copyright © 2001-2005 pinc Software. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */


#include <OS.h>
#include <KernelExport.h>
#include <SupportDefs.h>
#include <PCI.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "ether_driver.h"
#include "driver.h"
#include "device.h"
#include "sis900.h"

#define MAX_CARDS 4

int32 api_version = B_CUR_DRIVER_API_VERSION;

char *gDeviceNames[MAX_CARDS + 1];
pci_info *pciInfo[MAX_CARDS];
pci_module_info	*pci;

extern status_t init_hardware(void);
extern status_t init_driver(void);
extern void uninit_driver(void);
extern const char **publish_devices(void);
extern device_hooks *find_device(const char *name);


const char **
publish_devices(void)
{
	TRACE((DEVICE_NAME ": publish_devices()\n"));
	return (const char **)gDeviceNames;
}


status_t
init_hardware(void)
{
	TRACE((DEVICE_NAME ": init_hardware()\n"));
	return B_NO_ERROR;
}


status_t
init_driver(void)
{
	status_t status;
	pci_info *info;
	int i, found;

	TRACE((DEVICE_NAME ": init_driver()\n"));

	if ((status = get_module(B_PCI_MODULE_NAME,(module_info **)&pci)) != B_OK)
	{
		TRACE((DEVICE_NAME ": pci module unavailable\n"));
		return status;
	}

	// find devices
	info = malloc(sizeof(pci_info));
	for (i = found = 0; (status = pci->get_nth_pci_info(i, info)) == B_OK; i++) {
		if (info->vendor_id == VENDOR_ID_SiS && info->device_id == DEVICE_ID_SiS900) {
			// enable bus mastering for device
			pci->write_pci_config(info->bus, info->device, info->function,
				PCI_command, 2,
				PCI_command_master | pci->read_pci_config(
					info->bus, info->device, info->function,
					PCI_command, 2));

			pciInfo[found++] = info;
			dprintf(DEVICE_NAME ": revision = %x\n", info->revision);

			info = malloc(sizeof(pci_info));
		}
	}
	free(info);

	if (found == 0) {
		put_module(B_PCI_MODULE_NAME);
		return ENODEV;
	}

	// create device name list
	{
		char name[32];

		for (i = 0; i < found; i++) {
			sprintf(name, DEVICE_DRIVERNAME "/%d", i);
			gDeviceNames[i] = strdup(name);
		}
		gDeviceNames[i] = NULL;
	}
	return B_OK;
}


void
uninit_driver(void)
{
	void *item;
	int index;

	TRACE((DEVICE_NAME ": uninit_driver()\n"));

	// free device names & pci info
	for (index = 0; (item = gDeviceNames[index]) != NULL; index++) {
		free(item);
		free(pciInfo[index]);
	}

	put_module(B_PCI_MODULE_NAME);
}


device_hooks *
find_device(const char *name)
{
	int index;

	TRACE((DEVICE_NAME ": find_device()\n"));

	for (index = 0; gDeviceNames[index] != NULL; index++) {
		if (!strcmp(name, gDeviceNames[index]))
			return &gDeviceHooks;
	}

	return NULL;
}

/*
void
wake_driver(void)
{
	// for compatibility with Dano, only
}


void
suspend_driver(void)
{
	// for compatibility with Dano, only
}
*/
