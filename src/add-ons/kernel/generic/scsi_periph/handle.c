/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open SCSI Peripheral Driver

	Basic handling of file handles.
*/


#include "scsi_periph_int.h"

#include <dl_list.h>
#include <malloc.h>


status_t
periph_handle_open(scsi_periph_device_info *device, periph_handle_cookie periph_handle,
	scsi_periph_handle_info **res_handle)
{
	scsi_periph_handle_info *handle;

//	SHOW_FLOW( 3, "device=%p", device );

	handle = (scsi_periph_handle_info *)malloc(sizeof(*handle));
	if (handle == NULL)
		return B_NO_MEMORY;

	handle->periph_handle = periph_handle;
	handle->device = device;
	handle->pending_error = B_OK;

	ACQUIRE_BEN(&device->mutex);
	ADD_DL_LIST_HEAD(handle, device->handles, );
	RELEASE_BEN(&device->mutex);

	*res_handle = handle;

//	SHOW_FLOW( 3, "handle=%p", handle );

	return B_OK;
}


status_t
periph_handle_close(scsi_periph_handle_info *handle)
{
//	SHOW_FLOW( 3, "handle=%p", handle );
	return B_OK;
}


status_t
periph_handle_free(scsi_periph_handle_info *handle)
{
	scsi_periph_device_info *device = handle->device;

//	SHOW_FLOW( 3, "handle=%p, device=%p", handle, handle->device );

	ACQUIRE_BEN(&device->mutex);
	REMOVE_DL_LIST(handle, device->handles, );
	RELEASE_BEN(&device->mutex);

	free(handle);

//	SHOW_FLOW0( 3, "done" );

	return B_OK;
}
