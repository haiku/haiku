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


#include "driver.h"

#include <stdlib.h>
#include <string.h>


sem_id gDeviceListLock = -1;
bool gDeviceListChanged = true;	/* added or removed */
/* dynamically generated */
char **gDeviceNames = NULL;


static pegasus_dev *sDeviceList = NULL;
static int sDeviceCount = 0;


void
add_device_info(pegasus_dev *device)
{
	ASSERT(device != NULL);

	acquire_sem(gDeviceListLock);
	device->next = sDeviceList;
	sDeviceList = device;
	sDeviceCount++;
	gDeviceListChanged = true;
	release_sem(gDeviceListLock);
}


void
remove_device_info(pegasus_dev *device)
{
	ASSERT(device != NULL);

	acquire_sem(gDeviceListLock);

	if (sDeviceList == device) {
		sDeviceList = device->next;
		--sDeviceCount;
		gDeviceListChanged = true;
	} else {
		pegasus_dev *previous;
		for (previous = sDeviceList; previous != NULL; previous = previous->next) {
			if (previous->next == device) {
				previous->next = device->next;
				--sDeviceCount;
				gDeviceListChanged = true;
				break;
			}
		}
		ASSERT(previous != NULL);
	}
	release_sem(gDeviceListLock);
}


pegasus_dev *
search_device_info(const char* name)
{
	pegasus_dev *device;

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
	ASSERT(gDeviceNames == NULL);
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
	pegasus_dev *device;

	ASSERT(gDeviceNames != NULL);
	acquire_sem(gDeviceListLock);
	for (i = 0, device = sDeviceList; device != NULL; device = device->next) {
		gDeviceNames[i++] = strdup(device->name);
		DPRINTF_INFO(MY_ID "publishing %s\n", device->name);
	}
	gDeviceNames[i] = NULL;
	release_sem(gDeviceListLock);
}

