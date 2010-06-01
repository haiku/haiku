/*
 * Copyright 2004-2008, Haiku, Inc. All RightsReserved.
 * Copyright 2002-2003, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

//!	Main file.


#include "scsi_periph_int.h"

#include <module.h>

#include <string.h>


device_manager_info* gDeviceManager;


status_t
periph_simple_exec(scsi_periph_device_info* device, void* cdb, uchar cdbLength,
	void* data, size_t dataLength, int ccb_flags)
{
	SHOW_FLOW0( 0, "" );

	scsi_ccb* ccb = device->scsi->alloc_ccb(device->scsi_device);
	if (ccb == NULL)
		return B_NO_MEMORY;

	ccb->flags = ccb_flags;

	memcpy(ccb->cdb, cdb, cdbLength);
	ccb->cdb_length = cdbLength;

	ccb->sort = -1;
	ccb->timeout = device->std_timeout;

	ccb->data = (uint8*)data;
	ccb->sg_list = NULL;
	ccb->data_length = dataLength;

	status_t status = periph_safe_exec(device, ccb);

	device->scsi->free_ccb(ccb);

	return status;
}


status_t
periph_safe_exec(scsi_periph_device_info *device, scsi_ccb *request)
{
	err_res res;
	int retries = 0;

	do {
		device->scsi->sync_io(request);

		// ask generic peripheral layer what to do now
		res = periph_check_error(device, request);
		if (res.action == err_act_start) {
			// backup request, as we need it temporarily for sending "start"
			// (we cannot allocate a new cdb as there may be no more cdb and
			//  waiting for one to become empty may lead to deadlock if everyone
			//  does that)
			uint32 backup_flags;
			uint8 backup_cdb[SCSI_MAX_CDB_SIZE];
			uchar backup_cdb_len;
			int64 backup_sort;
			bigtime_t backup_timeout;
			uchar *backup_data;
			const physical_entry *backup_sg_list;
			uint16 backup_sg_count;
			uint32 backup_data_len;

			backup_flags = request->flags;
			memcpy(backup_cdb, request->cdb, SCSI_MAX_CDB_SIZE);
			backup_cdb_len = request->cdb_length;
			backup_sort = request->sort;
			backup_timeout = request->timeout;
			backup_data = request->data;
			backup_sg_list = request->sg_list;
			backup_sg_count = request->sg_count;
			backup_data_len = request->data_length;

			SHOW_INFO0( 2, "Sending start to init LUN" );

			res = periph_send_start_stop(device, request, 1, device->removable);

			request->flags = backup_flags;
			memcpy(request->cdb, backup_cdb, SCSI_MAX_CDB_SIZE);
			request->cdb_length = backup_cdb_len;
			request->sort = backup_sort;
			request->timeout = backup_timeout;
			request->data = backup_data;
			request->sg_list = backup_sg_list;
			request->sg_count = backup_sg_count;
			request->data_length = backup_data_len;

			if (res.action == err_act_ok)
				res.action = err_act_retry;
		}
	} while ((res.action == err_act_retry && retries++ < 3)
		|| (res.action == err_act_many_retries && retries++ < 30));

	return res.error_code;
}


module_dependency module_dependencies[] = {
	{B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager},
	{}
};


static scsi_periph_interface sSCSIPeripheralModule = {
	{
		SCSI_PERIPH_MODULE_NAME,
		0,
		NULL
	},

	periph_register_device,
	periph_unregister_device,

	periph_safe_exec,
	periph_simple_exec,

	periph_handle_open,
	periph_handle_close,
	periph_handle_free,

	periph_read_write,
	periph_io,
	periph_ioctl,
	periph_check_capacity,

	periph_media_changed_public,
	periph_check_error,
	periph_send_start_stop,
	periph_get_media_status,
	periph_synchronize_cache,

	periph_compose_device_name
};

scsi_periph_interface *modules[] = {
	&sSCSIPeripheralModule,
	NULL
};
