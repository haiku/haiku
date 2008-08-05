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

#include "dma_resources.h"
#include "io_requests.h"
#include "IOScheduler.h"


#define SCSI_DISK_DRIVER_MODULE_NAME "drivers/disk/scsi/scsi_disk/driver_v1"
#define SCSI_DISK_DEVICE_MODULE_NAME "drivers/disk/scsi/scsi_disk/device_v1"


struct das_driver_info {
	device_node*			node;
	::scsi_periph_device	scsi_periph_device;
	::scsi_device			scsi_device;
	scsi_device_interface*	scsi;
	IOScheduler*			io_scheduler;
	DMAResource*			dma_resource;

	uint64					capacity;
	uint32					block_size;

	bool					removable;
};

struct das_handle {
	::scsi_periph_handle	scsi_periph_handle;
	das_driver_info*		info;
};

#endif	/* _SCSI_DISK_H */
