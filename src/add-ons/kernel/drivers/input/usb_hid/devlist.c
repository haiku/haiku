/*
 * Copyright 2004-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *
 * Some portions of code are copyrighted by
 * USB Joystick driver for BeOS R5
 * Copyright 2000 (C) ITO, Takayuki. All rights reserved
 */


#include "hid.h"

#include <stdlib.h>
#include <string.h>


sem_id gDeviceListLock = -1;
bool gDeviceListChanged = true;	/* added or removed */
/* dynamically generated */
char **gDeviceNames = NULL;


static hid_device_info *sDeviceList = NULL;
static int sDeviceCount = 0;


void
add_device_info(hid_device_info *device)
{
	assert(device != NULL);

	acquire_sem(gDeviceListLock);
	device->next = sDeviceList;
	sDeviceList = device;
	sDeviceCount++;
	gDeviceListChanged = true;
	release_sem(gDeviceListLock);
}


void
remove_device_info(hid_device_info *device)
{
	assert(device != NULL);

	acquire_sem(gDeviceListLock);

	if (sDeviceList == device)
		sDeviceList = device->next;
	else {
		hid_device_info *previous;
		for (previous = sDeviceList; previous != NULL; previous = previous->next) {
			if (previous->next == device) {
				previous->next = device->next;
				--sDeviceCount;
				gDeviceListChanged = true;
				break;
			}
		}
		assert(previous != NULL);
	}
	release_sem(gDeviceListLock);
}


hid_device_info *
search_device_info(const char* name)
{
	hid_device_info *device;

	acquire_sem(gDeviceListLock);
	for (device = sDeviceList; device != NULL; device = device->next) {
		if (strcmp(device->name, name) == 0)
			break;
	}

	release_sem(gDeviceListLock);
	return device;
}


//	#pragma mark - device names


void
alloc_device_names(void)
{
	assert(gDeviceNames == NULL);
	gDeviceNames = malloc(sizeof(char *) * (sDeviceCount + 1));
}


void
free_device_names(void)
{
	if (gDeviceNames != NULL) {
		int i;
		for (i = 0; gDeviceNames [i] != NULL; i++) {
			free(gDeviceNames[i]);
		}

		free(gDeviceNames);
		gDeviceNames = NULL;
	}
}


void
rebuild_device_names(void)
{
	int i;
	hid_device_info *device;

	assert(gDeviceNames != NULL);
	acquire_sem(gDeviceListLock);
	for (i = 0, device = sDeviceList; device != NULL; device = device->next) {
		gDeviceNames[i++] = strdup(device->name);
		DPRINTF_INFO((MY_ID "publishing %s\n", device->name));
	}
	gDeviceNames[i] = NULL;
	release_sem(gDeviceListLock);
}

