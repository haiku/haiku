/*
 * midi usb driver
 * devlist.c
 *
 * Copyright 2006-2011 Haiku Inc.  All rights reserved.
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


sem_id			usbmidi_port_list_lock = -1;
bool			usbmidi_port_list_changed = true;	/* added or removed */

static usbmidi_port_info*	usbmidi_port_list = NULL;
static int		usbmidi_port_count = 0;


void
add_port_info(usbmidi_port_info* port)
{
	assert(port != NULL);
	acquire_sem(usbmidi_port_list_lock);
	port->next = usbmidi_port_list;
	usbmidi_port_list = port;
	usbmidi_port_count++;
	usbmidi_port_list_changed = true;
	release_sem(usbmidi_port_list_lock);
}


void
remove_port_info(usbmidi_port_info* port)
{
	assert(port != NULL);
	acquire_sem(usbmidi_port_list_lock);
	if (usbmidi_port_list == port) {
		usbmidi_port_list = port->next;
		--usbmidi_port_count;
		usbmidi_port_list_changed = true;
	} else {
		usbmidi_port_info* d;
		for (d = usbmidi_port_list; d != NULL; d = d->next) {
			if (d->next == port) {
				d->next = port->next;
				--usbmidi_port_count;
				usbmidi_port_list_changed = true;
				break;
			}
		}
		assert(d != NULL);
	}
	release_sem(usbmidi_port_list_lock);
}


usbmidi_port_info*
search_port_info(const char* name)
{
	usbmidi_port_info* port;

	acquire_sem(usbmidi_port_list_lock);
	for (port = usbmidi_port_list; port != NULL; port = port->next) {
		if (strcmp(port->name, name) == 0)
			break;
	}
	release_sem(usbmidi_port_list_lock);
	return port;
}


int
find_free_device_number(void)
{
	usbmidi_port_info* port;
	int number = 0;

	acquire_sem(usbmidi_port_list_lock);
	do {
		for (port = usbmidi_port_list; port != NULL; port = port->next) {
			if (port->device->devnum == number) {
				number++;
				break;	/* try next higher */
			}
		}
	} while (port);
	release_sem(usbmidi_port_list_lock);
	return number;
}



/*
	device names
*/

/* dynamically generated */
char** usbmidi_port_names = NULL;


void
alloc_port_names(void)
{
	assert(usbmidi_port_names == NULL);
	usbmidi_port_names = (char**)malloc(sizeof(char*)
		* (usbmidi_port_count + 1));
}


void
free_port_names(void)
{
	if (usbmidi_port_names != NULL) {
		int i;
		for (i = 0; usbmidi_port_names[i] != NULL; i++)
			free(usbmidi_port_names[i]);
		free(usbmidi_port_names);
		usbmidi_port_names = NULL;
	}
}


void
rebuild_port_names(void)
{
	int i;
	usbmidi_port_info* port;

	assert(usbmidi_port_names != NULL);
	acquire_sem(usbmidi_port_list_lock);
	for (i = 0, port = usbmidi_port_list; port != NULL; port = port->next) {
		usbmidi_port_names[i++] = strdup(port->name);
		DPRINTF_INFO((MY_ID "publishing %s\n", port->name));
	}
	usbmidi_port_names[i] = NULL;
	release_sem(usbmidi_port_list_lock);
}

