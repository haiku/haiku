/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open SCSI Disk Driver

	Device management.
*/

#include "scsi_dsk_int.h"

#include <pnp_devfs.h>
#include <stdlib.h>
#include <string.h>


status_t
das_init_device(pnp_node_handle node, void *user_cookie, void **cookie)
{
	das_device_info *device;
	status_t res;
	scsi_ccb *request;

	SHOW_FLOW0( 3, "" );

	device = (das_device_info *)malloc(sizeof(*device));
	if (device == NULL)
		return B_NO_MEMORY;

	memset(device, 0, sizeof(*device));

	device->blkman_device = user_cookie;
	device->node = node;

	res = pnp->get_attr_uint8(node, "removable", 
		&device->removable, false);
	if (res != B_OK)
		goto err1;

	// register it everywhere	
	res = pnp->load_driver(pnp->get_parent(node), NULL, (pnp_driver_info **)&device->scsi, 
		(void **)&device->scsi_device);
	if (res != B_OK)
		goto err2;

	res = scsi_periph->register_device((periph_device_cookie)device, &callbacks,
		device->scsi_device, device->scsi, device->node, device->removable,
		&device->scsi_periph_device);
	if (res != B_OK)
		goto err3;

	// and get (initial) capacity
	request = device->scsi->alloc_ccb(device->scsi_device);
	if (request == NULL) {
		res = B_NO_MEMORY;
		goto err4;
	}

	scsi_periph->check_capacity(device->scsi_periph_device, request);

	device->scsi->free_ccb(request);

	SHOW_FLOW0(3, "done");

	*cookie = device;
	return B_OK;

err4:
	scsi_periph->unregister_device(device->scsi_periph_device);
err3:
	pnp->unload_driver(pnp->get_parent(node));
err2:
err1:
	free(device);
	return res;
}


status_t
das_uninit_device(das_device_info *device)
{
	scsi_periph->unregister_device(device->scsi_periph_device);
	pnp->unload_driver(pnp->get_parent(device->node));
	free(device);

	return B_OK;
}


/**	called whenever a new device was added to system;
 *	if we really support it, we create a new node that gets
 *	server by blkdev
 */

status_t
das_device_added(pnp_node_handle node)
{
	char *str = NULL;
	scsi_res_inquiry *device_inquiry = NULL;
	uint8 device_type;
	size_t inquiry_len;
	char *name;
	uint32 max_blocks;

	// make sure we can handle this parent device
	if (pnp->get_attr_string(node, PNP_DRIVER_TYPE, &str, false) != B_OK
		|| strcmp(str, SCSI_DEVICE_TYPE_NAME) != 0)
		goto err;

	// check whether it's really a Direct Access Device
	if (pnp->get_attr_uint8(node, SCSI_DEVICE_TYPE_ITEM, &device_type, true) != B_OK
		|| device_type != scsi_dev_direct_access)
		goto err;

	// get inquiry data
	if (pnp->get_attr_raw(node, SCSI_DEVICE_INQUIRY_ITEM,
			(void **)&device_inquiry, &inquiry_len, true) != B_OK
		|| inquiry_len < sizeof(device_inquiry))
		goto err;

	// get block limit of underlying hardware to lower it (if necessary)
	if (pnp->get_attr_uint32(node, BLKDEV_MAX_BLOCKS_ITEM, &max_blocks, true) != B_OK)
		max_blocks = INT_MAX;

	// using 10 byte commands, at most 0xffff blocks can be transmitted at once
	// (sadly, we cannot update this value later on if only 6 byte commands
	//  are supported, but the blkdev can live with that)	
	max_blocks = min(max_blocks, 0xffff);

	name = scsi_periph->compose_device_name(node, "disk/scsi");
	if (name == NULL) {
		SHOW_ERROR0(2, "Cannot compose device name");
		goto err;
	}

	SHOW_FLOW(3, "name=%s", name);

	// ready to register
	{
		pnp_node_attr attrs[] =
		{
			{ PNP_DRIVER_DRIVER, B_STRING_TYPE, { string: SCSI_DSK_MODULE_NAME }},
			{ PNP_DRIVER_TYPE, B_STRING_TYPE, { string: BLKDEV_TYPE_NAME }},
			// we always want blkdev on top of us
			{ PNP_DRIVER_FIXED_CONSUMER, B_STRING_TYPE, { string: BLKMAN_MODULE_NAME }},
			// tell blkdev whether the device is removable
			{ "removable", B_UINT8_TYPE, { ui8: device_inquiry->RMB }},
			// tell which name we want to have in devfs
			{ PNP_DEVFS_FILENAME, B_STRING_TYPE, { string: name }},
			// impose own max block restriction
			{ BLKDEV_MAX_BLOCKS_ITEM, B_UINT32_TYPE, { ui32: max_blocks }},
			// in general, any disk can be a BIOS drive (even ZIP-disks)
			{ BLKDEV_IS_BIOS_DRIVE, B_UINT8_TYPE, { ui8: 1 }},
			{ NULL }
		};

		status_t res = pnp->register_device(node, attrs, NULL, &node);

		free(name);
		free(str);
		free(device_inquiry);

		return res;
	}
	
err:
	free(str);
	free(device_inquiry);

	return B_ERROR;
}
