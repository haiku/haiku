/*
** Copyright 2003, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open SCSI Raw Driver
*/

#ifndef _SCSI_RAW_H
#define _SCSI_RAW_H

#include <device_manager.h>
#include <bus/SCSI.h>

#define debug_level_flow 0
#define debug_level_error 3
#define debug_level_info 0

#define DEBUG_MSG_PREFIX "SCSI_RAW -- "

#include "wrapper.h"


typedef struct raw_device_info {
	pnp_node_handle node;
	scsi_device scsi_device;
	scsi_device_interface *scsi;
} raw_device_info;


#define SCSI_RAW_MODULE_NAME "drivers/bus/scsi/scsi_raw/"SCSI_DEVICE_TYPE_NAME

#endif
