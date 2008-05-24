/*
 * Copyright 2008, Marcus Overhagen. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "sata_request.h"
#include "scsi_cmds.h"
#include <string.h>

sata_request::sata_request()
 :	fCcb(NULL)
 ,	fIsATAPI(false)
 ,	fCompletionSem(create_sem(0, "sata completion"))
 ,	fCompletionStatus(0)
 ,	fData(NULL)
 ,	fDataSize(0)
{
}


sata_request::sata_request(scsi_ccb *ccb)
 :	fCcb(ccb)
 ,	fIsATAPI(false)
 ,	fCompletionSem(-1)
 ,	fCompletionStatus(0)
 ,	fData(NULL)
 ,	fDataSize(0)
{
}


sata_request::~sata_request()
{
	if (fCompletionSem >= 0)
		delete_sem(fCompletionSem);
}


void
sata_request::set_data(void *data, size_t dataSize)
{
	if (fCcb) panic("wrong usage");
 	fData = data;
 	fDataSize = dataSize;
}


void
sata_request::set_ata_cmd(uint8 command)
{
	memset(fFis, 0, sizeof(fFis));
	fFis[0] = 0x27;
	fFis[1] = 0x80;
	fFis[2] = command;
}


void
sata_request::set_ata28_cmd(uint8 command, uint32 lba, uint8 sectorCount)
{
	set_ata_cmd(command);
	fFis[4] = lba & 0xff;
	fFis[5] = (lba >> 8) & 0xff;
	fFis[6] = (lba >> 16) & 0xff;
	fFis[7] = 0x40 | ((lba >> 24) & 0x0f);
	fFis[12] = sectorCount & 0xff;
}


void 
sata_request::set_ata48_cmd(uint8 command, uint64 lba, uint16 sectorCount)
{
	set_ata_cmd(command);
	fFis[4] = lba & 0xff;
	fFis[5] = (lba >> 8) & 0xff;
	fFis[6] = (lba >> 16) & 0xff;
	fFis[7] = 0x40;
	fFis[8] = (lba >> 24) & 0xff;
	fFis[9] = (lba >> 32) & 0xff;
	fFis[10] = (lba >> 40) & 0xff;
	fFis[12] = sectorCount & 0xff;
	fFis[13] = (sectorCount >> 8) & 0xff;
}


void
sata_request::set_atapi_cmd()
{
	fIsATAPI = true;
	set_ata_cmd(0xa0);
	fFis[5] = 0xfe;
	fFis[6] = 0xff;
}


void
sata_request::finish(int tfd, size_t bytesTransfered)
{
	if (tfd & ATA_ERR)
		dprintf("ahci: sata_request::finish ATA_ERR set for command 0x%02x\n", fFis[2]);
	if (fCcb) {
		fCcb->data_resid = fCcb->data_length - bytesTransfered;
		fCcb->subsys_status = (tfd & ATA_ERR) ? SCSI_REQ_CMP_ERR : SCSI_REQ_CMP;
		if (fIsATAPI && (tfd & ATA_ERR)) {
			uint8 error = (tfd >> 8) & 0xff;
			dprintf("ahci: sata_request::finish status 0x%02x, error 0x%02x\n", tfd & 0xff, error);
			if (error & 0x04) { // ABRT
				fCcb->subsys_status = SCSI_REQ_ABORTED;
			} else {
				fCcb->device_status = SCSI_STATUS_CHECK_CONDITION;
				fCcb->subsys_status |= SCSI_AUTOSNS_VALID;
				fCcb->sense_resid = 0; //FIXME
				scsi_sense *sense = (scsi_sense *)fCcb->sense;
				sense->error_code = SCSIS_CURR_ERROR;
				sense->sense_key = error >> 4;
				sense->asc = 0;
				sense->ascq = 0;
			}
		}
		gSCSI->finished(fCcb, 1);
		delete this;
	} else {
		fCompletionStatus = tfd;
		release_sem(fCompletionSem);
	}
}


void
sata_request::abort()
{
	dprintf("ahci: sata_request::abort called for command 0x%02x\n", fFis[2]);
	if (fCcb) {
		fCcb->subsys_status = SCSI_REQ_ABORTED;
		gSCSI->finished(fCcb, 1);
		delete this;
	} else {
		fCompletionStatus = -1;
		release_sem(fCompletionSem);
	}
}


void
sata_request::wait_for_completition()
{
	if (fCcb) panic("wrong usage");
	acquire_sem(fCompletionSem);
}


int
sata_request::completition_status()
{
	if (fCcb) panic("wrong usage");
	return fCompletionStatus;
}






