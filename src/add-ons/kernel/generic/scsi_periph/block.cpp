/*
 * Copyright 2004-2013, Haiku, Inc. All RightsReserved.
 * Copyright 2002-2003, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


//!	Handling of block device


#include "scsi_periph_int.h"
#include <string.h>


status_t
periph_check_capacity(scsi_periph_device_info *device, scsi_ccb *request)
{
	scsi_res_read_capacity capacityResult;
	scsi_cmd_read_capacity *cmd = (scsi_cmd_read_capacity *)request->cdb;
	uint64 capacity;
	uint32 blockSize;
	status_t res;

	SHOW_FLOW(3, "%p, %p", device, request);

	// driver doesn't support capacity callback - seems to be no block
	// device driver, so ignore
	if (device->callbacks->set_capacity == NULL)
		return B_OK;

	request->flags = SCSI_DIR_IN;

	request->data = (uint8*)&capacityResult;
	request->data_length = sizeof(capacityResult);
	request->cdb_length = sizeof(scsi_cmd_read_capacity);
	request->timeout = device->std_timeout;
	request->sort = -1;
	request->sg_list = NULL;

	memset(cmd, 0, sizeof(*cmd));
	cmd->opcode = SCSI_OP_READ_CAPACITY;
	// we don't set PMI (partial medium indicator) as we want the whole capacity;
	// in this case, all other parameters must be zero

	res = periph_safe_exec(device, request);

	if (res == B_DEV_MEDIA_CHANGED) {
		// in this case, the error handler has already called check_capacity
		// recursively, so we ignore our (invalid) result
		SHOW_FLOW0( 3, "ignore result because medium change" );
		return B_DEV_MEDIA_CHANGED;
	}

	mutex_lock(&device->mutex);

	if (res == B_OK && request->data_resid == 0) {
		capacity = B_BENDIAN_TO_HOST_INT32(capacityResult.lba);

		if (capacity == UINT_MAX) {
			mutex_unlock(&device->mutex);

			scsi_cmd_read_capacity_long *cmd
				= (scsi_cmd_read_capacity_long *)request->cdb;

			scsi_res_read_capacity_long capacityLongResult;
			request->data = (uint8*)&capacityLongResult;
			request->data_length = sizeof(capacityLongResult);
			request->cdb_length = sizeof(scsi_cmd_read_capacity_long);

			memset(cmd, 0, sizeof(*cmd));
			cmd->opcode = SCSI_OP_SERVICE_ACTION_IN;
			cmd->service_action = SCSI_SAI_READ_CAPACITY_16;

			res = periph_safe_exec(device, request);

			mutex_lock(&device->mutex);

			if (res == B_OK && request->data_resid == 0) {
				capacity = B_BENDIAN_TO_HOST_INT64(capacityLongResult.lba);
			} else
				capacity = 0;
		}

		// the command returns the index of the _last_ block,
		// i.e. the size is one larger
		++capacity;

		blockSize = B_BENDIAN_TO_HOST_INT32(capacityResult.block_size);
	} else {
		capacity = 0;
		blockSize = 0;
	}

	SHOW_FLOW(3, "capacity = %" B_PRId64 ", block_size = %" B_PRId32, capacity,
		blockSize);

	device->block_size = blockSize;

	device->callbacks->set_capacity(device->periph_device,
		capacity, blockSize);

/*	device->byte2blk_shift = log2( device->block_size );
	if( device->byte2blk_shift < 0 ) {
		// this may be too restrictive...
		device->capacity = -1;
		return ERR_DEV_GENERAL;
	}*/

	mutex_unlock(&device->mutex);

	SHOW_FLOW(3, "done (%s)", strerror(res));

	return res;
}


status_t
periph_trim_device(scsi_periph_device_info *device, scsi_ccb *request,
	uint64 offset, uint64 numBlocks)
{
	err_res res;
	int retries = 0;

	do {
		request->flags = SCSI_DIR_OUT;
		request->sort = offset;
		request->timeout = device->std_timeout;

		scsi_cmd_wsame_16* cmd = (scsi_cmd_wsame_16*)request->cdb;

		memset(cmd, 0, sizeof(*cmd));
		cmd->opcode = SCSI_OP_WRITE_SAME_16;
		cmd->unmap = 1;
		cmd->lba = B_HOST_TO_BENDIAN_INT64(offset);
		cmd->length = B_HOST_TO_BENDIAN_INT32(numBlocks);

		request->cdb_length = sizeof(*cmd);

		device->scsi->async_io(request);

		acquire_sem(request->completion_sem);

		// ask generic peripheral layer what to do now
		res = periph_check_error(device, request);
	} while ((res.action == err_act_retry && retries++ < 3)
		|| (res.action == err_act_many_retries && retries++ < 30));

	// peripheral layer only creates "read" error
	if (res.error_code == B_DEV_READ_ERROR)
		return B_DEV_WRITE_ERROR;

	return res.error_code;
}

