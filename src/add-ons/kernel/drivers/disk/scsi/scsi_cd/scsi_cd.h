/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2003, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SCSI_CD_H
#define _SCSI_CD_H


#include <block_io.h>
#include <device_manager.h>
#include <scsi_periph.h>
#include <scsi.h>


#define SCSI_CD_MODULE_NAME "drivers/disk/scsi/scsi_cd/driver_v1"


// must start as block_device_cookie
typedef struct cd_device_info {
	device_node *node;
	::scsi_periph_device scsi_periph_device;
	::scsi_device scsi_device;
	scsi_device_interface *scsi;
	::block_io_device block_io_device;

	uint64 capacity;
	uint32 block_size;

	bool removable;
	uint8 device_type;
} cd_device_info;
	
typedef struct cd_handle_info {
	::scsi_periph_handle scsi_periph_handle;
	cd_device_info *device;
} cd_handle_info;

#endif	// _SCSI_CD_H
