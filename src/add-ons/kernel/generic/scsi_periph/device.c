/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open SCSI Peripheral Driver

	Basic handling of device.
*/


#include "scsi_periph_int.h"

#include <blkman.h>

#include <string.h>
#include <stdio.h>
#include <malloc.h>

#define SCSI_PERIPH_STD_TIMEOUT 10


char *
periph_compose_device_name(pnp_node_handle node, const char *prefix)
{
	uint8 path_id, target_id, target_lun;

	if (pnp->get_attr_uint8(node, SCSI_BUS_PATH_ID_ITEM, &path_id, true) != B_OK
		|| pnp->get_attr_uint8(node, SCSI_DEVICE_TARGET_ID_ITEM, &target_id, true) != B_OK
		|| pnp->get_attr_uint8(node, SCSI_DEVICE_TARGET_LUN_ITEM, &target_lun, true) != B_OK)
		return NULL;	

	{
		char name_buffer[100];
		sprintf(name_buffer, "%s/%d/%d/%d/raw", prefix, path_id, target_id, target_lun);

		return strdup(name_buffer);
	}
}


status_t
periph_register_device(periph_device_cookie periph_device, scsi_periph_callbacks *callbacks,
	scsi_device scsi_device, scsi_device_interface *scsi, pnp_node_handle node,
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
