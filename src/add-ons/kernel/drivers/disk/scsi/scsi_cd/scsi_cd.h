/*
** Copyright 2003, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open SCSI CD Driver

	Peripheral driver to handle CD-ROM drives. To be more
	precisely, it supports CD-ROM and WORM drives (well -
	I've never _seen_ a WORM driver). 
	
	Much work is done by scsi_periph and blkdev.
*/

#ifndef _SCSI_CD_H
#define _SCSI_CD_H

#include <device_manager.h>

#define SCSI_CD_MODULE_NAME "drivers/disk/scsi/scsi_cd/"SCSI_DEVICE_TYPE_NAME

#endif
