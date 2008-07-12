/*
	USB joystick driver for BeOS R5
	Copyright 2000 (C) ITO, Takayuki
	All rights reserved
*/

#include "driver.h"

#define	DPRINTF_INFO(x)	dprintf x
#define	DPRINTF_ERR(x)	dprintf x

static int device_number = 0;

my_device_info *create_device (
	const usb_device *dev, 
	const usb_interface_info *ii)
{
	my_device_info *my_dev = NULL;
	int number;
	area_id area;
	sem_id sem;
	char	area_name [32];

	assert (usb != NULL && dev != NULL);

	number = device_number++;

	my_dev = malloc (sizeof (my_device_info));
	if (my_dev == NULL)
		return NULL;

	my_dev->sem_cb = sem = create_sem (0, DRIVER_NAME "_cb");
	if (sem < 0)
	{
		DPRINTF_ERR ((MY_ID "create_sem() failed %d\n", (int) sem));
		free (my_dev);
		return NULL;
	}

	my_dev->sem_lock = sem = create_sem (1, DRIVER_NAME "_lock");
	if (sem < 0)
	{
		DPRINTF_ERR ((MY_ID "create_sem() failed %d\n", (int) sem));
		delete_sem (my_dev->sem_cb);
		free (my_dev);
		return NULL;
	}

	sprintf (area_name, DRIVER_NAME "_buffer%d", number);
	my_dev->buffer_area = area = create_area (area_name,
		(void **) &my_dev->buffer, B_ANY_KERNEL_ADDRESS,
		B_PAGE_SIZE, B_CONTIGUOUS, B_READ_AREA | B_WRITE_AREA);
	if (area < 0)
	{
		DPRINTF_ERR ((MY_ID "create_area() failed %d\n", (int) area));
		delete_sem (my_dev->sem_cb);
		delete_sem (my_dev->sem_lock);
		free (my_dev);
		return NULL;
	}

	my_dev->buffer_valid = false;
	my_dev->dev = dev;
	my_dev->number = number;
	my_dev->open = 0;
	my_dev->open_fds = NULL;
	my_dev->active = true;
	my_dev->insns = NULL;
	my_dev->num_insns = 0;
	my_dev->flags = 0;

	return my_dev;
}

void remove_device (my_device_info *my_dev)
{
	assert (my_dev != NULL);
	delete_area (my_dev->buffer_area);
	delete_sem (my_dev->sem_cb);
	delete_sem (my_dev->sem_lock);
	free (my_dev);
}
