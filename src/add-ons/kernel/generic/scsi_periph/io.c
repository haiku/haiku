/*
 * Copyright 2004-2006, Haiku, Inc. All RightsReserved.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Open SCSI Disk Driver
	Everything doing the real input/output stuff.
*/

#include "scsi_periph_int.h"
#include <scsi.h>

#include <string.h>
#include <stdlib.h>


// we don't want to inline this function - it's just not worth it
static int periph_read_write(scsi_periph_handle_info *handle, const phys_vecs *vecs,
	off_t pos, size_t num_blocks, uint32 block_size, size_t *bytes_transferred,
	int preferred_ccb_size, bool write);


status_t
periph_read(scsi_periph_handle_info *handle, const phys_vecs *vecs, off_t pos,
	size_t num_blocks, uint32 block_size, size_t *bytes_transferred, int preferred_ccb_size)
{
	return periph_read_write(handle, vecs, pos, num_blocks, block_size,
				bytes_transferred, preferred_ccb_size, false);
}


status_t
periph_write(scsi_periph_handle_info *handle, const phys_vecs *vecs, off_t pos,
	size_t num_blocks, uint32 block_size, size_t *bytes_transferred, int preferred_ccb_size)
{
	return periph_read_write(handle, vecs, pos, num_blocks, block_size,
				bytes_transferred, preferred_ccb_size, true);
}


/** universal read/write function */

static int
periph_read_write(scsi_periph_handle_info *handle, const phys_vecs *vecs,
	off_t pos64, size_t num_blocks, uint32 block_size, size_t *bytes_transferred, 
	int preferred_ccb_size, bool write)
{
	scsi_periph_device_info *device = handle->device;
	scsi_ccb *request;
	err_res res;
	int retries = 0;
	int err;
	uint32 pos = pos64;

	// don't test rw10_enabled restrictions - this flag may get changed	
	request = device->scsi->alloc_ccb(device->scsi_device);

	if (request == NULL)
		return B_NO_MEMORY;

	do {
		size_t num_bytes;
		bool is_rw10;

		request->flags = write ? SCSI_DIR_OUT : SCSI_DIR_IN;

		// make sure we avoid 10 byte commands if they aren't supported
		if( !device->rw10_enabled || preferred_ccb_size == 6) {
			// restricting transfer is OK - the block manager will
			// take care of transferring the rest
			if (num_blocks > 0x100)
				num_blocks = 0x100;

			// no way to break the 21 bit address limit	
			if (pos64 > 0x200000) {
				err = B_BAD_VALUE;
				goto abort;
			}

			// don't allow transfer cross the 24 bit address limit
			// (I'm not sure whether this is allowed, but this way we
			// are sure to not ask for trouble)
			num_blocks = min(num_blocks, 0x100000 - pos);
		}

		num_bytes = num_blocks * block_size;

		request->data = NULL;
		request->sg_list = vecs->vec;
		request->data_len = num_bytes;
		request->sg_cnt = vecs->num;
		request->sort = pos;
		request->timeout = device->std_timeout;
		// see whether daemon instructed us to post an ordered command;
		// reset flag after read
		SHOW_FLOW( 3, "flag=%x, next_tag=%x, ordered: %s", 
			(int)request->flags, (int)device->next_tag_action,
			(request->flags & SCSI_ORDERED_QTAG) != 0 ? "yes" : "no" );

		// use shortest commands whenever possible
		if (pos + num_blocks < 0x200000 && num_blocks <= 0x100) {
			scsi_cmd_rw_6 *cmd = (scsi_cmd_rw_6 *)request->cdb;

			is_rw10 = false;

			memset(cmd, 0, sizeof(*cmd));
			cmd->opcode = write ? SCSI_OP_WRITE_6 : SCSI_OP_READ_6;
			cmd->high_LBA = (pos >> 16) & 0x1f;
			cmd->mid_LBA = (pos >> 8) & 0xff;
			cmd->low_LBA = pos & 0xff;
			cmd->length = num_blocks;

			request->cdb_len = sizeof(*cmd);
		} else {
			scsi_cmd_rw_10 *cmd = (scsi_cmd_rw_10 *)request->cdb;

			is_rw10 = true;

			memset(cmd, 0, sizeof(*cmd));
			cmd->opcode = write ? SCSI_OP_WRITE_10 : SCSI_OP_READ_10;
			cmd->RelAdr = 0;
			cmd->FUA = 0;
			cmd->DPO = 0;

			cmd->top_LBA = (pos >> 24) & 0xff;
			cmd->high_LBA = (pos >> 16) & 0xff;
			cmd->mid_LBA = (pos >> 8) & 0xff;
			cmd->low_LBA = pos & 0xff;

			cmd->high_length = (num_blocks >> 8) & 0xff;
			cmd->low_length = num_blocks & 0xff;

			request->cdb_len = sizeof(*cmd);
		}

		// last chance to detect errors that occured during concurrent accesses
		err = handle->pending_error;

		if (err)
			goto abort;

		device->scsi->async_io(request);

		acquire_sem(request->completion_sem);

		// ask generic peripheral layer what to do now	
		res = periph_check_error(device, request);

		switch (res.action) {
			case err_act_ok:
				*bytes_transferred = num_bytes - request->data_resid;
				break;

			case err_act_start:
				res = periph_send_start_stop(device, request, 1, device->removable);
				if (res.action == err_act_ok)
					res.action = err_act_retry;
				break;

			case err_act_invalid_req:
				// if this was a 10 byte command, the device probably doesn't 
				// support them, so disable them and retry
				if (is_rw10) {
					atomic_and(&device->rw10_enabled, 0);
					res.action = err_act_retry;
				} else
					res.action = err_act_fail;
				break;
		}
	} while((res.action == err_act_retry && retries++ < 3)
		|| (res.action == err_act_many_retries && retries++ < 30));

	device->scsi->free_ccb(request);

	// peripheral layer only created "read" error, so we have to
	// map them to "write" errors if this was a write request
	if (res.error_code == B_DEV_READ_ERROR && write)
		return B_DEV_WRITE_ERROR;

	return res.error_code;	

abort:
	device->scsi->free_ccb(request);
	return err;
}


static status_t
inquiry(scsi_periph_device_info *device, scsi_inquiry *inquiry)
{
	scsi_res_inquiry *device_inquiry = NULL;
	size_t inquiry_len;

	if (pnp->get_attr_raw(device->node, SCSI_DEVICE_INQUIRY_ITEM, 
			(void **)&device_inquiry, &inquiry_len, true) != B_OK)
		return B_ERROR;

	memcpy(inquiry, device_inquiry, min(inquiry_len, sizeof(scsi_inquiry)));
	free(device_inquiry);

	return B_OK;
}


static status_t
prevent_allow(scsi_periph_device_info *device, bool prevent)
{
	scsi_cmd_prevent_allow cmd;

	SHOW_FLOW0(0, "");

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = SCSI_OP_PREVENT_ALLOW;
	cmd.prevent = prevent;

	return periph_simple_exec(device, (uint8 *)&cmd, sizeof(cmd), NULL, 0, 0);
}


/** !!!keep this in sync with scsi_raw driver!!! */

static status_t
raw_command(scsi_periph_device_info *device, raw_device_command *cmd)
{
	scsi_ccb *request;

	SHOW_FLOW0(0, "");

	request = device->scsi->alloc_ccb(device->scsi_device);
	if (request == NULL)
		return B_NO_MEMORY;

	request->flags = 0;

	if (cmd->flags & B_RAW_DEVICE_DATA_IN)
		request->flags |= SCSI_DIR_IN;
	else
		request->flags |= SCSI_DIR_OUT;

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
			min(cmd->sense_data_length, (size_t)SCSI_MAX_SENSE_SIZE - request->sense_resid));
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


status_t
periph_ioctl(scsi_periph_handle_info *handle, int op, void *buffer, size_t length)
{
	switch (op) {
		case B_GET_MEDIA_STATUS: {
			status_t res = B_OK;

			if (handle->device->removable)
				res = periph_get_media_status(handle);

			SHOW_FLOW(2, "%s", strerror(res));

			*(status_t *)buffer = res;
			return B_OK;
		}

		case B_SCSI_INQUIRY:
			return inquiry(handle->device, (scsi_inquiry *)buffer);

		case B_SCSI_PREVENT_ALLOW:
			return prevent_allow(handle->device, *(bool *)buffer);

		case B_RAW_DEVICE_COMMAND:
			return raw_command(handle->device, buffer);

		default:
			if (handle->device->scsi->ioctl != NULL) {
				return handle->device->scsi->ioctl(handle->device->scsi_device,
					op, buffer, length);
			}

			SHOW_ERROR(4, "Unknown ioctl: %x", op);
			return B_BAD_VALUE;
	}
}


/**	kernel daemon
 *	once in a minute, it sets a flag so that the next command is executed
 *	ordered; this way, we avoid starvation of SCSI commands inside the
 *	SCSI queuing system - the ordered command waits for all previous
 *	commands and thus no command can starve longer then a minute
 */

void
periph_sync_queue_daemon(void *arg, int iteration)
{
	scsi_periph_device_info *device = (scsi_periph_device_info *)arg;

	SHOW_FLOW0(3, "Setting ordered flag for next R/W access");
	atomic_or(&device->next_tag_action, SCSI_ORDERED_QTAG);
}
