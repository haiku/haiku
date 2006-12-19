/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>
#include <KernelExport.h>
#include <SupportDefs.h>
#include <PCI.h>
#include <frame_buffer_console.h>
#include <boot_item.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "driver.h"
#include "device.h"

#define TRACE_DRIVER
#ifdef TRACE_DRIVER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define MAX_CARDS 1

int32 api_version = B_CUR_DRIVER_API_VERSION;

char *gDeviceNames[MAX_CARDS + 1];
vesa_info *gDeviceInfo[MAX_CARDS];
isa_module_info *gISA;
lock gLock;


extern "C" {
	status_t init_hardware(void);
	status_t init_driver(void);
	void uninit_driver(void);
	const char **publish_devices(void);
	device_hooks *find_device(const char *name);
}

#if 0
static status_t
get_next_graphics_card(int32 *_cookie, pci_info &info)
{
	int32 index = *_cookie;

	// find devices
	for (; gPCI->get_nth_pci_info(index, &info) == B_OK; index++) {
		if (info.class_base == PCI_display) {
			*_cookie = index + 1;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}
#endif

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

	return get_boot_item(FRAME_BUFFER_BOOT_INFO) != NULL ? B_OK : B_ERROR;
}


status_t
init_driver(void)
{
	TRACE((DEVICE_NAME ": init_driver()\n"));

	if ((gDeviceInfo[0] = (vesa_info *)malloc(sizeof(vesa_info))) != NULL)
		memset(gDeviceInfo[0], 0, sizeof(vesa_info));
	else
		return B_NO_MEMORY;

	status_t status = get_module(B_ISA_MODULE_NAME, (module_info **)&gISA);
	if (status < B_OK)
		goto err1;

	gDeviceNames[0] = strdup("graphics/vesa");
	gDeviceNames[1] = NULL;

	status = init_lock(&gLock, "vesa lock");
	if (status == B_OK)
		return B_OK;

	put_module(B_ISA_MODULE_NAME);
err1:
	free(gDeviceInfo[0]);
	return status;
}


void
uninit_driver(void)
{
	TRACE((DEVICE_NAME ": uninit_driver()\n"));

	put_module(B_ISA_MODULE_NAME);
	uninit_lock(&gLock);

	// free device related structures
	char *name;
	for (int32 index = 0; (name = gDeviceNames[index]) != NULL; index++) {
		free(gDeviceInfo[index]);
		free(name);
	}
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

