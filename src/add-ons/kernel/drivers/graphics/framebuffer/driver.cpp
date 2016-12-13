/*
 * Copyright 2005-2009, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2016, Jessica Hamilton, jessica.l.hamilton@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>
#include <KernelExport.h>
#include <SupportDefs.h>
#include <PCI.h>
#include <frame_buffer_console.h>
#include <boot_item.h>
#include <vesa_info.h>

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

char* gDeviceNames[MAX_CARDS + 1];
framebuffer_info* gDeviceInfo[MAX_CARDS];
mutex gLock;


extern "C" const char**
publish_devices(void)
{
	TRACE((DEVICE_NAME ": publish_devices()\n"));
	return (const char**)gDeviceNames;
}


extern "C" status_t
init_hardware(void)
{
	TRACE((DEVICE_NAME ": init_hardware()\n"));

	// If we don't have a VESA mode list, then we are a dumb
	// framebuffer, e.g. when there are no drivers available
	// on a UEFI system.
	return (get_boot_item(VESA_MODES_BOOT_INFO, NULL) == NULL
			&& get_boot_item(FRAME_BUFFER_BOOT_INFO, NULL) != NULL)
		? B_OK : B_ERROR;
}


extern "C" status_t
init_driver(void)
{
	TRACE((DEVICE_NAME ": init_driver()\n"));

	gDeviceInfo[0] = (framebuffer_info*)malloc(sizeof(framebuffer_info));
	if (gDeviceInfo[0] == NULL)
		return B_NO_MEMORY;

	memset(gDeviceInfo[0], 0, sizeof(framebuffer_info));

	gDeviceNames[0] = strdup("graphics/framebuffer");
	if (gDeviceNames[0] == NULL) {
		free(gDeviceNames[0]);
		return B_NO_MEMORY;
	}

	gDeviceNames[1] = NULL;

	mutex_init(&gLock, "framebuffer lock");
	return B_OK;
}


extern "C" void
uninit_driver(void)
{
	TRACE((DEVICE_NAME ": uninit_driver()\n"));

	mutex_destroy(&gLock);

	// free device related structures
	char* name;
	for (int32 index = 0; (name = gDeviceNames[index]) != NULL; index++) {
		free(gDeviceInfo[index]);
		free(name);
	}
}


extern "C" device_hooks*
find_device(const char* name)
{
	int index;

	TRACE((DEVICE_NAME ": find_device()\n"));

	for (index = 0; gDeviceNames[index] != NULL; index++) {
		if (!strcmp(name, gDeviceNames[index]))
			return &gDeviceHooks;
	}

	return NULL;
}

