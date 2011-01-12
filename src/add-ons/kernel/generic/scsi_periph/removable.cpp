/*
 * Copyright 2004-2011, Haiku, Inc. All RightsReserved.
 * Copyright 2002-2003, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


//!	Handling of removable media.


#include "scsi_periph_int.h"

#include <string.h>


void
periph_media_changed(scsi_periph_device_info *device, scsi_ccb *request)
{
	uint32 backup_flags;
	uint8 backup_cdb[SCSI_MAX_CDB_SIZE];
	uchar backup_cdb_len;
	int64 backup_sort;
	bigtime_t backup_timeout;
	uchar *backup_data;
	const physical_entry *backup_sg_list;
	uint16 backup_sg_count;
	uint32 backup_data_len;

	// if there is no hook, the driver doesn't handle removal devices
	if (!device->removable) {
		SHOW_ERROR0( 1, "Driver doesn't support medium changes, but there occured one!?" );
		return;
	}

	// when medium has changed, tell all handles
	periph_media_changed_public(device);

	// the peripheral driver may need a fresh ccb; sadly, we cannot allocate one
	// as this may lead to a deadlock if all ccb are in use already; thus, we
	// have to backup all relevant data of current ccb and use it instead of a 
	// new one - not pretty but working (and performance is not an issue in this
	// path)
	backup_flags = request->flags;
	memcpy(backup_cdb, request->cdb, SCSI_MAX_CDB_SIZE);
	backup_cdb_len = request->cdb_length;
	backup_sort = request->sort;
	backup_timeout = request->timeout;
	backup_data = request->data;
	backup_sg_list = request->sg_list;
	backup_sg_count = request->sg_count;
	backup_data_len = request->data_length;

	if (device->callbacks->media_changed != NULL)
		device->callbacks->media_changed(device->periph_device, request);

	request->flags = backup_flags;
	memcpy(request->cdb, backup_cdb, SCSI_MAX_CDB_SIZE);
	request->cdb_length = backup_cdb_len;
	request->sort = backup_sort;
	request->timeout = backup_timeout;
	request->data = backup_data;
	request->sg_list = backup_sg_list;
	request->sg_count = backup_sg_count;
	request->data_length = backup_data_len;
}


void
periph_media_changed_public(scsi_periph_device_info *device)
{
	scsi_periph_handle_info *handle;

	mutex_lock(&device->mutex);

	// when medium has changed, tell all handles	
	// (this must be atomic for each handle!)
	for (handle = device->handles; handle; handle = handle->next)
		handle->pending_error = B_DEV_MEDIA_CHANGED;

	mutex_unlock(&device->mutex);
}


/** send TUR */

static err_res
send_tur(scsi_periph_device_info *device, scsi_ccb *request)
{
	scsi_cmd_tur *cmd = (scsi_cmd_tur *)request->cdb;

	request->flags = SCSI_DIR_NONE | SCSI_ORDERED_QTAG;

	request->data = NULL;
	request->sg_list = NULL;
	request->data_length = 0;
	request->timeout = device->std_timeout;
	request->sort = -1;
	request->sg_list = NULL;

	memset(cmd, 0, sizeof(*cmd));
	cmd->opcode = SCSI_OP_TEST_UNIT_READY;

	request->cdb_length = sizeof(*cmd);

	device->scsi->sync_io(request);

	return periph_check_error(device, request);
}


/** wait until device is ready */

static err_res
wait_for_ready(scsi_periph_device_info *device, scsi_ccb *request)
{	
	int retries = 0;
	
	while (true) {
		err_res res;

		// we send TURs until the device is OK or all hope is lost
		res = send_tur(device, request);

		switch (res.action) {
			case err_act_ok:
				return MK_ERROR(err_act_ok, B_OK);

			case err_act_retry:
				if (++retries >= 3)
					return MK_ERROR(err_act_fail, res.error_code);
				break;

			case err_act_many_retries:
				if (++retries >= 30)
					return MK_ERROR(err_act_fail, res.error_code);
				break;

			default:
				SHOW_FLOW( 3, "action: %x, error: %x", (int)res.action, (int)res.error_code);
				return res;
		}
	}
}


status_t
periph_get_media_status(scsi_periph_handle_info *handle)
{
	scsi_periph_device_info *device = handle->device;
	scsi_ccb *request;
	err_res res;
	status_t err;

	mutex_lock(&device->mutex);

	// removal requests are returned to exactly one handle
	// (no real problem, as noone check medias status "by mistake")	
	if (device->removal_requested) {
		device->removal_requested = false;
		err = B_DEV_MEDIA_CHANGE_REQUESTED;
		goto err;
	}

	// if there is a pending error (read: media has changed), return once per handle
	err = handle->pending_error;
	if (err != B_OK) {
		handle->pending_error = B_OK;
		goto err;
	}

	SHOW_FLOW0( 3, "" );

	mutex_unlock(&device->mutex);

	// finally, ask the device itself
		
	request = device->scsi->alloc_ccb(device->scsi_device);
	if (request == NULL)
		return B_NO_MEMORY;

	res = wait_for_ready(device, request);

	device->scsi->free_ccb(request);

	SHOW_FLOW(3, "error_code: %x", (int)res.error_code);

	return res.error_code;	

err:	
	mutex_unlock(&device->mutex);
	return err;
}


/*!	Send START/STOP command to device
	start - true for start, false for stop
	withLoadEject - if true, then lock drive on start and eject on stop
*/
err_res
periph_send_start_stop(scsi_periph_device_info *device, scsi_ccb *request,
	bool start, bool withLoadEject)
{
	scsi_cmd_ssu *cmd = (scsi_cmd_ssu *)request->cdb;

	// this must be ordered, so all previous commands are really finished
	request->flags = SCSI_DIR_NONE | SCSI_ORDERED_QTAG;

	request->data = NULL;
	request->sg_list = NULL;
	request->data_length = 0;
	request->timeout = device->std_timeout;
	request->sort = -1;
	request->sg_list = NULL;

	memset(cmd, 0, sizeof(*cmd));
	cmd->opcode = SCSI_OP_START_STOP;
	// we don't want to poll; we give a long timeout instead
	// (well - the default timeout _is_ large)
	cmd->immediately = 0;
	cmd->start = start;
	cmd->load_eject = withLoadEject;

	request->cdb_length = sizeof(*cmd);

	device->scsi->sync_io(request);

	return periph_check_error(device, request);
}
