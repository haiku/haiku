/*
 * Copyright 2004-2007, Haiku, Inc. All RightsReserved.
 * Copyright 2002-2004, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

//!	Device management.


#include "scsi_dsk_int.h"

#include <pnp_devfs.h>

#include <stdlib.h>
#include <string.h>


status_t
das_init_device(device_node *node, void **cookie)
{
	das_device_info *device;
	status_t status;
	uint8 removable;

	SHOW_FLOW0(3, "");

	status = pnp->get_attr_uint8(node, "removable",
		&removable, false);
	if (status != B_OK)
		return status;

	device = (das_device_info *)malloc(sizeof(*device));
	if (device == NULL)
		return B_NO_MEMORY;

	memset(device, 0, sizeof(*device));

	device->node = node;
	device->removable = removable;

	{
		device_node *parent = pnp->get_parent_node(node);
		pnp->get_driver(parent, (driver_module_info **)&device->scsi,
			(void **)&device->scsi_device);
		pnp->put_node(parent);
	}

	status = scsi_periph->register_device((periph_device_cookie)device,
		&callbacks, device->scsi_device, device->scsi, device->node,
		device->removable, &device->scsi_periph_device);
	if (status != B_OK) {
		free(device);
		return status;
	}

	SHOW_FLOW0(3, "done");

	*cookie = device;
	return B_OK;
}


void
das_uninit_device(void *_cookie)
{
	das_device_info *device = (das_device_info *)_cookie;

	scsi_periph->unregister_device(device->scsi_periph_device);
	free(device);
}


/**	called whenever a new device was added to system;
 *	if we really support it, we create a new node that gets
 *	server by the block_io module
 */

status_t
das_device_added(device_node *node)
{
	const scsi_res_inquiry *deviceInquiry = NULL;
	uint8 device_type;
	size_t inquiryLength;
	uint32 max_blocks;

	// check whether it's really a Direct Access Device
	if (pnp->get_attr_uint8(node, SCSI_DEVICE_TYPE_ITEM, &device_type, true) != B_OK
		|| device_type != scsi_dev_direct_access)
		return B_ERROR;

	// get inquiry data
	if (pnp->get_attr_raw(node, SCSI_DEVICE_INQUIRY_ITEM,
			(const void **)&deviceInquiry, &inquiryLength, true) != B_OK
		|| inquiryLength < sizeof(deviceInquiry))
		return B_ERROR;

	// get block limit of underlying hardware to lower it (if necessary)
	if (pnp->get_attr_uint32(node, B_BLOCK_DEVICE_MAX_BLOCKS_ITEM, &max_blocks,
			true) != B_OK)
		max_blocks = INT_MAX;

	// using 10 byte commands, at most 0xffff blocks can be transmitted at once
	// (sadly, we cannot update this value later on if only 6 byte commands
	//  are supported, but the block_io module can live with that)
	max_blocks = min(max_blocks, 0xffff);

	// ready to register
	{
		device_attr attrs[] = {
			// tell block_io whether the device is removable
			{ "removable", B_UINT8_TYPE, { ui8: deviceInquiry->removable_medium }},
			// impose own max block restriction
			{ B_BLOCK_DEVICE_MAX_BLOCKS_ITEM, B_UINT32_TYPE, { ui32: max_blocks }},
			// in general, any disk can be a BIOS drive (even ZIP-disks)
			{ B_BLOCK_DEVICE_IS_BIOS_DRIVE, B_UINT8_TYPE, { ui8: 1 }},
			{ NULL }
		};

		return pnp->register_node(node, SCSI_DSK_MODULE_NAME, attrs, NULL,
			NULL);
	}
}
