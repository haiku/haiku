/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open SCSI Disk Driver

	File handle management.
*/

#include "scsi_cd_int.h"

#include <malloc.h>

status_t cd_open( cd_device_info *device, cd_handle_info **handle_out )
{
	cd_handle_info *handle;
	int res;
	
//	SHOW_FLOW0( 3, "" );
	
	handle = (cd_handle_info *)malloc( sizeof( *handle ));
	if( handle == NULL )
		return B_NO_MEMORY;
		
	handle->device = device;
		
	res = scsi_periph->handle_open( device->scsi_periph_device, 
		(periph_handle_cookie)handle, 
		&handle->scsi_periph_handle );
	if( res < 0 ) {
		free( handle );
		return res;
	}

	if( device->block_size == 0 || device->capacity == 0 ) {	
		scsi_ccb *request;
		
		request = device->scsi->alloc_ccb( device->scsi_device );
		
		// don't care if no test was possible
		if( request != NULL ) {
			scsi_periph->check_capacity( device->scsi_periph_device, request );
	
			device->scsi->free_ccb( request );
		}
	}
	
//	SHOW_FLOW0( 3, "opened" );

	*handle_out = handle;	
	return B_OK;
}

status_t cd_close( cd_handle_info *handle )
{
//	SHOW_FLOW0( 3, "" );
	
	scsi_periph->handle_close( handle->scsi_periph_handle );
	
//	SHOW_FLOW0( 3, "done" );
	
	return B_OK;
}

status_t cd_free( cd_handle_info *handle )
{
//	SHOW_FLOW0( 3, "" );
	
	scsi_periph->handle_free( handle->scsi_periph_handle );
	free( handle );
	
//	SHOW_FLOW0( 3, "done" );
	
	return B_OK;
}
