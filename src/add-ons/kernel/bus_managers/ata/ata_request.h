/*
 * Copyright 2008 Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ATA_REQUEST_H
#define _ATA_REQUEST_H

#include <SupportDefs.h>

typedef struct ata_request {

	struct ide_device_info *	device;
	struct scsi_ccb *			ccb;				// basic scsi request
	uint8 						is_write : 1;		// true for write request
	uint8 						uses_dma : 1;		// true if using dma
	uint8 						packet_irq : 1;		// true if waiting for command packet irq

	uint8						senseKey;
	uint8						senseAsc;
	uint8						senseAscq;

	bigtime_t					timeout;
} ata_request;

struct scsi_ccb;
struct ide_device_info;

void ata_request_init(ata_request *request, struct ide_device_info *device);
void ata_request_start(ata_request **_request, struct ide_device_info *device, struct scsi_ccb *ccb);
void ata_request_clear_sense(ata_request *request);
void ata_request_set_status(ata_request *request, uint8 status);
void ata_request_set_sense(ata_request *request, uint8 key, uint16 asc_acq);
void ata_request_finish(ata_request *request, bool resubmit);

#endif
