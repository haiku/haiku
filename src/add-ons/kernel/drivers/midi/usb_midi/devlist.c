/*
 * midi usb driver
 * devlist.c
 *
 * Copyright 2006-2009 Haiku Inc.  All rights reserved.
 * Distributed under the terms of the MIT Licence.
 *
 * Authors:
 *		Jérôme Duval
 *		Pete Goodeve, pete.goodeve@computer.org
 *
 *		Some portions of this code were originally derived from
 *		USB Joystick driver for BeOS R5
 *		Copyright 2000 (C) ITO, Takayuki
 *		All rights reserved
 *
 */


#include "usb_midi.h"

#include <stdlib.h>
#include <string.h>


sem_id			usbmidi_device_list_lock = -1;
bool			usbmidi_device_list_changed = true;	/* added or removed */

static usbmidi_device_info*	usbmidi_device_list = NULL;
static int		usbmidi_device_count = 0;


void
add_device_info(usbmidi_device_info* my_dev)
{
	assert(my_dev != NULL);
	acquire_sem(usbmidi_device_list_lock);
	my_dev->next = usbmidi_device_list;
	usbmidi_device_list = my_dev;
	usbmidi_device_count++;
	usbmidi_device_list_changed = true;
	release_sem(usbmidi_device_list_lock);
}


void
remove_device_info(usbmidi_device_info* my_dev)
{
	assert(my_dev != NULL);
	acquire_sem(usbmidi_device_list_lock);
	if (usbmidi_device_list == my_dev) {
		usbmidi_device_list = my_dev->next;
		--usbmidi_device_count;
		usbmidi_device_list_changed = true;		
	} else {
		usbmidi_device_info* d;
		for (d = usbmidi_device_list; d != NULL; d = d->next) {
			if (d->next == my_dev) {
				d->next = my_dev->next;
				--usbmidi_device_count;
				usbmidi_device_list_changed = true;
				break;
			}
		}
		assert(d != NULL);
	}
	release_sem(usbmidi_device_list_lock);
}


usbmidi_device_info*
search_device_info(const char* name)
{
	usbmidi_device_info* my_dev;

	acquire_sem(usbmidi_device_list_lock);
	for (my_dev = usbmidi_device_list; my_dev != NULL; my_dev = my_dev->next) {
		if (strcmp(my_dev->name, name) == 0)
			break;
	}
	release_sem(usbmidi_device_list_lock);
	return my_dev;
}


/*
	device names
*/

/* dynamically generated */
char** usbmidi_device_names = NULL;


void
alloc_device_names(void)
{
	assert(usbmidi_device_names == NULL);
	usbmidi_device_names = malloc(sizeof(char*) * (usbmidi_device_count + 1));
}


void
free_device_names(void)
{
	if (usbmidi_device_names != NULL) {
		int i;
		for (i = 0; usbmidi_device_names[i] != NULL; i++)
			free(usbmidi_device_names[i]);
		free(usbmidi_device_names);
		usbmidi_device_names = NULL;
	}
}


void
rebuild_device_names(void)
{
	int i;
	usbmidi_device_info* my_dev;

	assert(usbmidi_device_names != NULL);
	acquire_sem(usbmidi_device_list_lock);
	for (i = 0, my_dev = usbmidi_device_list; my_dev != NULL; my_dev = my_dev->next) {
		usbmidi_device_names[i++] = strdup(my_dev->name);
		DPRINTF_INFO((MY_ID "publishing %s\n", my_dev->name));
	}
	usbmidi_device_names[i] = NULL;
	release_sem(usbmidi_device_list_lock);
}
