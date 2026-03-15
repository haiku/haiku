/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "HyperVSCSI.h"


HyperVSCSI::HyperVSCSI(device_node* node)
	:
	fNode(node),
	fBus(NULL),
	fBusDPC(NULL),
	fStatus(B_NO_INIT),
	fIsIDE(false),
	fTargetID(0),
	fMaxDevices(HV_SCSI_MAX_SCSI_DEVICES),
	fVersion(0),
	fMaxSubChannels(0),
	fMaxTransferBytes(0),
	fPacket(NULL),
	fHyperV(NULL),
	fHyperVCookie(NULL)
{
	CALLED();

	mutex_init(&fRequestLock, "hyperv_scsi requests");

	// VMBus device will be the parent of the parent
	DeviceNodePutter<&gDeviceManager> simParent(gDeviceManager->get_parent_node(node));
	DeviceNodePutter<&gDeviceManager> parent(gDeviceManager->get_parent_node(simParent.Get()));
	gDeviceManager->get_driver(parent.Get(), (driver_module_info**)&fHyperV,
		(void**)&fHyperVCookie);

	// Check if parent is a Hyper-V IDE controller
	const char* type;
	fStatus = gDeviceManager->get_attr_string(parent.Get(), HYPERV_DEVICE_TYPE_STRING_ITEM, &type,
		false);
	if (fStatus != B_OK)
		return;

	fIsIDE = strcmp(type, VMBUS_TYPE_IDE) == 0;
	if (fIsIDE) {
		const vmbus_guid_t* instanceID = NULL;
		size_t instanceIDLength;
		fStatus = gDeviceManager->get_attr_raw(parent.Get(), HYPERV_INSTANCE_ID_ITEM,
			(const void**)&instanceID, &instanceIDLength, false);
		if (fStatus != B_OK)
			return;

		if (instanceIDLength != sizeof(*instanceID)) {
			fStatus = B_BAD_VALUE;
			return;
		}

		// Hyper-V creates a device per IDE disk
		// Use the target ID specified as part of the channel's instance ID
		fMaxDevices = HV_SCSI_MAX_IDE_DEVICES;
		fTargetID = static_cast<uint8>(instanceID->data2);
		TRACE("Controller is IDE using target ID %u\n", fTargetID);
	}

	fStatus = gSCSI->alloc_dpc(&fBusDPC);
	if (fStatus != B_OK)
		return;

	fPacket = static_cast<uint8*>(malloc(HV_SCSI_RX_PKT_BUFFER_SIZE));
	if (fPacket == NULL) {
		fStatus = B_NO_MEMORY;
		return;
	}

	for (uint32 i = 0; i < HV_SCSI_MAX_REQUESTS; i++) {
		HyperVSCSIRequest* request = new(std::nothrow) HyperVSCSIRequest;
		if (request == NULL) {
			fStatus = B_NO_MEMORY;
			return;
		}

		fStatus = request->InitCheck();
		if (fStatus != B_OK) {
			delete request;
			return;
		}

		fFreeRequests.Add(request);
	}

	fStatus = fHyperV->open(fHyperVCookie, HV_SCSI_RING_SIZE, HV_SCSI_RING_SIZE, _CallbackHandler,
		this);
	if (fStatus != B_OK) {
		ERROR("Failed to open channel (%s)\n", strerror(fStatus));
		return;
	}

	fStatus = _BeginInit();
	if (fStatus != B_OK) {
		ERROR("Failed to begin controller initialization (%s)\n", strerror(fStatus));
		return;
	}

	fStatus = _NegotiateProtocol();
	if (fStatus != B_OK) {
		ERROR("Failed to negotiate controller protocol (%s)\n", strerror(fStatus));
		return;
	}

	fStatus = _QueryProperties();
	if (fStatus != B_OK) {
		ERROR("Failed to query controller properties (%s)\n", strerror(fStatus));
		return;
	}

	fStatus = _EndInit();
	if (fStatus != B_OK) {
		ERROR("Failed to end controller initialization (%s)\n", strerror(fStatus));
		return;
	}
}


HyperVSCSI::~HyperVSCSI()
{
	CALLED();

	fHyperV->close(fHyperVCookie);
	gSCSI->free_dpc(fBusDPC);

	MutexLocker requestLocker(fRequestLock);

	// Abort any active requests
	HyperVSCSIRequestList::Iterator activeIterator = fActiveRequests.GetIterator();
	while (activeIterator.HasNext()) {
		HyperVSCSIRequest* request = activeIterator.Next();
		request->Complete(SCSI_REQ_ABORTED);
		request->Notify();
		delete request;
	}

	HyperVSCSIRequestList::Iterator freeIterator = fFreeRequests.GetIterator();
	while (freeIterator.HasNext()) {
		HyperVSCSIRequest* request = freeIterator.Next();
		delete request;
	}

	mutex_destroy(&fRequestLock);
	free(fPacket);
}


status_t
HyperVSCSI::StartIO(scsi_ccb* ccb)
{
	HyperVSCSIRequest* request = _GetRequest();
	if (request == NULL)
		return B_BUSY;
	request->SetCCB(ccb);

	if (ccb->target_id > fMaxDevices) {
		request->Complete(SCSI_TID_INVALID);
		_ReturnRequest(request);
		return B_BAD_INDEX;
	}

	if (ccb->target_lun > 0) {
		request->Complete(SCSI_LUN_INVALID);
		_ReturnRequest(request);
		return B_BAD_INDEX;
	}

	if (ccb->cdb_length > HV_SCSI_CDB_SIZE) {
		request->Complete(SCSI_REQ_INVALID);
		_ReturnRequest(request);
		return B_BAD_VALUE;
	}

	if (ccb->data_length > HV_SCSI_MAX_BUFFER_SIZE) {
		request->Complete(SCSI_REQ_INVALID);
		_ReturnRequest(request);
		return B_NO_MEMORY;
	}

	request->SetMessageType(HV_SCSI_MSGTYPE_EXECUTE_SRB);
	hv_scsi_msg_request* message = &request->GetMessage()->request;

	// Hyper-V represents all attached devices under a single SCSI target
	// This driver represents each device as a target with one LUN
	message->target_id = fTargetID;
	message->lun = ccb->target_id;
	message->length = sizeof(*message) - sizeof(message->header) - _GetMessageLengthDelta();
	message->sense_info_length = _GetSenseLength();

	if (!_IsLegacy()) {
		message->win8_extension.timeout = 60;
		message->win8_extension.srb_flags |= HV_SCSI_SRB_FLAGS_DISABLE_SYNC_TRANSFER;
	}

	switch (ccb->flags & SCSI_DIR_MASK) {
		case SCSI_DIR_IN:
			message->direction = HV_SCSI_DIRECTION_READ;
			message->data_length = ccb->data_length;
			if (!_IsLegacy())
				message->win8_extension.srb_flags |= HV_SCSI_SRB_FLAGS_DATA_IN;
			break;

		case SCSI_DIR_OUT:
			message->direction = HV_SCSI_DIRECTION_WRITE;
			message->data_length = ccb->data_length;
			if (!_IsLegacy())
				message->win8_extension.srb_flags |= HV_SCSI_SRB_FLAGS_DATA_OUT;
			break;

		case SCSI_DIR_NONE:
			message->direction = HV_SCSI_DIRECTION_UNKNOWN;
			break;

		default:
			request->Complete(SCSI_REQ_INVALID);
			_ReturnRequest(request);
			return B_BAD_VALUE;
	}

	message->cdb_length = ccb->cdb_length;
	memcpy(message->cdb, ccb->cdb, ccb->cdb_length);

	TRACE("CDB[%u] %02X%02X %02X%02X %02X%02X %02X%02X %02X%02X %02X%02X %02X%02X %02X%02X\n",
		message->cdb_length, message->cdb[0], message->cdb[1], message->cdb[2], message->cdb[3],
		message->cdb[4], message->cdb[5], message->cdb[6], message->cdb[7], message->cdb[8],
		message->cdb[9], message->cdb[10], message->cdb[11], message->cdb[12], message->cdb[13],
		message->cdb[14], message->cdb[15]);

	request->SetTimeout(ccb->timeout > 0 ? HV_MS_TO_US(ccb->timeout * 1000) : HV_SCSI_TIMEOUT_US);

#ifdef TRACE_HYPERV_SCSI
	const char* dirStr = "";
	if (message->direction == HV_SCSI_DIRECTION_READ)
		dirStr = " IN";
	else if (message->direction == HV_SCSI_DIRECTION_WRITE)
		dirStr = " OUT";
	TRACE("SCSI%s request target %u LUN %u length 0x%X\n", dirStr, message->target_id,
		message->lun, message->data_length);
#endif

	status_t status;
	if (message->direction != HV_SCSI_DIRECTION_UNKNOWN) {
		status = request->PrepareData();
		if (status != B_OK) {
			ERROR("Failed to prepare for data transfer (%s)\n", strerror(status));
			request->Complete(SCSI_REQ_ABORTED);
			_ReturnRequest(request);
			return status;
		}
	}

	status = _SendRequest(request, true);
	if (status != B_OK) {
		ERROR("Failed to send request (%s)\n", strerror(status));
		request->Complete(SCSI_REQ_ABORTED);
		_ReturnRequest(request);
		return status;
	}

	uint8 requestStatus = message->header.status == HV_SCSI_SUCCESS
		? SCSI_REQ_CMP : SCSI_REQ_CMP_ERR;
	request->Complete(requestStatus);

	_ReturnRequest(request);
	return B_OK;
}


uchar
HyperVSCSI::PathInquiry(scsi_path_inquiry* inquiry_data)
{
	if (GetBus() == NULL)
		return SCSI_NO_HBA;

	inquiry_data->hba_inquiry = 0;
	inquiry_data->hba_misc = 0;
	inquiry_data->initiator_id = fMaxDevices;
	inquiry_data->hba_queue_size = HV_SCSI_MAX_REQUESTS;
	bzero(inquiry_data->vuhba_flags, sizeof(inquiry_data->vuhba_flags));

	strlcpy(inquiry_data->sim_vid, "Haiku", SCSI_SIM_ID);
	strlcpy(inquiry_data->hba_vid, "Hyper-V", SCSI_HBA_ID);
	strlcpy(inquiry_data->sim_version, "1.0", SCSI_VERS);
	snprintf(inquiry_data->hba_version, SCSI_VERS, "%u.%u", GET_SCSI_VERSION_MAJOR(fVersion),
		GET_SCSI_VERSION_MINOR(fVersion));
	strlcpy(inquiry_data->controller_family, "Hyper-V", SCSI_FAM_ID);
	strlcpy(inquiry_data->controller_type, "Hyper-V", SCSI_TYPE_ID);

	return SCSI_REQ_CMP;
}


uchar
HyperVSCSI::ResetBus()
{
	HyperVSCSIRequest* request = _GetRequest();
	if (request == NULL)
		return SCSI_REQ_ABORTED;

	request->SetMessageType(HV_SCSI_MSGTYPE_RESET_BUS);
	status_t status = _SendRequest(request, true);
	if (status == B_OK)
		status = request->GetMessageStatus() == HV_SCSI_SUCCESS ? B_OK : B_IO_ERROR;

	_ReturnRequest(request);
	return status == B_OK ? SCSI_REQ_CMP : SCSI_REQ_ABORTED;
}


uchar
HyperVSCSI::ResetDevice(uchar targetID, uchar targetLUN)
{
	if (targetID > fMaxDevices)
		return SCSI_TID_INVALID;
	if (targetLUN > 0)
		return SCSI_LUN_INVALID;

	HyperVSCSIRequest* request = _GetRequest();
	if (request == NULL)
		return SCSI_REQ_ABORTED;

	request->SetMessageType(HV_SCSI_MSGTYPE_RESET_LUN);
	hv_scsi_msg_request* message = &request->GetMessage()->request;

	message->target_id = fTargetID;
	message->lun = targetID;

	status_t status = _SendRequest(request, true);
	if (status == B_OK)
		status = request->GetMessageStatus() == HV_SCSI_SUCCESS ? B_OK : B_IO_ERROR;

	_ReturnRequest(request);
	return status == B_OK ? SCSI_REQ_CMP : SCSI_REQ_ABORTED;
}


/*static*/ void
HyperVSCSI::_CallbackHandler(void* data)
{
	HyperVSCSI* scsi = reinterpret_cast<HyperVSCSI*>(data);
	scsi->_Callback();
}


uint8
HyperVSCSI::_GetSenseLength() const
{
	return _IsLegacy() ? HV_SCSI_LEGACY_SENSE_SIZE : HV_SCSI_SENSE_SIZE;
}

uint32
HyperVSCSI::_GetMessageLengthDelta() const
{
	return _IsLegacy() ? sizeof(hv_scsi_request_win8_extension) : 0;
}


void
HyperVSCSI::_Callback()
{
	while (true) {
		uint32 length = HV_SCSI_RX_PKT_BUFFER_SIZE;
		uint32 headerLength;
		uint32 messageLength;

		status_t status = fHyperV->read_packet(fHyperVCookie, fPacket, &length, &headerLength,
			&messageLength);
		if (status == B_DEV_NOT_READY) {
			break;
		} else if (status != B_OK) {
			ERROR("Failed to read packet (%s)\n", strerror(status));
			break;
		}

		vmbus_pkt_header* header = reinterpret_cast<vmbus_pkt_header*>(fPacket);
		hv_scsi_msg* message = reinterpret_cast<hv_scsi_msg*>(fPacket + headerLength);

		switch (message->header.type) {
			case HV_SCSI_MSGTYPE_COMPLETE_IO:
				_CompleteIO(header->transaction_id, message);
				break;

			case HV_SCSI_MSGTYPE_REMOVE_DEVICE:
			case HV_SCSI_MSGTYPE_ENUM_BUS:
				// Device added/removed, trigger rescan on seperate thread
				gSCSI->schedule_dpc(fBus, fBusDPC, _RescanDPCHandler, this);
				break;

			default:
				TRACE("Unknown message type %u\n", message->header.type);
				break;
		}
	}
}


/*static*/ void
HyperVSCSI::_RescanDPCHandler(void* data)
{
	HyperVSCSI* scsi = reinterpret_cast<HyperVSCSI*>(data);
	scsi->_RescanBus();
}

void
HyperVSCSI::_RescanBus()
{
	TRACE("Rescanning bus\n");

	device_node *childNode = NULL;
	const device_attr attrs[] = { { NULL } };
	if (gDeviceManager->get_next_child_node(fNode, attrs, &childNode) != B_OK) {
		ERROR("Failed to find child node for %p\n", fNode);
		return;
	}

	gDeviceManager->rescan_node(childNode);
	gDeviceManager->put_node(childNode);
}


status_t
HyperVSCSI::_SendRequest(HyperVSCSIRequest* request, bool wait)
{
	hv_scsi_msg* message = request->GetMessage();
	scsi_ccb* ccb = request->GetCCB();

	message->header.flags = HV_SCSI_FLAG_REQUEST_COMPLETION;

	MutexLocker requestLocker(fRequestLock);
	fActiveRequests.Add(request);
	requestLocker.Unlock();

	ConditionVariableEntry entry;
	if (wait)
		request->AddWaiter(&entry);

	// Data requests are sent as GPA packets, all others as standard inband
	status_t status;
	if (ccb != NULL && message->request.direction != HV_SCSI_DIRECTION_UNKNOWN) {
		vmbus_gpa_range* gpaRange = request->GetGPARange();
		uint32 gpaLength = request->GetGPARangeLength();

		status = fHyperV->write_gpa_packet(fHyperVCookie, 1, gpaRange, gpaLength, message,
			sizeof(*message) - _GetMessageLengthDelta(), true, (uint64)request);
	} else {
		status = fHyperV->write_packet(fHyperVCookie, VMBUS_PKTTYPE_DATA_INBAND, message,
			sizeof(*message) - _GetMessageLengthDelta(), true, (uint64)request);
	}

	if (status != B_OK) {
		ERROR("Failed to submit request %p (%s)\n", request, strerror(status));
		requestLocker.Lock();
		fActiveRequests.Remove(request);
		return status;
	}

	if (wait) {
		status = entry.Wait(B_RELATIVE_TIMEOUT, request->GetTimeout());
		if (status != B_OK) {
			ERROR("Failed to wait for request %p (%s)\n", request, strerror(status));
			requestLocker.Lock();
			if (fActiveRequests.Contains(request))
				fActiveRequests.Remove(request);
			else
				ERROR("Request %p not present in active list\n", request);
			return status;
		}
	}

	return B_OK;
}


HyperVSCSIRequest*
HyperVSCSI::_GetRequest()
{
	MutexLocker requestLocker(fRequestLock);

	HyperVSCSIRequest* request = fFreeRequests.RemoveHead();
	if (request != NULL)
		request->Reset();

	return request;
}


void
HyperVSCSI::_ReturnRequest(HyperVSCSIRequest* request)
{
	MutexLocker requestLocker(fRequestLock);
	fFreeRequests.Add(request);
}


void
HyperVSCSI::_CompleteIO(uint64 transactionID, hv_scsi_msg* message)
{
	HyperVSCSIRequest* request = (HyperVSCSIRequest*)transactionID;
	MutexLocker requestLocker(fRequestLock);

	if (!fActiveRequests.Contains(request)) {
		ERROR("Attempted to complete unknown request %p\n", request);
		return;
	}

	request->SetMessageData(message);

	fActiveRequests.Remove(request);
	requestLocker.Unlock();

	request->Notify();
}


status_t
HyperVSCSI::_BeginInit()
{
	HyperVSCSIRequest* request = _GetRequest();
	if (request == NULL)
		return B_BUSY;

	request->SetMessageType(HV_SCSI_MSGTYPE_BEGIN_INIT);
	status_t status = _SendRequest(request, true);
	if (status == B_OK)
		status = request->GetMessageStatus() == HV_SCSI_SUCCESS ? B_OK : B_IO_ERROR;

	_ReturnRequest(request);
	return status;
}


status_t
HyperVSCSI::_NegotiateProtocol()
{
	HyperVSCSIRequest* request = _GetRequest();
	if (request == NULL)
		return B_BUSY;

	status_t status = B_UNSUPPORTED;
	for (uint32 i = 0; i < hv_scsi_version_count; i++) {
		request->Reset();
		request->SetMessageType(HV_SCSI_MSGTYPE_QUERY_PROTOCOL_VER);
		request->GetMessage()->protocol.version = hv_scsi_versions[i];
		request->GetMessage()->protocol.revision = 0;

		status = _SendRequest(request, true);
		if (status == B_OK)
			status = request->GetMessageStatus() == HV_SCSI_SUCCESS ? B_OK : B_UNSUPPORTED;

		if (status == B_OK) {
			fVersion = hv_scsi_versions[i];
			TRACE("SCSI protocol version %u.%u, sense 0x%X, delta 0x%X\n",
				GET_SCSI_VERSION_MAJOR(fVersion), GET_SCSI_VERSION_MINOR(fVersion),
				_GetSenseLength(), _GetMessageLengthDelta());
			break;
		}
	}

	_ReturnRequest(request);
	return status;
}


status_t
HyperVSCSI::_QueryProperties()
{
	HyperVSCSIRequest* request = _GetRequest();
	if (request == NULL)
		return B_BUSY;

	request->SetMessageType(HV_SCSI_MSGTYPE_QUERY_PROPERTIES);
	status_t status = _SendRequest(request, true);
	if (status == B_OK)
		status = request->GetMessageStatus() == HV_SCSI_SUCCESS ? B_OK : B_IO_ERROR;

	if (status == B_OK) {
		if ((request->GetMessage()->channel_properties.flags & HV_SCSI_FLAG_SUPPORTS_MULTI_CHANNEL)
				!= 0)
			fMaxSubChannels = request->GetMessage()->channel_properties.max_channels;
		fMaxTransferBytes = request->GetMessage()->channel_properties.max_transfer_bytes;
		TRACE("SCSI max sub channels %u max transfer bytes 0x%X\n", fMaxSubChannels,
			fMaxTransferBytes);
	}

	_ReturnRequest(request);
	return B_OK;
}


status_t
HyperVSCSI::_EndInit()
{
	HyperVSCSIRequest* request = _GetRequest();
	if (request == NULL)
		return B_BUSY;

	request->SetMessageType(HV_SCSI_MSGTYPE_END_INIT);
	status_t status = _SendRequest(request, true);
	if (status == B_OK)
		status = request->GetMessageStatus() == HV_SCSI_SUCCESS ? B_OK : B_IO_ERROR;

	_ReturnRequest(request);
	return status;
}
