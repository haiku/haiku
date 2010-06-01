/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2003, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SCSI_CD_H
#define _SCSI_CD_H


#include <device_manager.h>
#include <scsi_periph.h>
#include <scsi.h>

#include "dma_resources.h"
#include "IORequest.h"


struct IOCache;
struct IOScheduler;


#define SCSI_CD_DRIVER_MODULE_NAME "drivers/disk/scsi/scsi_cd/driver_v1"
#define SCSI_CD_DEVICE_MODULE_NAME "drivers/disk/scsi/scsi_cd/device_v1"


struct cd_driver_info {
	device_node*			node;
	::scsi_periph_device	scsi_periph_device;
	::scsi_device			scsi_device;
	scsi_device_interface*	scsi;
	IOScheduler*			io_scheduler;
	DMAResource*			dma_resource;

	uint64					capacity;
	uint64					original_capacity;
	uint32					block_size;

	bool					removable;
	uint8					device_type;
};

struct cd_handle {
	::scsi_periph_handle	scsi_periph_handle;
	cd_driver_info*			info;
};

#endif	// _SCSI_CD_H
