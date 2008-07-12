/*
	USB joystick driver for BeOS R5
	Copyright 2000 (C) ITO, Takayuki
	All rights reserved
*/

#include "driver.h"

#define	DPRINTF_INFO(x)	dprintf x
#define	DPRINTF_ERR(x)	dprintf x

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

my_device_info *search_device_info (int number)
{
	my_device_info *my_dev;

	acquire_sem (my_device_list_lock);
	for (my_dev = my_device_list; my_dev != NULL; my_dev = my_dev->next)
	{
		if (my_dev->number == number)
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
		char *p = malloc (strlen (my_base_name) + 4);
		assert (p != NULL);
		sprintf (p, "%s%d", my_base_name, my_dev->number);
		DPRINTF_INFO ((MY_ID "publishing %s\n", p));
		my_device_names [i++] = p;
	}
	my_device_names [i] = NULL;
	release_sem (my_device_list_lock);
}

