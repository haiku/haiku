/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open IDE CD-ROM driver
*/

#include <device_manager.h>
#include "scsi_cd.h"
#include <bus/scsi/scsi_periph.h>
#include <blkman.h>

#define debug_level_flow 0
#define debug_level_error 3
#define debug_level_info 3

#define DEBUG_MSG_PREFIX "SCSI_CD -- "

#include "wrapper.h"


typedef struct cd_device_info {
	pnp_node_handle node;
	scsi_periph_device scsi_periph_device;
	scsi_device scsi_device;
	scsi_device_interface *scsi;
	blkman_device blkman_device;
	
	uint64 capacity;
	uint32 block_size;
	
	bool removable;
	uint8 device_type;
} cd_device_info;
	
typedef struct cd_handle_info {
	scsi_periph_handle scsi_periph_handle;
	cd_device_info *device;
} cd_handle_info;

extern scsi_periph_interface *scsi_periph;
extern device_manager_info *pnp;
extern scsi_periph_callbacks callbacks;
extern blkman_for_driver_interface *blkman;


// device_mgr.c

status_t cd_init_device( pnp_node_handle node, void *user_cookie, void **cookie );
status_t cd_uninit_device( cd_device_info *device );
status_t cd_device_added( pnp_node_handle node );


// handle_mgr.c

status_t cd_open( cd_device_info *device, cd_handle_info **handle_out );
status_t cd_close( cd_handle_info *handle );
status_t cd_free( cd_handle_info *handle );
