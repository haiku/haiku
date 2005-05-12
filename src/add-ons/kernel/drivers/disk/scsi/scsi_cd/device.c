/*
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Open SCSI Disk Driver

	Device management.
*/

#include "scsi_cd_int.h"

#include <pnp_devfs.h>

#include <stdlib.h>
#include <string.h>


status_t
cd_init_device(device_node_handle node, void *user_cookie, void **cookie)
{
	cd_device_info *device;
	status_t res;
	bool removable;

	SHOW_FLOW0(3, "");

	device = (cd_device_info *)malloc(sizeof(*device));
	if (device == NULL)
		return B_NO_MEMORY;

	memset(device, 0, sizeof(*device));

	device->block_io_device = user_cookie;
	device->node = node;

	res = pnp->get_attr_uint8(node, "removable", 
		&removable, false);
	if (res != B_OK)
		goto err1;

	device->removable = removable;

	res = pnp->get_attr_uint8(node, SCSI_DEVICE_TYPE_ITEM, 
		&device->device_type, true);
	if (res != B_OK)
		goto err1;

	// we don't verify type - in worst case, the device doesn't work
		
	// register it everywhere	
	res = pnp->init_driver(pnp->get_parent(node), NULL, (driver_module_info **)&device->scsi, 
		(void **)&device->scsi_device);
	if (res != B_OK)
		goto err2;

	res = scsi_periph->register_device((periph_device_cookie)device, &callbacks,
		device->scsi_device, device->scsi, device->node, removable,
		&device->scsi_periph_device);
	if (res != B_OK)
		goto err3;

	// set capacity to zero, so it get checked on first opened handle
	device->capacity = 0;
	device->block_size = 0;

	SHOW_FLOW0(3, "done");

	*cookie = device;
	return B_OK;

err3:
	pnp->uninit_driver(pnp->get_parent(node));
err2:
err1:
	free(device);
	return res;
}


status_t
cd_uninit_device(cd_device_info *device)
{
	scsi_periph->unregister_device(device->scsi_periph_device);
	pnp->uninit_driver(pnp->get_parent(device->node));
	free(device);

	return B_OK;
}


/**	called whenever a new device was added to system;
 *	if we really support it, we create a new node that gets
 *	server by blkdev	
 */

status_t
cd_device_added(device_node_handle node)
{
	uint8 device_type;
	scsi_res_inquiry *device_inquiry = NULL;
	size_t inquiry_len;
	char *name;
	uint32 max_blocks;
	
	SHOW_FLOW0( 3, "" );
	
	// check whether it's really a CD-ROM or a WORM
	if( pnp->get_attr_uint8( node, SCSI_DEVICE_TYPE_ITEM, 
		&device_type, true ) != B_OK ||
		(device_type != scsi_dev_CDROM && device_type != scsi_dev_WORM) )
		goto err;

	// get inquiry data
	if( pnp->get_attr_raw( node, SCSI_DEVICE_INQUIRY_ITEM, 
		(void **)&device_inquiry, &inquiry_len, true ) != B_OK ||
		inquiry_len < sizeof( device_inquiry ) )
		goto err;
		
	// get block limit of underlying hardware to lower it (if necessary)
	if( pnp->get_attr_uint32( node, B_BLOCK_DEVICE_MAX_BLOCKS_ITEM, 
		&max_blocks, true ) != B_OK )
		max_blocks = INT_MAX;

	// using 10 byte commands, at most 0xffff blocks can be transmitted at once
	// (sadly, we cannot update this value later on if only 6 byte commands
	//  are supported, but the blkdev can live with truncated transfers)	
	max_blocks = min(max_blocks, 0xffff);

	name = scsi_periph->compose_device_name(node, "disk/scsi");
	if (name == NULL) {
		SHOW_ERROR0(2, "Cannot compose device name");
		goto err;
	}

	SHOW_FLOW(3, "name = %s", name);

	// ready to register
	{
		device_attr attrs[] = {
			{ B_DRIVER_MODULE, B_STRING_TYPE, { string: SCSI_CD_MODULE_NAME }},
			{ B_DRIVER_FIXED_CHILD, B_STRING_TYPE, { string: B_BLOCK_IO_MODULE_NAME }},

			// tell block_io whether the device is removable
			{ "removable", B_UINT8_TYPE, { ui8: device_inquiry->RMB }},
			// tell which name we want to have in devfs
			{ PNP_DEVFS_FILENAME, B_STRING_TYPE, { string: name }},
			// impose own max block restriction
			{ B_BLOCK_DEVICE_MAX_BLOCKS_ITEM, B_UINT32_TYPE, { ui32: max_blocks }},
			{ NULL }
		};
		status_t res;

		res = pnp->register_device(node, attrs, NULL, &node);

		free(name);
		free(device_inquiry);

		return res;
	}

err:
	free(device_inquiry);

	return B_ERROR;
}
