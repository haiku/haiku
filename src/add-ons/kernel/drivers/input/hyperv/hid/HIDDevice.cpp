/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Driver.h"
#include "HIDCollection.h"
#include "HIDDevice.h"
#include "HIDReport.h"
#include "HIDReportItem.h"
#include "HIDWriter.h"
#include "ProtocolHandler.h"
#include <usb/USB_hid.h>


HIDDevice::HIDDevice(hyperv_device_interface* hyperv,
	hyperv_device hyperv_cookie)
	:
	fStatus(B_NO_INIT),
	fOpenCount(0),
	fRemoved(false),
	fParser(this),
	fProtocolHandlerCount(0),
	fProtocolHandlerList(NULL),
	fHyperV(hyperv),
	fHyperVCookie(hyperv_cookie),
	fDeviceInfo(NULL),
	fDeviceInfoLength(0),
	fPacket(NULL),
	fLastX(0),
	fLastY(0)
{
	CALLED();

	fProtocolRespEvent.Init(this, "hyper-v hid protoresp");
	fDeviceInfoEvent.Init(this, "hyper-v hid devinfo");

	fPacket = static_cast<uint8*>(malloc(HV_HID_RX_PKT_BUFFER_SIZE));
	if (fPacket == NULL) {
		fStatus = B_NO_MEMORY;
		return;
	}

	fStatus = _Connect();
	if (fStatus != B_OK) {
		ERROR("Failed to connect to Hyper-V HID\n");
		return;
	}

	ProtocolHandler::AddHandlers(*this, fProtocolHandlerList, fProtocolHandlerCount);
}


HIDDevice::~HIDDevice()
{
	CALLED();

	ProtocolHandler* handler = fProtocolHandlerList;
	while (handler != NULL) {
		ProtocolHandler* next = handler->NextHandler();
		delete handler;
		handler = next;
	}

	free(fPacket);
}


status_t
HIDDevice::Open(ProtocolHandler* handler, uint32 flags)
{
	atomic_add(&fOpenCount, 1);
	return B_OK;
}


status_t
HIDDevice::Close(ProtocolHandler* handler)
{
	atomic_add(&fOpenCount, -1);
	return B_OK;
}


void
HIDDevice::Removed()
{
	fRemoved = true;
}


status_t
HIDDevice::MaybeScheduleTransfer(HIDReport* report)
{
	if (fRemoved)
		return ENODEV;

	// Reports get sent as they become available, cannot query on-demand
	return B_OK;
}


status_t
HIDDevice::SendReport(HIDReport* report)
{
	// Hyper-V does not support reports to itself
	return B_NOT_SUPPORTED;
}


ProtocolHandler*
HIDDevice::ProtocolHandlerAt(uint32 index) const
{
	ProtocolHandler* handler = fProtocolHandlerList;
	while (handler != NULL) {
		if (index == 0)
			return handler;

		handler = handler->NextHandler();
		index--;
	}

	return NULL;
}


/*static*/ void
HIDDevice::_CallbackHandler(void* data)
{
	HIDDevice* hidDevice = reinterpret_cast<HIDDevice*>(data);
	hidDevice->_Callback();
}


void
HIDDevice::_Callback()
{
	while (true) {
		uint32 length = HV_HID_RX_PKT_BUFFER_SIZE;
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

		// Check if this is an HID pipe data message
		hv_hid_pipe_in_msg* message = reinterpret_cast<hv_hid_pipe_in_msg*>(fPacket + headerLength);
		if (message->pipe_header.type != HV_HID_PIPE_MSGTYPE_DATA) {
			ERROR("Non-data HID pipe message type %u received\n", message->pipe_header.type);
			continue;
		}

		switch (message->header.type) {
			case HV_HID_MSGTYPE_PROTOCOL_RESPONSE:
				memcpy(&fProtocolResponse, &message->protocol_resp, sizeof(fProtocolResponse));
				fProtocolRespEvent.NotifyAll();
				break;

			case HV_HID_MSGTYPE_INITIAL_DEV_INFO:
				if (fDeviceInfo != NULL)
					free(fDeviceInfo);
				fDeviceInfo = reinterpret_cast<hv_hid_msg_initial_dev_info*>(
					malloc(message->header.length));
				if (fDeviceInfo != NULL) {
					fDeviceInfoLength = message->header.length;
					memcpy(fDeviceInfo, &message->dev_info, fDeviceInfoLength);
					fDeviceInfoEvent.NotifyAll();
				} else {
					fDeviceInfoLength = 0;
					ERROR("Failed to allocate device info\n");
					fDeviceInfoEvent.NotifyAll(B_NO_MEMORY);
				}
				break;

			case HV_HID_MSGTYPE_INPUT_REPORT:
				_HandleInputReport(&message->input_report);
				break;

			default:
				TRACE("Unexpected HID message type %u received\n", message->header.type);
				break;
		}
	}
}


void
HIDDevice::_HandleInputReport(hv_hid_msg_input_report* reportMessage)
{
	if (reportMessage->header.length < sizeof(hyperv_hid_input_report))
		return;

	hyperv_hid_input_report* report
		= reinterpret_cast<hyperv_hid_input_report*>(reportMessage->data);
	TRACE("New input report (btn %u x %u y %u wheel %d)\n", report->buttons, report->x, report->y,
		report->wheel);

	// Some versions of Hyper-V report an X/Y of zero when the wheel is scrolling,
	// use the last reported X/Y
	if (report->wheel != 0 && report->x == 0 && report->y == 0) {
		report->x = fLastX;
		report->y = fLastY;
		TRACE("Fixed input report x %u y %u\n", report->x, report->y);
	} else {
		fLastX = report->x;
		fLastY = report->y;
	}

	fParser.SetReport(B_OK, reportMessage->data, reportMessage->header.length);
}

status_t
HIDDevice::_Connect()
{
	status_t status = fHyperV->open(fHyperVCookie, HV_HID_RING_SIZE, HV_HID_RING_SIZE,
		_CallbackHandler, this);
	if (status != B_OK) {
		ERROR("Failed to open channel\n");
		return status;
	}

	hv_hid_pipe_out_msg message;
	message.pipe_header.type = HV_HID_PIPE_MSGTYPE_DATA;
	message.pipe_header.length = sizeof(message.protocol_req);
	message.header.type = HV_HID_MSGTYPE_PROTOCOL_REQUEST;
	message.header.length = message.pipe_header.length - sizeof(message.header);
	message.protocol_req.version = HV_HID_VERSION_V2_0;

	ConditionVariableEntry protocolRespEntry;
	ConditionVariableEntry deviceInfoEntry;
	fProtocolRespEvent.Add(&protocolRespEntry);
	fDeviceInfoEvent.Add(&deviceInfoEntry);

	// Send the protocol request message and wait
	// Initial device info is received immediately after protocol response
	status = fHyperV->write_packet(fHyperVCookie, VMBUS_PKTTYPE_DATA_INBAND, &message,
		sizeof(message.pipe_header) + message.pipe_header.length, TRUE, HV_HID_REQUEST_TRANS_ID);
	if (status != B_OK) {
		ERROR("Failed to send HID protocol request\n");
		return status;
	}

	status = protocolRespEntry.Wait(B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, HV_HID_TIMEOUT_US);
	if (status != B_OK)
		return status;

	TRACE("HID protocol version %u.%u status %u\n",
		GET_HID_VERSION_MAJOR(fProtocolResponse.version),
		GET_HID_VERSION_MINOR(fProtocolResponse.version), fProtocolResponse.result);

	status = deviceInfoEntry.Wait(B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, HV_HID_TIMEOUT_US);
	if (status != B_OK)
		return status;

	message.pipe_header.type = HV_HID_PIPE_MSGTYPE_DATA;
	message.pipe_header.length = sizeof(message.dev_info_ack);
	message.header.type = HV_HID_MSGTYPE_INITIAL_DEV_INFO_ACK;
	message.header.length = message.pipe_header.length - sizeof(message.header);
	message.dev_info_ack.reserved = 0;

	// Send device info acknowledgement to Hyper-V
	status = fHyperV->write_packet(fHyperVCookie, VMBUS_PKTTYPE_DATA_INBAND, &message,
		sizeof(message.pipe_header) + message.pipe_header.length, FALSE, HV_HID_REQUEST_TRANS_ID);
	if (status != B_OK) {
		ERROR("Failed to send HID device info ack\n");
		return status;
	}

	TRACE("Hyper-V HID vid 0x%04X pid 0x%04X version 0x%X\n", fDeviceInfo->info.vendor_id,
		fDeviceInfo->info.product_id, fDeviceInfo->info.version);

	status = fParser.ParseReportDescriptor(fDeviceInfo->descriptor_data,
		fDeviceInfo->descriptor.hid_descriptor_length);
	free(fDeviceInfo);
	fDeviceInfo = NULL;

	HIDReport* report = fParser.ReportAt(HID_REPORT_TYPE_INPUT, 0);
	if (report == NULL)
		return B_ERROR;

#ifdef TRACE_HYPERV_HID
	report->PrintToStream();
#endif

	// Check report for wheel item, if not present add a wheel item
	HIDReportItem* wheelItem = report->FindItem(B_HID_USAGE_PAGE_GENERIC_DESKTOP,
		B_HID_UID_GD_WHEEL);
	if (wheelItem == NULL) {
		TRACE("No wheel found in HID report descriptor, adding\n");
		status = _AddWheelHIDItem(report);
		if (status != B_OK)
			ERROR("Failed to add wheel device to HID descriptor (%s)\n", strerror(status));
	}

	return B_OK;
}


status_t
HIDDevice::_AddWheelHIDItem(HIDReport* report)
{
	HIDCollection* rootCollection = fParser.RootCollection();
	if (rootCollection == NULL)
		return B_ERROR;

	HIDCollection* mouseCollection = NULL;
	for (uint32 i = 0; i < rootCollection->CountChildrenFlat(COLLECTION_APPLICATION); i++) {
		HIDCollection* collection = rootCollection->ChildAtFlat(COLLECTION_APPLICATION, i);
		if (collection != NULL && (collection->UsageID() == B_HID_UID_GD_MOUSE
				|| collection->UsageID() == B_HID_UID_GD_POINTER)) {
			mouseCollection = collection;
			break;
		}
	}

	if (mouseCollection == NULL)
		return B_ERROR;

	global_item_state globalState;
	memset(&globalState, 0, sizeof(global_item_state));

	local_item_state localState;
	memset((void*)&localState, 0, sizeof(local_item_state));

	main_item_data_converter converter;
	converter.flat_data = 0;
	converter.main_data.array_variable = 1;

	globalState.logical_minimum = -127;
	globalState.logical_maximum = 127;
	globalState.report_count = 1;
	globalState.report_size = 8;
	globalState.usage_page = B_HID_USAGE_PAGE_GENERIC_DESKTOP;

	usage_value usageValue;
	usageValue.u.s.usage_page = B_HID_USAGE_PAGE_GENERIC_DESKTOP;
	usageValue.u.s.usage_id = B_HID_UID_GD_WHEEL;

	localState.usage_stack_used = 1;
	localState.usage_stack = &usageValue;

	report->AddMainItem(globalState, localState, converter.main_data, mouseCollection);

#ifdef TRACE_HYPERV_HID
	report->PrintToStream();
#endif

	return B_OK;
}
