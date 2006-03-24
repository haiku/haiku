/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "driver.h"
#include "device.h"
#include "lock.h"

#include <OS.h>
#include <KernelExport.h>
#include <SupportDefs.h>
#include <PCI.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#define TRACE_DRIVER
#ifdef TRACE_DRIVER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define MAX_CARDS 4

int32 api_version = B_CUR_DRIVER_API_VERSION;

char *gDeviceNames[MAX_CARDS + 1];
intel_info *gDeviceInfo[MAX_CARDS];
pci_info *gPCIInfo[MAX_CARDS];
pci_module_info *gPCI;
lock gLock;


extern "C" {
	status_t init_hardware(void);
	status_t init_driver(void);
	void uninit_driver(void);
	const char **publish_devices(void);
	device_hooks *find_device(const char *name);
}


static status_t
get_next_intel_extreme(int32 *_cookie, pci_info &info)
{
	int32 index = *_cookie;

	// find devices
	for (; gPCI->get_nth_pci_info(index, &info) == B_OK; index++) {
		if (info.vendor_id == VENDOR_ID_INTEL
			&& info.device_id == DEVICE_ID_i865) {
			*_cookie = index + 1;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


const char **
publish_devices(void)
{
	TRACE((DEVICE_NAME ": publish_devices()\n"));
	return (const char **)gDeviceNames;
}


status_t
init_hardware(void)
{
	status_t status;

	TRACE((DEVICE_NAME ": init_hardware()\n"));

	if ((status = get_module(B_PCI_MODULE_NAME,(module_info **)&gPCI)) != B_OK) {
		TRACE((DEVICE_NAME ": pci module unavailable\n"));
		return status;
	}

	int32 cookie = 0;
	pci_info info;
	status = get_next_intel_extreme(&cookie, info);
dprintf("found: %s\n", strerror(status));

	put_module(B_PCI_MODULE_NAME);
	return status;
}


status_t
init_driver(void)
{
	status_t status;
	TRACE((DEVICE_NAME ": init_driver()\n"));

	if ((status = get_module(B_PCI_MODULE_NAME,(module_info **)&gPCI)) != B_OK) {
		TRACE((DEVICE_NAME ": pci module unavailable\n"));
		return status;
	}

	// find devices

	int32 found = 0;

	for (int32 cookie = 0; found < MAX_CARDS;) {
		pci_info *info = (pci_info *)malloc(sizeof(pci_info));
		status = get_next_intel_extreme(&cookie, *info);
		if (status < B_OK) {
			free(info);
			break;
		}

		// enable bus mastering for device
//		pci->write_pci_config(info->bus, info->device, info->function,
//			PCI_command, 2,
//			PCI_command_master | pci->read_pci_config(
//				info->bus, info->device, info->function,
//				PCI_command, 2));

		gPCIInfo[found++] = info;
		dprintf(DEVICE_NAME ": (%ld) revision = 0x%x\n", found, info->revision);
	}

	if (found == 0) {
		put_module(B_PCI_MODULE_NAME);
		return ENODEV;
	}

	// create device name list & allocate device info structure
	{
		int32 index = 0;
		char name[64];

		for (; index < found; index++) {
			pci_info *info = gPCIInfo[index];
			sprintf(name, "graphics/%04X_%04X_%02X%02X%02X",
				 info->vendor_id, info->device_id,
				 info->bus, info->device,
				 info->function);
			gDeviceNames[index] = strdup(name);

			if ((gDeviceInfo[index] = (intel_info *)malloc(sizeof(intel_info))) != NULL)
				memset(gDeviceInfo[index], 0, sizeof(intel_info));
			else
				free(gDeviceNames[index]);
		}
		gDeviceNames[index] = NULL;
	}

	return init_lock(&gLock, "intel extreme ksync");
}


void
uninit_driver(void)
{
	TRACE((DEVICE_NAME ": uninit_driver()\n"));

	uninit_lock(&gLock);

	// free device related structures
	char *name;
	for (int32 index = 0; (name = gDeviceNames[index]) != NULL; index++) {
		free(gDeviceInfo[index]);
		free(gPCIInfo[index]);
		free(name);
	}

	put_module(B_PCI_MODULE_NAME);
}


device_hooks *
find_device(const char *name)
{
	int index;

	TRACE((DEVICE_NAME ": find_device()\n"));

	for (index = 0; gDeviceNames[index] != NULL; index++)
		if (!strcmp(name, gDeviceNames[index]))
			return &gDeviceHooks;

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
