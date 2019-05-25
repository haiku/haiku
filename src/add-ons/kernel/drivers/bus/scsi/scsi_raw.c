/*
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Open SCSI raw driver.

	We don't support particular file handles, instead we use
	file handle = device handle.
*/


#include "scsi_raw.h"
#include <string.h>
#include <scsi.h>
#include <malloc.h>
#include <pnp_devfs.h>
#include <stdio.h>


device_manager_info *pnp;


static status_t
raw_open(void *device_cookie, uint32 flags, void **handle_cookie)
{
	*handle_cookie = device_cookie;
	return B_OK;
}


static status_t
raw_close(void *cookie)
{
	return B_OK;
}


static status_t
raw_free(void *cookie)
{
	return B_OK;
}


#if 0
static status_t
raw_control(void *cookie, uint32 op, void *data, size_t len)
{
	return B_OK;
}
#endif


static status_t
raw_read(void *cookie, off_t position, void *data, size_t *numBytes)
{
	return B_ERROR;
}


static status_t
raw_write(void *cookie, off_t position, const void *data, size_t *numBytes)
{
	return B_ERROR;
}


/* TODO: sync with scsi_periph module, this has been updated there to use
	user_memcpy */
static status_t
raw_command(raw_device_info *device, raw_device_command *cmd)
{
	scsi_ccb *request;

	SHOW_FLOW0(3, "");

	request = device->scsi->alloc_ccb(device->scsi_device);
	if (request == NULL)
		return B_NO_MEMORY;

	request->flags = 0;

	if (cmd->flags & B_RAW_DEVICE_DATA_IN) 
		request->flags |= SCSI_DIR_IN;
	else if (cmd->data_length)
		request->flags |= SCSI_DIR_OUT;
	else
		request->flags |= SCSI_DIR_NONE;

	request->data = cmd->data;
	request->sg_list = NULL;
	request->data_len = cmd->data_length;
	request->sort = -1;
	request->timeout = cmd->timeout;

	memcpy(request->cdb, cmd->command, SCSI_MAX_CDB_SIZE);
	request->cdb_len = cmd->command_length;

	device->scsi->sync_io(request);

	// TBD: should we call standard error handler here, or may the
	// actions done there (like starting the unit) confuse the application?

	cmd->cam_status = request->subsys_status;
	cmd->scsi_status = request->device_status;

	if ((request->subsys_status & SCSI_AUTOSNS_VALID) != 0 && cmd->sense_data) {
		memcpy(cmd->sense_data, request->sense,
			min((int32)cmd->sense_data_length, SCSI_MAX_SENSE_SIZE - request->sense_resid));
	}

	if ((cmd->flags & B_RAW_DEVICE_REPORT_RESIDUAL) != 0) {
		// this is a bit strange, see Be's sample code where I pinched this from;
		// normally, residual means "number of unused bytes left"
		// but here, we have to return "number of used bytes", which is the opposite
		cmd->data_length = cmd->data_length - request->data_resid;
		cmd->sense_data_length = SCSI_MAX_SENSE_SIZE - request->sense_resid;
	}

	device->scsi->free_ccb(request);
	return B_OK;
}


static status_t
raw_ioctl(raw_device_info *device, int op, void *buffer, size_t length)
{
	status_t res;

	switch (op) {
		case B_RAW_DEVICE_COMMAND:
			res = raw_command(device, buffer);
			break;

		default:
			res = B_DEV_INVALID_IOCTL;
	}

	SHOW_FLOW(4, "%x: %s", op, strerror(res));

	return res;
}


static status_t
raw_init_device(device_node_handle node, void *user_cookie, void **cookie)
{
	raw_device_info *device;
	status_t res;

	SHOW_FLOW0(3, "");

	device = (raw_device_info *)calloc(1, sizeof(*device));
	if (device == NULL)
		return B_NO_MEMORY;

	device->node = node;

	// register it everywhere
	res = pnp->init_driver(pnp->get_parent(node), NULL, 
			(driver_module_info **)&device->scsi, (void **)&device->scsi_device);
	if (res != B_OK)
		goto err;

	SHOW_FLOW0(3, "done");

	*cookie = device;
	return B_OK;

err:
	free(device);
	return res;
}


static status_t
raw_uninit_device(raw_device_info *device)
{
	pnp->uninit_driver(pnp->get_parent(device->node));
	free(device);

	return B_OK;
}


/**	called whenever a new SCSI device was added to system;
 *	we register a devfs entry for every device
 */

static status_t
raw_device_added(device_node_handle node)
{
	uint8 path_id, target_id, target_lun;
	char name[100];

	SHOW_FLOW0(3, "");

	// compose name	
	if (pnp->get_attr_uint8(node, SCSI_BUS_PATH_ID_ITEM, &path_id, true) != B_OK
		|| pnp->get_attr_uint8(node, SCSI_DEVICE_TARGET_ID_ITEM, &target_id, true) != B_OK
		|| pnp->get_attr_uint8(node, SCSI_DEVICE_TARGET_LUN_ITEM, &target_lun, true) != B_OK)
		return B_ERROR;

	sprintf(name, "bus/scsi/%d/%d/%d/raw",
		path_id, target_id, target_lun);

	SHOW_FLOW(3, "name=%s", name);

	// ready to register
	{
		device_attr attrs[] = {
			{ B_DRIVER_MODULE, B_STRING_TYPE, { string: SCSI_RAW_MODULE_NAME }},

			// default connection is used by peripheral drivers, and as we don't
			// want to kick them out, we use concurrent "raw" connection
			// (btw: this shows nicely that something goes wrong: one device
			// and two drivers means begging for trouble)
			{ PNP_DRIVER_CONNECTION, B_STRING_TYPE, { string: "raw" }},

			// we want devfs on top of us (who wouldn't?)
			{ B_DRIVER_FIXED_CHILD, B_STRING_TYPE, { string: PNP_DEVFS_MODULE_NAME }},
			// tell which name we want to have in devfs
			{ PNP_DEVFS_FILENAME, B_STRING_TYPE, { string: name }},
			{ NULL }
		};

		return pnp->register_device(node, attrs, NULL, &node);
	}
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


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&pnp },
	{}
};

pnp_devfs_driver_info scsi_raw_module = {
	{
		{
			SCSI_RAW_MODULE_NAME,
			0,			
			std_ops
		},

		NULL,
		raw_device_added,
		raw_init_device,
		(status_t (*) (void *))raw_uninit_device,
		NULL
	},

	(status_t (*)(void *, uint32, void **)) 		&raw_open,
	raw_close,
	raw_free,
	(status_t (*)(void *, uint32, void *, size_t))	&raw_ioctl,

	raw_read,
	raw_write,

	NULL,
	NULL,

	NULL,
	NULL
};

module_info *modules[] = {
	(module_info *)&scsi_raw_module,
	NULL
};
