/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open SCSI Peripheral Driver

	Handling of block device (currently, only a capacity check is provided)
*/


#include "scsi_periph_int.h"
#include <string.h>


status_t
periph_check_capacity(scsi_periph_device_info *device, scsi_ccb *request)
{
	scsi_res_read_capacity capacity_res;
	scsi_cmd_read_capacity *cmd = (scsi_cmd_read_capacity *)request->cdb;
	uint64 capacity;
	uint32 block_size;
	status_t res;

	SHOW_FLOW(3, "%p, %p", device, request);

	// driver doesn't support capacity callback - seems to be no block
	// device driver, so ignore	
	if (device->callbacks->set_capacity == NULL)
		return B_OK;

	request->flags = SCSI_DIR_IN;

	request->data = (char *)&capacity_res;
	request->data_len = sizeof(capacity_res);
	request->cdb_len = sizeof(scsi_cmd_read_capacity);
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

	ACQUIRE_BEN(&device->mutex);

	if (res == B_OK && request->data_resid == 0) {
		capacity = (capacity_res.top_LBA << 24)
			| (capacity_res.high_LBA << 16)
			| (capacity_res.mid_LBA << 8)
			| capacity_res.low_LBA;

		// the command returns the index of the _last_ block,
		// i.e. the size is one larger
		++capacity;

		block_size = (capacity_res.top_block_size << 24)
			| (capacity_res.high_block_size << 16)
			| (capacity_res.mid_block_size << 8)
			| capacity_res.low_block_size;
	} else {
		capacity = 0;
		block_size = 0;
	}

	SHOW_FLOW(3, "capacity = %Ld, block_size = %ld", capacity, block_size);

	device->callbacks->set_capacity(device->periph_device, 
		capacity, block_size);

/*	device->byte2blk_shift = log2( device->block_size );
	if( device->byte2blk_shift < 0 ) {
		// this may be too restrictive...
		device->capacity = -1;
		return ERR_DEV_GENERAL;
	}*/

	RELEASE_BEN(&device->mutex);

	SHOW_FLOW(3, "done (%s)", strerror(res));

	return res;
}

