/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open SCSI Peripheral Driver

	Main file.
*/

#include "scsi_periph_int.h"

#include <module.h>

#include <string.h>


device_manager_info *pnp;


status_t
periph_simple_exec(scsi_periph_device_info *device, void *cdb, uchar cdb_len,
	void *data, size_t data_len, int ccb_flags)
{
	scsi_ccb *ccb;
	status_t res;

	SHOW_FLOW0( 0, "" );

	ccb = device->scsi->alloc_ccb(device->scsi_device);
	if (ccb == NULL)
		return B_NO_MEMORY;

	ccb->flags = ccb_flags;

	memcpy(ccb->cdb, cdb, cdb_len);
	ccb->cdb_len = cdb_len;

	ccb->sort = -1;
	ccb->timeout = device->std_timeout;

	ccb->data = data;
	ccb->sg_list = NULL;
	ccb->data_len = data_len;

	res = periph_safe_exec(device, ccb);

	device->scsi->free_ccb(ccb);

	return res;
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
			uint16 backup_sg_cnt;
			uint32 backup_data_len;

			backup_flags = request->flags;
			memcpy(backup_cdb, request->cdb, SCSI_MAX_CDB_SIZE);
			backup_cdb_len = request->cdb_len;
			backup_sort = request->sort;
			backup_timeout = request->timeout;
			backup_data = request->data;
			backup_sg_list = request->sg_list;
			backup_sg_cnt = request->sg_cnt;
			backup_data_len = request->data_len;

			SHOW_INFO0( 2, "Sending start to init LUN" );

			res = periph_send_start_stop(device, request, 1, device->removable);

			request->flags = backup_flags;
			memcpy(request->cdb, backup_cdb, SCSI_MAX_CDB_SIZE);
			request->cdb_len = backup_cdb_len;
			request->sort = backup_sort;
			request->timeout = backup_timeout;
			request->data = backup_data;
			request->sg_list = backup_sg_list;
			request->sg_cnt = backup_sg_cnt;
			request->data_len = backup_data_len;

			if (res.action == err_act_ok)
				res.action = err_act_retry;
		}
	} while ((res.action == err_act_retry && retries++ < 3)
		|| (res.action == err_act_many_retries && retries++ < 30));

	return res.error_code;
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
	{ DEVICE_MANAGER_MODULE_NAME, (module_info **)&pnp },
	{}
};


scsi_periph_interface scsi_periph_module = {
	{
		SCSI_PERIPH_MODULE_NAME,
		0,
		std_ops
	},
	
	periph_register_device,
	periph_unregister_device,
	
	periph_safe_exec,
	periph_simple_exec,

	periph_handle_open,
	periph_handle_close,
	periph_handle_free,
	
	periph_read,
	periph_write,
	periph_ioctl,
	periph_check_capacity,

	periph_media_changed_public,
	periph_check_error,
	periph_send_start_stop,
	periph_get_media_status,
	
	periph_compose_device_name,
	periph_get_icon
};


#if !_BUILDING_kernel && !BOOT
_EXPORT 
scsi_periph_interface *modules[] = {
	&scsi_periph_module, 
	NULL
};
#endif
