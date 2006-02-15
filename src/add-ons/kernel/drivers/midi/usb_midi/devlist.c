/*****************************************************************************/
// midi usb driver
// Written by Jérôme Duval
//
// devlist.c
//
// Copyright (c) 2006 Haiku Project
//
// 	Some portions of code are copyrighted by
//	USB Joystick driver for BeOS R5
//	Copyright 2000 (C) ITO, Takayuki
//	All rights reserved
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "usb_midi.h"

sem_id			my_device_list_lock = -1;
bool			my_device_list_changed = true;	/* added or removed */

static my_device_info	*my_device_list = NULL;
static int				my_device_count = 0;

void add_device_info (my_device_info *my_dev)
{
	assert (my_dev != NULL);
	acquire_sem (my_device_list_lock);
	my_dev->next = my_device_list;
	my_device_list = my_dev;
	my_device_count++;
	my_device_list_changed = true;
	release_sem (my_device_list_lock);
}

void remove_device_info (my_device_info *my_dev)
{
	assert (my_dev != NULL);
	acquire_sem (my_device_list_lock);
	if (my_device_list == my_dev)
		my_device_list = my_dev->next;
	else
	{
		my_device_info *d;
		for (d = my_device_list; d != NULL; d = d->next)
		{
			if (d->next == my_dev)
			{
				d->next = my_dev->next;
				--my_device_count;
				my_device_list_changed = true;
				break;
			}
		}
		assert (d != NULL);
	}
	release_sem (my_device_list_lock);
}

my_device_info *search_device_info (const char* name)
{
	my_device_info *my_dev;

	acquire_sem (my_device_list_lock);
	for (my_dev = my_device_list; my_dev != NULL; my_dev = my_dev->next)
	{
		if (strcmp(my_dev->name, name) == 0)
			break;
	}
	release_sem (my_device_list_lock);
	return my_dev;
}

/*
	device names
*/

/* dynamically generated */
char **my_device_names = NULL;

void alloc_device_names (void)
{
	assert (my_device_names == NULL);
	my_device_names = malloc (sizeof (char *) * (my_device_count + 1));
}

void free_device_names (void)
{
	if (my_device_names != NULL)
	{
		int i;
		for (i = 0; my_device_names [i] != NULL; i++)
			free (my_device_names [i]);
		free (my_device_names);
		my_device_names = NULL;
	}
}

void rebuild_device_names (void)
{
	int i;
	my_device_info *my_dev;

	assert (my_device_names != NULL);
	acquire_sem (my_device_list_lock);
	for (i = 0, my_dev = my_device_list; my_dev != NULL; my_dev = my_dev->next)
	{
		my_device_names [i++] = strdup(my_dev->name);
		DPRINTF_INFO ((MY_ID "publishing %s\n", my_dev->name));
	}
	my_device_names [i] = NULL;
	release_sem (my_device_list_lock);
}

