/*
 * Copyright 2013, Jérôme Duval, korli@users.berlios.de.
 * Distributed under the terms of the MIT License.
 */


#include "VirtioSCSIPrivate.h"

#include <StackOrHeapArray.h>

#include <new>
#include <stdlib.h>
#include <strings.h>

#include <util/AutoLock.h>


const char *
get_feature_name(uint32 feature)
{
	switch (feature) {
		case VIRTIO_SCSI_F_INOUT:
			return "in out";
		case VIRTIO_SCSI_F_HOTPLUG:
			return "hotplug";
		case VIRTIO_SCSI_F_CHANGE:
			return "change";
	}
	return NULL;
}


VirtioSCSIController::VirtioSCSIController(device_node *node)
	:
	fNode(node),
	fVirtio(NULL),
	fVirtioDevice(NULL),
	fStatus(B_NO_INIT),
	fRequest(NULL),
	fCurrentRequest(0),
	fEventDPC(NULL)
{
	CALLED();

	fInterruptCondition.Init(this, "virtio scsi transfer");

	if (gSCSI->alloc_dpc(&fEventDPC) != B_OK)
		return;

	// get the Virtio device from our parent's parent
	device_node *parent = gDeviceManager->get_parent_node(node);
	device_node *virtioParent = gDeviceManager->get_parent_node(parent);
	gDeviceManager->put_node(parent);

	gDeviceManager->get_driver(virtioParent, (driver_module_info **)&fVirtio,
		(void **)&fVirtioDevice);
	gDeviceManager->put_node(virtioParent);

	fVirtio->negotiate_features(fVirtioDevice,
		VIRTIO_SCSI_F_CHANGE /*VIRTIO_SCSI_F_HOTPLUG*/,
		&fFeatures, &get_feature_name);

	fStatus = fVirtio->read_device_config(fVirtioDevice, 0, &fConfig,
		sizeof(struct virtio_scsi_config));
	if (fStatus != B_OK)
		return;

	fConfig.sense_size = VIRTIO_SCSI_SENSE_SIZE;
	fConfig.cdb_size = VIRTIO_SCSI_CDB_SIZE;

	fVirtio->write_device_config(fVirtioDevice,
		offsetof(struct virtio_scsi_config, sense_size), &fConfig.sense_size,
		sizeof(fConfig.sense_size));
	fVirtio->write_device_config(fVirtioDevice,
		offsetof(struct virtio_scsi_config, cdb_size), &fConfig.cdb_size,
		sizeof(fConfig.cdb_size));

	fRequest = new(std::nothrow) VirtioSCSIRequest(true);
	if (fRequest == NULL) {
		fStatus = B_NO_MEMORY;
		return;
	}

	::virtio_queue virtioQueues[3];
	fStatus = fVirtio->alloc_queues(fVirtioDevice, 3, virtioQueues);
	if (fStatus != B_OK) {
		ERROR("queue allocation failed (%s)\n", strerror(fStatus));
		return;
	}

	fControlVirtioQueue = virtioQueues[0];
	fEventVirtioQueue = virtioQueues[1];
	fRequestVirtioQueue = virtioQueues[2];

	for (uint32 i = 0; i < VIRTIO_SCSI_NUM_EVENTS; i++)
		_SubmitEvent(i);

	fStatus = fVirtio->setup_interrupt(fVirtioDevice, NULL, this);
	if (fStatus != B_OK) {
		ERROR("interrupt setup failed (%s)\n", strerror(fStatus));
		return;
	}

	fStatus = fVirtio->queue_setup_interrupt(fControlVirtioQueue,
		NULL, NULL);
	if (fStatus == B_OK) {
		fStatus = fVirtio->queue_setup_interrupt(fEventVirtioQueue,
			VirtioSCSIController::_EventCallback, this);
	}
	if (fStatus == B_OK) {
		fStatus = fVirtio->queue_setup_interrupt(fRequestVirtioQueue,
			VirtioSCSIController::_RequestCallback, this);
	}

	if (fStatus != B_OK) {
		ERROR("queue interrupt setup failed (%s)\n", strerror(fStatus));
		return;
	}
}


VirtioSCSIController::~VirtioSCSIController()
{
	CALLED();
	delete fRequest;

	gSCSI->free_dpc(fEventDPC);
}


status_t
VirtioSCSIController::InitCheck()
{
	return fStatus;
}


void
VirtioSCSIController::SetBus(scsi_bus bus)
{
	fBus = bus;
}


void
VirtioSCSIController::PathInquiry(scsi_path_inquiry *info)
{
	info->hba_inquiry = SCSI_PI_TAG_ABLE;
	info->hba_misc = 0;
	info->sim_priv = 0;
	info->initiator_id = VIRTIO_SCSI_INITIATOR_ID;
	info->hba_queue_size = fConfig.cmd_per_lun != 0 ? fConfig.cmd_per_lun : 1;
	memset(info->vuhba_flags, 0, sizeof(info->vuhba_flags));

	strlcpy(info->sim_vid, "Haiku", SCSI_SIM_ID);
	strlcpy(info->hba_vid, "VirtIO", SCSI_HBA_ID);

	strlcpy(info->sim_version, "1.0", SCSI_VERS);
	strlcpy(info->hba_version, "1.0", SCSI_VERS);
	strlcpy(info->controller_family, "Virtio", SCSI_FAM_ID);
	strlcpy(info->controller_type, "Virtio", SCSI_TYPE_ID);
}


void
VirtioSCSIController::GetRestrictions(uint8 targetID, bool *isATAPI,
	bool *noAutoSense, uint32 *maxBlocks)
{
	*isATAPI = false;
	*noAutoSense = true;
	*maxBlocks = fConfig.cmd_per_lun;
}


uchar
VirtioSCSIController::ResetDevice(uchar targetID, uchar targetLUN)
{
	return SCSI_REQ_CMP;
}


status_t
VirtioSCSIController::ExecuteRequest(scsi_ccb *ccb)
{
	status_t result = fRequest->Start(ccb);
	if (result != B_OK)
		return result;

	if (ccb->cdb[0] == SCSI_OP_REQUEST_SENSE && fRequest->HasSense()) {
		TRACE("request sense\n");
		fRequest->RequestSense();
		fRequest->Finish(false);
		return B_OK;
	}

	if (ccb->target_id > fConfig.max_target) {
		ERROR("invalid target device\n");
		fRequest->SetStatus(SCSI_TID_INVALID);
		fRequest->Finish(false);
		return B_BAD_INDEX;
	}

	if (ccb->target_lun > fConfig.max_lun) {
		ERROR("invalid lun device\n");
		fRequest->SetStatus(SCSI_LUN_INVALID);
		fRequest->Finish(false);
		return B_BAD_INDEX;
	}

	if (ccb->cdb_length > VIRTIO_SCSI_CDB_SIZE) {
		fRequest->SetStatus(SCSI_REQ_INVALID);
		fRequest->Finish(false);
		return B_BAD_VALUE;
	}

	bool isOut = (ccb->flags & SCSI_DIR_MASK) == SCSI_DIR_OUT;
	bool isIn = (ccb->flags & SCSI_DIR_MASK) == SCSI_DIR_IN;

	// TODO check feature inout if request is bidirectional

	fRequest->SetTimeout(ccb->timeout > 0 ? ccb->timeout * 1000 * 1000
		: VIRTIO_SCSI_STANDARD_TIMEOUT);

	uint32 inCount = (isIn ? ccb->sg_count : 0) + 1;
	uint32 outCount = (isOut ? ccb->sg_count : 0) + 1;
	BStackOrHeapArray<physical_entry, 16> entries(inCount + outCount);
	if (!entries.IsValid()) {
		fRequest->SetStatus(SCSI_REQ_INVALID);
		fRequest->Finish(false);
		return B_NO_MEMORY;
	}
	fRequest->FillRequest(inCount, outCount, entries);

	atomic_add(&fCurrentRequest, 1);
	fInterruptCondition.Add(&fInterruptConditionEntry);

	fVirtio->queue_request_v(fRequestVirtioQueue, entries,
		outCount, inCount, (void *)(addr_t)fCurrentRequest);

	result = fInterruptConditionEntry.Wait(B_RELATIVE_TIMEOUT,
		fRequest->Timeout());

	if (result != B_OK) {
		ERROR("wait failed with status: %#" B_PRIx32 "\n", result);
		fRequest->Abort();
		return result;
	}

	return fRequest->Finish(false);
}


uchar
VirtioSCSIController::AbortRequest(scsi_ccb *request)
{
	return SCSI_REQ_CMP;
}


uchar
VirtioSCSIController::TerminateRequest(scsi_ccb *request)
{
	return SCSI_REQ_CMP;
}


status_t
VirtioSCSIController::Control(uint8 targetID, uint32 op, void *buffer,
	size_t length)
{
	CALLED();
	return B_DEV_INVALID_IOCTL;
}


void
VirtioSCSIController::_RequestCallback(void* driverCookie, void* cookie)
{
	CALLED();
	VirtioSCSIController* controller = (VirtioSCSIController*)driverCookie;
	controller->_RequestInterrupt();
}


void
VirtioSCSIController::_RequestInterrupt()
{
	void* cookie = NULL;
	while (fVirtio->queue_dequeue(fRequestVirtioQueue, &cookie, NULL)) {
		if ((int32)(addr_t)cookie == atomic_get(&fCurrentRequest))
			fInterruptCondition.NotifyAll();
	}
}



void
VirtioSCSIController::_EventCallback(void* driverCookie, void* cookie)
{
	CALLED();
	VirtioSCSIController* controller = (VirtioSCSIController*)driverCookie;

	virtio_scsi_event* event = NULL;
	while (controller->fVirtio->queue_dequeue(controller->fEventVirtioQueue,
			(void**)&event, NULL)) {
		controller->_EventInterrupt(event);
	}
}


void
VirtioSCSIController::_EventInterrupt(struct virtio_scsi_event* event)
{
	CALLED();
	TRACE("events %#x\n", event->event);
	if ((event->event & VIRTIO_SCSI_T_EVENTS_MISSED) != 0) {
		ERROR("events missed\n");
	} else switch (event->event) {
		case VIRTIO_SCSI_T_TRANSPORT_RESET:
			ERROR("transport reset\n");
			break;
		case VIRTIO_SCSI_T_ASYNC_NOTIFY:
			ERROR("async notify\n");
			break;
		case VIRTIO_SCSI_T_PARAM_CHANGE:
		{
			uint16 sense = (event->reason >> 8)
				| ((event->reason & 0xff) << 8);
			if (sense == SCSIS_ASC_CAPACITY_DATA_HAS_CHANGED) {
				ERROR("capacity data has changed for %x:%x\n", event->lun[1],
					event->lun[2] << 8 | event->lun[3]);
				gSCSI->schedule_dpc(fBus, fEventDPC, _RescanChildBus, this);
			} else
				ERROR("param change, unknown reason\n");
			break;
		}
		default:
			ERROR("unknown event %#x\n", event->event);
			break;
	}
}


void
VirtioSCSIController::_SubmitEvent(uint32 eventNumber)
{
	CALLED();
	struct virtio_scsi_event* event = &fEventBuffers[eventNumber];
	bzero(event, sizeof(struct virtio_scsi_event));

	physical_entry entry;
	get_memory_map(event, sizeof(struct virtio_scsi_event), &entry, 1);

	fVirtio->queue_request_v(fEventVirtioQueue, &entry,
		0, 1, event);
}


void
VirtioSCSIController::_RescanChildBus(void *cookie)
{
	CALLED();
	VirtioSCSIController* controller = (VirtioSCSIController*)cookie;
	device_node *childNode = NULL;
	const device_attr attrs[] = { { NULL } };
	if (gDeviceManager->get_next_child_node(controller->fNode, attrs,
		&childNode) != B_OK) {
		ERROR("couldn't find the child node for %p\n", controller->fNode);
		return;
	}

	gDeviceManager->rescan_node(childNode);
	TRACE("rescan done %p\n", childNode);
	gDeviceManager->put_node(childNode);
}
