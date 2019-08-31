/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */


#include "VirtioSCSIPrivate.h"

#include <strings.h>


VirtioSCSIRequest::VirtioSCSIRequest(bool hasLock)
	:
	fHasLock(hasLock),
	fStatus(SCSI_REQ_CMP),
	fTimeout(0),
	fBytesLeft(0),
	fIsWrite(false),
	fCCB(NULL)
{
	if (hasLock)
		mutex_init(&fLock, "virtio scsi request");

	fBuffer = malloc(sizeof(struct virtio_scsi_cmd_req)
		+ sizeof(struct virtio_scsi_cmd_resp));
	bzero(fBuffer, sizeof(struct virtio_scsi_cmd_req)
		+ sizeof(struct virtio_scsi_cmd_resp));

	fRequest = (struct virtio_scsi_cmd_req *)fBuffer;
	fResponse = (struct virtio_scsi_cmd_resp *)
		((addr_t)fBuffer + sizeof(struct virtio_scsi_cmd_req));

	fResponse->sense_len = 0;
}


VirtioSCSIRequest::~VirtioSCSIRequest()
{
	if (fHasLock)
		mutex_destroy(&fLock);

	free(fBuffer);
}


void
VirtioSCSIRequest::SetStatus(uint8 status)
{
	fStatus = status;
}


void
VirtioSCSIRequest::SetTimeout(bigtime_t timeout)
{
	fTimeout = timeout;
}


void
VirtioSCSIRequest::SetIsWrite(bool isWrite)
{
	fIsWrite = isWrite;
}


void
VirtioSCSIRequest::SetBytesLeft(uint32 bytesLeft)
{
	fBytesLeft = bytesLeft;
}


status_t
VirtioSCSIRequest::Start(scsi_ccb *ccb)
{
	CALLED();
	if (mutex_trylock(&fLock) != B_OK)
		return B_BUSY;

	fCCB = ccb;
	fStatus = SCSI_REQ_CMP;
	fCCB->device_status = SCSI_STATUS_GOOD;
	fIsWrite = false;
	bzero(fResponse, sizeof(struct virtio_scsi_cmd_resp));

	TRACE("VirtioSCSIRequest::Start() opcode %x tid %x lun %x\n", ccb->cdb[0],
		ccb->target_id, ccb->target_lun);

	return B_OK;
}


status_t
VirtioSCSIRequest::Finish(bool resubmit)
{
	CALLED();
	fStatus = _ResponseStatus();
	fCCB->data_resid = fResponse->resid;
	fCCB->subsys_status = fStatus;

	TRACE("VirtioSCSIRequest::Finish() status 0x%x response 0x%x resid:0x%x"
		" sense_len:%x\n", fResponse->status, fResponse->response,
		fResponse->resid, fResponse->sense_len);

	if (fCCB->cdb[0] == SCSI_OP_INQUIRY) {
		// when the request is an inquiry, don't do anything
	} else if (fStatus == SCSI_REQ_CMP && fResponse->status != 0
		&& HasSense()) {
		// when the request completed and has set sense
		// data, report this to the scsi stack by setting
		// CHECK CONDITION status
		TRACE("setting check condition\n");

		fCCB->subsys_status = SCSI_REQ_CMP_ERR;
		fCCB->device_status = SCSI_STATUS_CHECK_CONDITION;

		// copy sense data if caller requested it
		if ((fCCB->flags & SCSI_DIS_AUTOSENSE) == 0) {
			size_t senseLength = min_c(sizeof(fCCB->sense),
				fResponse->sense_len);
			memcpy(fCCB->sense, fResponse->sense, senseLength);
			fCCB->sense_resid = sizeof(fCCB->sense) - senseLength;
			fCCB->subsys_status |= SCSI_AUTOSNS_VALID;
		}
	}

	scsi_ccb *ccb = fCCB;
	mutex_unlock(&fLock);

	if (resubmit)
		gSCSI->resubmit(ccb);
	else
		gSCSI->finished(ccb, 1);

	TRACE("VirtioSCSIRequest::Finish() done\n");

	return B_OK;
}


void
VirtioSCSIRequest::Abort()
{
	scsi_ccb *ccb = fCCB;
	mutex_unlock(&fLock);

	ccb->subsys_status = SCSI_REQ_ABORTED;
	gSCSI->finished(ccb, 1);
}


void
VirtioSCSIRequest::RequestSense()
{
	CALLED();
	// Copy sense data from last request into data buffer of current request.
	// The sense data of last request is still present in the current request,
	// as it isn't cleared on SCSI_OP_REQUEST_SENSE.
	scsi_cmd_request_sense *command = (scsi_cmd_request_sense *)fCCB->cdb;
	copy_sg_data(fCCB, 0, command->allocation_length, fResponse->sense,
		fResponse->sense_len, false);

	fCCB->data_resid = fCCB->data_length - min_c(min_c(fResponse->sense_len,
		command->allocation_length), fCCB->data_length);
	fResponse->sense_len = 0;
}


void
VirtioSCSIRequest::FillRequest(uint32 inCount, uint32 outCount,
	physical_entry *entries)
{
	CALLED();
	fRequest->task_attr = VIRTIO_SCSI_S_SIMPLE;
	fRequest->tag = (addr_t)fCCB;
	fRequest->lun[0] = 1;
	fRequest->lun[1] = fCCB->target_id;
	// we don't support lun >= 256
	fRequest->lun[2] = 0x40;
	fRequest->lun[3] = fCCB->target_lun & 0xff;

	memcpy(fRequest->cdb, fCCB->cdb, min_c(fCCB->cdb_length,
		min_c(sizeof(fRequest->cdb), sizeof(fCCB->cdb))));

	get_memory_map(fBuffer, sizeof(struct virtio_scsi_cmd_req)
		+ sizeof(struct virtio_scsi_cmd_resp), &entries[0], 1);
	entries[0].size = sizeof(struct virtio_scsi_cmd_req);
	if (outCount > 1) {
		memcpy(entries + 1, fCCB->sg_list, fCCB->sg_count
			* sizeof(physical_entry));
	}

	entries[outCount].address = entries[0].address
		+ sizeof(struct virtio_scsi_cmd_req);
	entries[outCount].size = sizeof(struct virtio_scsi_cmd_resp);

	if (inCount > 1) {
		memcpy(entries + outCount + 1, fCCB->sg_list, fCCB->sg_count
			* sizeof(physical_entry));
	}
}


uchar
VirtioSCSIRequest::_ResponseStatus()
{
	uchar status;

	switch (fResponse->response) {
		case VIRTIO_SCSI_S_OK:
			status = SCSI_REQ_CMP;
			break;
		case VIRTIO_SCSI_S_OVERRUN:
			status = SCSI_DATA_RUN_ERR;
			break;
		case VIRTIO_SCSI_S_ABORTED:
			status = SCSI_REQ_ABORTED;
			break;
		case VIRTIO_SCSI_S_BAD_TARGET:
			status = SCSI_TID_INVALID;
			break;
		case VIRTIO_SCSI_S_RESET:
			status = SCSI_SCSI_BUS_RESET;
			break;
		case VIRTIO_SCSI_S_BUSY:
			status = SCSI_SCSI_BUSY;
			break;
		case VIRTIO_SCSI_S_TRANSPORT_FAILURE:
		case VIRTIO_SCSI_S_TARGET_FAILURE:
		case VIRTIO_SCSI_S_NEXUS_FAILURE:
			status = SCSI_NO_NEXUS;
			break;
		default: /* VIRTIO_SCSI_S_FAILURE */
			status = SCSI_REQ_CMP_ERR;
			break;
	}

	return status;
}
