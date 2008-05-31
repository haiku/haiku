/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SCSI_DISK_H
#define _SCSI_DISK_H


#include <block_io.h>
#include <device_manager.h>
#include <scsi.h>
#include <scsi_periph.h>


#define SCSI_DISK_MODULE_NAME "drivers/disk/scsi/scsi_dsk/driver_v1"


// must start as block_device_cookie
typedef struct das_device_info {
	device_node *node;
	::scsi_periph_device scsi_periph_device;
	::scsi_device scsi_device;
	scsi_device_interface *scsi;
	::block_io_device block_io_device;

	uint64 capacity;
	uint32 block_size;

	bool removable;			// true, if device is removable
} das_device_info;

typedef struct das_handle_info {
	::scsi_periph_handle scsi_periph_handle;
	das_device_info *device;
} das_handle_info;

#endif	/* _SCSI_DISK_H */
