/*
** Copyright 2002-04, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open SCSI bus manager

	Devfs entry for raw bus access.

	This interface will go away. It's used by scsi_probe as
	long as we have no proper pnpfs where all the info can
	be retrieved from.
*/

#include "scsi_internal.h"
#include <device/scsi_bus_raw_driver.h>
#include <pnp_devfs.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


// info about bus
// (used both as bus cookie and file handle cookie)
typedef struct bus_raw_info {
	scsi_bus_interface *interface;
	scsi_bus cookie;
	pnp_node_handle node;
} bus_raw_info;


static status_t scsi_bus_raw_init_device(
	pnp_node_handle	node, void *user_cookie, void **cookie )
{
	bus_raw_info *bus;
	scsi_bus_interface *interface;
	scsi_bus bus_cookie;
	status_t res;
	
	res = pnp->load_driver( pnp->get_parent( node ), NULL, 
		(pnp_driver_info **)&interface, (void **)&bus_cookie );
	if( res != B_OK )
		return res;
		
	bus = malloc( sizeof( *bus ));
	bus->interface = interface;
	bus->cookie = bus_cookie;
	bus->node = node;
	
	*cookie = bus;
	return B_OK;
}


static status_t scsi_bus_raw_uninit_device( bus_raw_info *bus )
{
	status_t res;
	
	res = pnp->unload_driver( pnp->get_parent( bus->node ));
	if( res != B_OK )
		return res;
		
	free( bus );
	return B_OK;
}

	
static status_t scsi_bus_raw_probe( pnp_node_handle parent )
{
	uint8 path_id;
	char *name;
	
	if( pnp->get_attr_uint8( parent, SCSI_BUS_PATH_ID_ITEM, &path_id, false ) != B_OK )
		return B_ERROR;

	// put that on heap to not overflow the limited kernel stack	
	name = malloc( PATH_MAX + 1 );
	if( name == NULL )
		return B_NO_MEMORY;
		
	sprintf( name, "bus/scsi/%d/bus_raw", path_id );
			
	{
		pnp_node_attr attributes[] = {
			{ PNP_DRIVER_DRIVER, B_STRING_TYPE, { string: SCSI_BUS_RAW_MODULE_NAME }},
			{ PNP_DRIVER_TYPE, B_STRING_TYPE, { string: PNP_DEVFS_TYPE_NAME }},
			{ PNP_DRIVER_FIXED_CONSUMER, B_STRING_TYPE, { string: PNP_DEVFS_MODULE_NAME }},

			{ PNP_DRIVER_CONNECTION, B_STRING_TYPE, { string: "bus_raw" }},
			{ PNP_DRIVER_DEVICE_IDENTIFIER, B_STRING_TYPE, { string: "bus_raw" }},
			
			{ PNP_DEVFS_FILENAME, B_STRING_TYPE, { string: name }},
			{}
		};
		
		pnp_node_handle node;
		status_t res;
		
		res = pnp->register_device( parent, attributes, NULL, &node );
		free( name );
		
		return res;
	}
}


static status_t scsi_bus_raw_open( bus_raw_info *bus, uint32 flags, 
	bus_raw_info **handle_cookie )
{
	*handle_cookie = bus;
	return B_ERROR;
}


static status_t scsi_bus_raw_close( void *cookie )
{
	return B_OK;
}


static status_t scsi_bus_raw_free( void *cookie )
{
	return B_ERROR;
}


static status_t scsi_bus_raw_control( bus_raw_info *bus, uint32 op, void *data,
	size_t len )
{
	switch( op ) {
	case B_SCSI_BUS_RAW_RESET:
		return bus->interface->reset_bus( bus->cookie );
		
	case B_SCSI_BUS_RAW_PATH_INQUIRY:
		return bus->interface->path_inquiry( bus->cookie, data );
	}

	return B_ERROR;
}


static status_t scsi_bus_raw_read( void *cookie, off_t position, void *data,
	size_t *numBytes )
{
	*numBytes = 0;
	return B_ERROR;
}


static status_t scsi_bus_raw_write( void *cookie, off_t position,
	const void *data, size_t *numBytes )
{
	*numBytes = 0;
	return B_ERROR;
}


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


pnp_devfs_driver_info scsi_bus_raw_module = {
	{
		{
			SCSI_BUS_RAW_MODULE_NAME,
			0,
			std_ops
		},

		scsi_bus_raw_init_device,
		(status_t (*) (void *))scsi_bus_raw_uninit_device,
		scsi_bus_raw_probe,
		NULL,
		NULL
	},

	(status_t (*) (void *, uint32, void **))scsi_bus_raw_open,
	scsi_bus_raw_close,
	scsi_bus_raw_free,
	(status_t (*) (void *, uint32, void *, size_t))scsi_bus_raw_control,
	scsi_bus_raw_read,
	scsi_bus_raw_write
};
