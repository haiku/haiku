/*
 * Copyright 2002-04, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


//!	Devfs entry for raw bus access.


#include "scsi_internal.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <device/scsi_bus_raw_driver.h>


// info about bus
// (used both as bus cookie and file handle cookie)
typedef struct bus_raw_info {
	scsi_bus_interface *interface;
	scsi_bus cookie;
	device_node *node;
} bus_raw_info;


static status_t
scsi_bus_raw_init(void *driverCookie, void **_cookie)
{
	device_node *node = (device_node *)driverCookie;
	device_node *parent;
	bus_raw_info *bus;

	bus = (bus_raw_info*)malloc(sizeof(*bus));
	if (bus == NULL)
		return B_NO_MEMORY;

	parent = pnp->get_parent_node(node);
	pnp->get_driver(parent,
		(driver_module_info **)&bus->interface, (void **)&bus->cookie);
	pnp->put_node(parent);

	bus->node = node;

	*_cookie = bus;
	return B_OK;
}


static void
scsi_bus_raw_uninit(void *bus)
{
	free(bus);
}


static status_t
scsi_bus_raw_open(void *bus, const char *path, int openMode,
	void **handle_cookie)
{
	*handle_cookie = bus;
	return B_OK;
}


static status_t
scsi_bus_raw_close(void *cookie)
{
	return B_OK;
}


static status_t
scsi_bus_raw_free(void *cookie)
{
	return B_OK;
}


static status_t
scsi_bus_raw_control(void *_cookie, uint32 op, void *data, size_t length)
{
	bus_raw_info *bus = (bus_raw_info*)_cookie;

	switch (op) {
		case B_SCSI_BUS_RAW_RESET:
			return bus->interface->reset_bus(bus->cookie);

		case B_SCSI_BUS_RAW_PATH_INQUIRY:
			return bus->interface->path_inquiry(bus->cookie,
				(scsi_path_inquiry*)data);
	}

	return B_ERROR;
}


static status_t
scsi_bus_raw_read(void *cookie, off_t position, void *data,
	size_t *numBytes)
{
	*numBytes = 0;
	return B_ERROR;
}


static status_t
scsi_bus_raw_write(void *cookie, off_t position,
	const void *data, size_t *numBytes)
{
	*numBytes = 0;
	return B_ERROR;
}


struct device_module_info gSCSIBusRawModule = {
	{
		SCSI_BUS_RAW_MODULE_NAME,
		0,
		NULL
	},

	scsi_bus_raw_init,
	scsi_bus_raw_uninit,
	NULL,	// removed

	scsi_bus_raw_open,
	scsi_bus_raw_close,
	scsi_bus_raw_free,
	scsi_bus_raw_read,
	scsi_bus_raw_write,
	NULL,	// io
	scsi_bus_raw_control,
	NULL,	// select
	NULL	// deselect
};
