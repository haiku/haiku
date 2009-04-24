/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */

#include "ATAPrivate.h"


ATARequest::ATARequest(bool hasLock)
	:
	fHasLock(hasLock),
	fDevice(NULL),
	fTimeout(0),
	fBytesLeft(0),
	fIsWrite(false),
	fUseDMA(false),
	fCCB(NULL)
{
	if (hasLock)
		mutex_init(&fLock, "ata request");

	ClearSense();
}


ATARequest::~ATARequest()
{
	if (fHasLock)
		mutex_destroy(&fLock);
}


void
ATARequest::SetStatus(uint8 status)
{
	fStatus = status;
}


void
ATARequest::SetSense(uint8 key, uint16 codeQualifier)
{
	fSenseKey = key;
	fSenseCode = (uint8)(codeQualifier >> 8);
	fSenseQualifier = (uint8)(codeQualifier & 0xff);
}


void
ATARequest::ClearSense()
{
	fSenseKey = fSenseCode = fSenseQualifier = 0;
}


void
ATARequest::SetDevice(ATADevice *device)
{
	fDevice = device;
}


void
ATARequest::SetTimeout(bigtime_t timeout)
{
	fTimeout = timeout;
}


void
ATARequest::SetIsWrite(bool isWrite)
{
	fIsWrite = isWrite;
}


void
ATARequest::SetUseDMA(bool useDMA)
{
	fUseDMA = useDMA;
}


void
ATARequest::SetBytesLeft(uint32 bytesLeft)
{
	fBytesLeft = bytesLeft;
}


status_t
ATARequest::Start(scsi_ccb *ccb)
{
	if (mutex_trylock(&fLock) != B_OK)
		return B_BUSY;

	fCCB = ccb;
	fStatus = SCSI_REQ_CMP;
	fCCB->device_status = SCSI_STATUS_GOOD;
	fIsWrite = false;
	return B_OK;
}


status_t
ATARequest::Finish(bool resubmit)
{
	// when the request completed and has set sense
    // data, report this to the scsi stack by setting
    // CHECK CONDITION status
	if (fStatus == SCSI_REQ_CMP && fSenseKey != 0) {
		TRACE("setting check condition\n");

		fCCB->subsys_status = SCSI_REQ_CMP_ERR;
		fCCB->device_status = SCSI_STATUS_CHECK_CONDITION;

		// copy sense data if caller requested it
		if ((fCCB->flags & SCSI_DIS_AUTOSENSE) == 0) {
			// we cannot copy sense directly as sense buffer may be too small
			scsi_sense sense;
			_FillSense(&sense);

			size_t senseLength = MIN(sizeof(fCCB->sense), sizeof(sense));
			memcpy(fCCB->sense, &sense, senseLength);
			fCCB->sense_resid = SCSI_MAX_SENSE_SIZE - senseLength;
			fCCB->subsys_status |= SCSI_AUTOSNS_VALID;
			ClearSense();
		}
	} else
		fCCB->subsys_status = fStatus;

	mutex_unlock(&fLock);

	if (resubmit)
		gSCSIModule->resubmit(fCCB);
	else
		gSCSIModule->finished(fCCB, 1);

	return B_OK;
}


void
ATARequest::RequestSense()
{
	// Copy sense data from last request into data buffer of current request.
	// The sense data of last request is still present in the current request,
	// as it isn't cleared on SCSI_OP_REQUEST_SENSE.
	scsi_sense sense;
	if (fSenseKey != 0)
		_FillSense(&sense);
	else
		memset(&sense, 0, sizeof(sense));

	scsi_cmd_request_sense *command = (scsi_cmd_request_sense *)fCCB->cdb;
	copy_sg_data(fCCB, 0, command->allocation_length, &sense, sizeof(sense),
		false);

	fCCB->data_resid = fCCB->data_length - MIN(MIN(sizeof(sense),
		command->allocation_length), fCCB->data_length);
	ClearSense();
}


void
ATARequest::PrepareSGInfo()
{
	fSGElementsLeft = fCCB->sg_count;
	fCurrentSGElement = fCCB->sg_list;
	fCurrentSGOffset = 0;
	fHasOddByte = false;
	fCCB->data_resid = fCCB->data_length;
}


void
ATARequest::AdvanceSG(uint32 bytes)
{
	uint32 bytesLeft = fCurrentSGElement->size - fCurrentSGOffset;
	if (bytesLeft <= bytes) {
		fCurrentSGOffset = 0;
		fCurrentSGElement++;
		fSGElementsLeft--;
	} else
		fCurrentSGOffset += bytes;
}


void
ATARequest::SetOddByte(uint8 byte)
{
	fOddByte = byte;
	fHasOddByte = true;
}


bool
ATARequest::GetOddByte(uint8 *byte)
{
	if (!fHasOddByte)
		return false;

	if (byte != NULL)
		*byte = fOddByte;

	fHasOddByte = false;
	return true;
}


void
ATARequest::_FillSense(scsi_sense *sense)
{
	memset(sense, 0, sizeof(*sense));
	sense->error_code = SCSIS_CURR_ERROR;
	sense->sense_key = fSenseKey;
	sense->add_sense_length = sizeof(*sense) - 7;
	sense->asc = fSenseCode;
	sense->ascq = fSenseQualifier;
	sense->sense_key_spec.raw.SKSV = 0;	// no additional info
}
