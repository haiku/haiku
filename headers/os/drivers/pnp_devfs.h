/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	PnP devfs driver interface.

	The interface follows the normal pnp driver rules, i.e.
	your driver creates a node for it and chooses PNP_DEVFS_MODULE_NAME
	as the consumer, the type must be PNP_DEVFS_TYPE_NAME. The file 
	name is set via a PNP_DEVFS_FILENAME item. 

	Loading and unloading of your driver is done via the pnp manager.
	Hooks are similar to the original devfs interface, but the open
	hook gets the driver cookie (as returned by init_device of the
	driver) instead of a filename.
*/

#ifndef _PNP_DEVFS_H
#define _PNP_DEVFS_H

#include <Drivers.h>
#include <device_manager.h>
//#include "r5_wrapper.h"

// changed open hook - gets cookie returned by init_device instead of name
typedef status_t (*pnp_device_open_hook)(void *device_cookie, uint32 flags, 
	void **handle_cookie);

// new interface to devfs
typedef struct pnp_devfs_driver_info {
	pnp_driver_info			dinfo;

	pnp_device_open_hook	open;			// called to open the device
	device_close_hook		close;			// called to close the device
	device_free_hook		free;			// called to free the cookie
	device_control_hook		control;		// called to control the device
	device_read_hook		read;			// reads from the device
	device_write_hook		write;			// writes to the device
	device_select_hook		select;			// start select (can be NULL)
	device_deselect_hook	deselect;		// stop select (can be NULL)
	device_readv_hook		readv;			// scatter-gather read from the device (can be NULL)
	device_writev_hook		writev;			// scatter-gather write to the device (can be NULL)
} pnp_devfs_driver_info;


// items:
// name under which the device should be published under /dev (required, string)
#define PNP_DEVFS_FILENAME		"devfs/filename"

// type of devfs device drivers
#define PNP_DEVFS_TYPE_NAME		"devfs_device"

#define PNP_DEVFS_MODULE_NAME	"sys/pnp_devfs/v1"

#endif
