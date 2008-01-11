
#include "ata_request.h"
#include "ide_internal.h"
#include "scsi_cmds.h"
#include <string.h>

#define TRACE dprintf

void
ata_request_init(ata_request *request, struct ide_device_info *device)
{
	memset(request, 0, sizeof(request));
	request->device = device;
}


/* Start the request, but don't clear sense to allow 
 * retrieving the previous sense data.
 */
void ata_request_start(ata_request **_request, struct ide_device_info *device, struct scsi_ccb *ccb)
{
	ata_request *request;

	IDE_LOCK(device->bus);
	if (device->bus->state != ata_state_idle) {
		request = NULL;
	} else {
		ASSERT(device->requestFree != NULL);
		ASSERT(device->requestActive == NULL);
		ASSERT(device->bus->active_device == NULL);

		device->bus->state = ata_state_busy;
		device->bus->active_device = device;
	
		request = device->requestFree;
		device->requestActive = request;
		device->requestFree = NULL;
	}
	IDE_UNLOCK(device->bus);

	*_request = request;

	if (!request)
		return; // bus was busy

	ASSERT(request->device == device);

	request->ccb = ccb;
	request->is_write = 0;
	request->uses_dma = 0;
	request->packet_irq = 0;

	// XXX the following always triggers. Why?
	// ASSERT(request->ccb->subsys_status == SCSI_REQ_INPROG);

	// pretend success
	request->ccb->subsys_status = SCSI_REQ_CMP;

	// device_status always remains set to SCSI_STATUS_GOOD
	// except when ata_request_set_checkcondition() is called.
	request->ccb->device_status = SCSI_STATUS_GOOD;
}


void
ata_request_clear_sense(ata_request *request)
{
	request->senseKey = 0;
	request->senseAsc = 0;
	request->senseAscq = 0;
}


void
ata_request_set_status(ata_request *request, uint8 status)
{
	ASSERT(status != SCSI_REQ_CMP);
	if (!request)
		return;
	request->ccb->subsys_status = status;
}


void
ata_request_set_sense(ata_request *request, uint8 key, uint16 asc_acq)
{
	if (!request)
		return;
	request->senseKey = key;
	request->senseAsc = asc_acq >> 8;
	request->senseAscq = asc_acq & 0xff;
}


void
ata_request_finish(ata_request *request, bool resubmit)
{
	scsi_ccb *ccb;

	TRACE("ata_request_finish: request %p, subsys_status 0x%02x, senseKey %02x\n",
		request, request->ccb->subsys_status, request->senseKey);

	// when the request completed and has set sense
    // data, report this to the scsci stack by setting 
    // CHECK CONDITION status
	if (request->ccb->subsys_status == SCSI_REQ_CMP && request->senseKey != 0) {
	
		TRACE("ata_request_finish - setting check condition\n");

		request->ccb->subsys_status = SCSI_REQ_CMP_ERR;
		request->ccb->device_status = SCSI_STATUS_CHECK_CONDITION;

		// copy sense data if caller requested it
		if ((request->ccb->flags & SCSI_DIS_AUTOSENSE) == 0) {
			scsi_sense sense;
			int sense_len;

			TRACE("ata_request_finish - copying autosense data\n");

			// we cannot copy sense directly as sense buffer may be too small
			scsi_set_sense(&sense, request);

			ASSERT(sizeof(request->ccb->sense) == SCSI_MAX_SENSE_SIZE);

			sense_len = min(sizeof(request->ccb->sense), sizeof(sense));

			memcpy(request->ccb->sense, &sense, sense_len);
			request->ccb->sense_resid = SCSI_MAX_SENSE_SIZE - sense_len;
			request->ccb->subsys_status |= SCSI_AUTOSNS_VALID;

			// device sense gets reset once it's read
			ata_request_clear_sense(request);

			ASSERT(request->ccb->subsys_status == (SCSI_REQ_CMP_ERR | SCSI_AUTOSNS_VALID));
			ASSERT(request->ccb->device_status == SCSI_STATUS_CHECK_CONDITION);
		}
	}

	ccb = request->ccb;

	IDE_LOCK(request->device->bus);
	ASSERT(request->device->bus->state == ata_state_busy);
	ASSERT(request->device->bus->active_device == request->device);
	ASSERT(request->device->requestActive == request);
	ASSERT(request->device->requestFree == NULL);
	request->device->bus->state = ata_state_idle;
	request->device->bus->active_device = NULL;
	request->device->requestActive = NULL;
	request->device->requestFree = request;
	IDE_UNLOCK(request->device->bus);

	ACQUIRE_BEN(&request->device->bus->status_report_ben);
	if (resubmit)
		scsi->resubmit(ccb);
	else
		scsi->finished(ccb, 1);
	RELEASE_BEN(&request->device->bus->status_report_ben);
}
