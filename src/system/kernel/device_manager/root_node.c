/*
 * Copyright 2004-2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002/2003, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Device Manager

	Interface of device root module.

	This module creates the root devices (usually PCI and ISA) to
	initiate the PnP device scan.
*/


#include <KernelExport.h>
#include <Drivers.h>

#include "device_manager_private.h"


#define TRACE_ROOT_NODE
#ifdef TRACE_ROOT_NODE
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


#define PNP_ROOT_MODULE_NAME "system/devices_root/driver_v1"


void
dm_init_root_node(void)
{
	device_attr attrs[] = {
		{ B_DRIVER_MODULE, B_STRING_TYPE, { string: PNP_ROOT_MODULE_NAME }},
		{ B_DRIVER_PRETTY_NAME, B_STRING_TYPE, { string: "Devices Root" }},

		{ B_DRIVER_BUS, B_STRING_TYPE, { string: "root" }},
		{ NULL }
	};

	if (dm_register_node(NULL, attrs, NULL, &gRootNode) != B_OK) {
		// ToDo: don't panic for now
		dprintf("Cannot register Devices Root Node\n");
	}
}


static status_t
root_init_driver(device_node_handle node, void *user_cookie, void **_cookie)
{
	*_cookie = NULL;
	return B_OK;
}


static status_t
root_uninit_driver(void *cookie)
{
	return B_OK;
}


static status_t
root_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


driver_module_info gDeviceRootModule = {
	{
		PNP_ROOT_MODULE_NAME,
		0,
		root_std_ops,
	},

	NULL,	// supported devices
	NULL,	// register device
	root_init_driver,
	root_uninit_driver,
	NULL,
	NULL
};

