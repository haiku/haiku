/*
 * Copyright 2004-2006, Haiku, Inc. All RightsReserved.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	Basic handling of device.
*/


#include "scsi_periph_int.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#define SCSI_PERIPH_STD_TIMEOUT 10


char *
periph_compose_device_name(device_node_handle node, const char *prefix)
{
	uint8 pathID, targetID, targetLUN, isPrimary, type;
	char name[128];
	uint32 channel;

	if (pnp->get_attr_uint8(node, SCSI_BUS_PATH_ID_ITEM, &pathID, true) != B_OK
		|| pnp->get_attr_uint8(node, SCSI_DEVICE_TARGET_ID_ITEM, &targetID, true) != B_OK
		|| pnp->get_attr_uint8(node, SCSI_DEVICE_TARGET_LUN_ITEM, &targetLUN, true) != B_OK)
		return NULL;	

	// IDE devices have a different naming scheme

	if (pnp->get_attr_uint32(node, "ide/channel_id", &channel, true) == B_OK
		&& pnp->get_attr_uint8(node, "ide_adapter/is_primary", &isPrimary, true) == B_OK
		&& pnp->get_attr_uint8(node, SCSI_DEVICE_TYPE_ITEM, &type, true) == B_OK) {
		// this is actually an IDE device, so we ignore the prefix

		// a bus device for those
		snprintf(name, sizeof(name), "disk/ata%s/%ld/%s/raw",
			type == scsi_dev_CDROM ? "pi" : "", channel,
			targetID == 0 ? "master" : "slave");
	} else {
		// this is a real SCSI device

		snprintf(name, sizeof(name), "%s/%d/%d/%d/raw",
			prefix, pathID, targetID, targetLUN);
	}

	return strdup(name);
}


status_t
periph_register_device(periph_device_cookie periph_device, scsi_periph_callbacks *callbacks,
	scsi_device scsi_device, scsi_device_interface *scsi, device_node_handle node,
	bool removable, scsi_periph_device *driver)
{
	scsi_periph_device_info *device;
	status_t res;

	SHOW_FLOW0( 3, "" );

	device = (scsi_periph_device_info *)malloc(sizeof(*device));
	if (device == NULL)
		return B_NO_MEMORY;

	memset(device, 0, sizeof(*device));

	if (INIT_BEN(&device->mutex, "SCSI_PERIPH") != B_OK) {
		res = B_NO_MEMORY;
		goto err1;
	}

	device->scsi_device = scsi_device;
	device->scsi = scsi;
	device->periph_device = periph_device;
	device->removal_requested = false;
	device->callbacks = callbacks;
	device->node = node;
	device->removable = removable;
	device->std_timeout = SCSI_PERIPH_STD_TIMEOUT;

	// set some default options
	device->next_tag_action = 0;
	device->rw10_enabled = true;

	// launch sync daemon	
	res = register_kernel_daemon(periph_sync_queue_daemon, device, 60*10);
	if (res != B_OK)
		goto err2;

	*driver = device;

	SHOW_FLOW0(3, "done");

	return B_OK;

err2:
	DELETE_BEN(&device->mutex);
err1:
	free(device);
	return res;
}


status_t
periph_unregister_device(scsi_periph_device_info *device)
{
	unregister_kernel_daemon(periph_sync_queue_daemon, device);
	free(device);

	return B_OK;
}
