/* 
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of PnP devfs

	Interface of PnP root module.

	This module creates the root devices (PCI and ISA) to initiate
	the PnP device scan. It is calles by pnp_boot_helper but should 
	probably be moved into kernel for simplification.

	In the future, we want to check hardware configuration before we
	start loading ISA and PCI bus manager.
*/


#include <KernelExport.h>
#include <Drivers.h>

#include "device_manager_private.h"

//#include <pnp/pnp_manager.h>
/*#include <bus/ISA2.h>
#include <bus/PCI2.h>*/


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


static pnp_node_handle sRootNode;


void
pnp_root_init_root(void)
{
	pnp_node_attr attrs[] = {
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
pnp_root_init_device(pnp_node_handle node, void *user_cookie, void **cookie)
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
	pnp_node_handle parent )
{
	char *tmp;
	status_t res;
	pnp_node_handle loader_node;
	
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
		pnp_node_attr attrs[] = {
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
		pnp_node_attr attrs[] = {
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


pnp_driver_info gDeviceRootModule = {
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

