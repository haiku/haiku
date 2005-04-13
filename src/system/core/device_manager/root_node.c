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


#define PNP_ROOT_TYPE_NAME "pnp/root"
	// type of pnp root device

#define PNP_ROOT_DRIVERS_DIR "root"
	// registration directory of pnp root drivers
	// (put all drivers under "universal" and use unique connection names)

#define PNP_ROOT_MODULE_NAME "sys/pnp_root/v1"


static device_node_handle sRootNode;


void
pnp_root_init_root(void)
{
	device_attr attrs[] = {
		{ PNP_DRIVER_DRIVER, B_STRING_TYPE, { string: PNP_ROOT_MODULE_NAME }},
		{ PNP_DRIVER_TYPE, B_STRING_TYPE, { string: PNP_ROOT_TYPE_NAME }},

		// well - connection is actually pointless as there is no other root node
		{ PNP_DRIVER_CONNECTION, B_STRING_TYPE, { string: "pnp_root" }},
		{ PNP_DRIVER_DEVICE_IDENTIFIER, B_STRING_TYPE, { string: "pnp_root" }},

		// directory for root drivers
		// ToDo: temporary hack to get things started!
		{ PNP_DRIVER_FIXED_CONSUMER, B_STRING_TYPE, { string: "bus_managers/isa/root" }},
		{ PNP_DRIVER_DYNAMIC_CONSUMER, B_STRING_TYPE, { string: PNP_ROOT_DRIVERS_DIR "/" }},

		{ NULL }
	};

	if (pnp_register_device(NULL, attrs, NULL, &sRootNode) != B_OK)
		dprintf("Cannot register PnP-Root\n");
	// ToDo: don't panic for now
	//	panic("Cannot register PnP-Root\n");
}


void
pnp_root_destroy_root(void)
{
	// make sure we are registered
	if (sRootNode == NULL)
		return;

	TRACE(("Destroying PnP-root\n"));

	if (pnp_unregister_device(sRootNode) != B_OK)
		return;

	sRootNode = NULL;
}


void
pnp_root_rescan_root(void)
{
	// make sure we are registered
	if (sRootNode == NULL)
		return;

	TRACE(("Rescanning PnP-root\n"));

	// scan _very_ deep
	pnp_rescan(sRootNode, 30000);
}


static status_t
pnp_root_init_device(device_node_handle node, void *user_cookie, void **cookie)
{
	*cookie = NULL;
	return B_OK;
}


static status_t
pnp_root_uninit_device(void *cookie)
{
	return B_OK;
}

/*
static status_t pnp_root_device_added( 
	device_node_handle parent )
{
	char *tmp;
	status_t res;
	device_node_handle loader_node;
	
	// make sure parent is really root
	if( pnp->get_attr_string( parent, PNP_DRIVER_TYPE, &tmp, false ))
		return B_ERROR;
		
	if( strcmp( tmp, PNP_ROOT_NODE_TYPE_NAME ) != 0 ) {
		free( tmp );
		return B_ERROR;
	}
	
	free( tmp );

	// load ISA bus manager
	// if you don't want ISA, remove this registration
	{
		device_attr attrs[] = {
			// info about ourself
			{ PNP_DRIVER_DRIVER, B_STRING_TYPE, { string: PNP_ROOT_MODULE_NAME }},
			{ PNP_DRIVER_TYPE, B_STRING_TYPE, { string: "pnp/isa_loader" }},
			{ PNP_DRIVER_FIXED_CONSUMER, B_STRING_TYPE, { string: ISA2_MODULE_NAME }},
			{ PNP_DRIVER_CONNECTION, B_STRING_TYPE, { string: "ISA" }},
			{ PNP_DRIVER_DEVICE_IDENTIFIER, B_STRING_TYPE, { string: "ISA" }},
			{ NULL }
		};
		
		SHOW_FLOW0( 4, "registering ISA root" );

		res = pnp->register_device( parent, attrs, NULL, &loader_node );
		if( res != B_OK )
			return res;
	}

	// load PCI bus manager
	{
		device_attr attrs[] = {
			// info about ourself
			{ PNP_DRIVER_DRIVER, B_STRING_TYPE, { string: PNP_ROOT_MODULE_NAME }},
			{ PNP_DRIVER_TYPE, B_STRING_TYPE, { string: "pnp/pci_loader" }},
			{ PNP_DRIVER_FIXED_CONSUMER, B_STRING_TYPE, { string: PCI_ROOT_MODULE_NAME }},
			{ PNP_DRIVER_CONNECTION, B_STRING_TYPE, { string: "PCI" }},
			{ PNP_DRIVER_DEVICE_IDENTIFIER, B_STRING_TYPE, { string: "PCI" }},
			{ NULL }
		};
		
		SHOW_FLOW0( 4, "registering PCI root" );

		res = pnp->register_device( parent, attrs, NULL, &loader_node );
		if( res != B_OK )
			return res;
	}

	return B_OK;		

}
*/

static status_t
std_ops(int32 op, ...)
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
		std_ops,
	},

	pnp_root_init_device,
	pnp_root_uninit_device,
	NULL,
	NULL,
	NULL
};

