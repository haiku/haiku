/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open SCSI Disk Driver

	File handle management.
*/

#include "scsi_dsk_int.h"

#include <malloc.h>

status_t das_open( das_device_info *device, das_handle_info **handle_out )
{
	das_handle_info *handle;
	int res;
	
	SHOW_FLOW0( 3, "" );
	
	handle = (das_handle_info *)malloc( sizeof( *handle ));
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
	
	SHOW_FLOW0( 3, "opened" );

	*handle_out = handle;	
	return B_OK;
}

status_t das_close( das_handle_info *handle )
{
	SHOW_FLOW0( 3, "" );
	
	scsi_periph->handle_close( handle->scsi_periph_handle );
	
	SHOW_FLOW0( 3, "done" );
	
	return B_OK;
}

status_t das_free( das_handle_info *handle )
{
	SHOW_FLOW0( 3, "" );
	
	scsi_periph->handle_free( handle->scsi_periph_handle );
	free( handle );
	
	SHOW_FLOW0( 3, "done" );
	
	return B_OK;
}
