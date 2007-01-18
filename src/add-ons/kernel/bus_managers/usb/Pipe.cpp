/*
 * Copyright 2004-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 */

#include "usb_p.h"


Pipe::Pipe(Object *parent, int8 deviceAddress, uint8 endpointAddress,
	pipeDirection direction, usb_speed speed, size_t maxPacketSize)
	:	Object(parent),
		fDeviceAddress(deviceAddress),
		fEndpointAddress(endpointAddress),
		fDirection(direction),
		fSpeed(speed),
		fMaxPacketSize(maxPacketSize),
		fDataToggle(false)
{
	GetBusManager()->NotifyPipeChange(this, USB_CHANGE_CREATED);
}


Pipe::~Pipe()
{
	GetBusManager()->NotifyPipeChange(this, USB_CHANGE_DESTROYED);
}


status_t
Pipe::SubmitTransfer(Transfer *transfer)
{
	// ToDo: keep track of all submited transfers to be able to cancel them
	return GetBusManager()->SubmitTransfer(transfer);
}


status_t
Pipe::CancelQueuedTransfers()
{
	TRACE_ERROR(("USB Pipe: cancelling transfers is not implemented!\n"));
	return B_ERROR;
}


status_t
Pipe::SetFeature(uint16 selector)
{
	return ((Device *)Parent())->DefaultPipe()->SendRequest(
		USB_REQTYPE_STANDARD | USB_REQTYPE_ENDPOINT_OUT,
		USB_REQUEST_SET_FEATURE,
		selector,
		fEndpointAddress,
		0,
		NULL,
		0,
		NULL);
}


status_t
Pipe::ClearFeature(uint16 selector)
{
	// clearing a stalled condition resets the data toggle
	if (selector == USB_FEATURE_ENDPOINT_HALT)
		SetDataToggle(false);

	return ((Device *)Parent())->DefaultPipe()->SendRequest(
		USB_REQTYPE_STANDARD | USB_REQTYPE_ENDPOINT_OUT,
		USB_REQUEST_CLEAR_FEATURE,
		selector,
		fEndpointAddress,
		0,
		NULL,
		0,
		NULL);
}


status_t
Pipe::GetStatus(uint16 *status)
{
	return ((Device *)Parent())->DefaultPipe()->SendRequest(
		USB_REQTYPE_STANDARD | USB_REQTYPE_ENDPOINT_IN,
		USB_REQUEST_GET_STATUS,
		0,
		fEndpointAddress,
		2,
		(void *)status,
		2,
		NULL);
}


//
// #pragma mark -
//


InterruptPipe::InterruptPipe(Object *parent, int8 deviceAddress,
	uint8 endpointAddress, pipeDirection direction, usb_speed speed,
	size_t maxPacketSize)
	:	Pipe(parent, deviceAddress, endpointAddress, direction, speed,
			maxPacketSize)
{
}


status_t
InterruptPipe::QueueInterrupt(void *data, size_t dataLength,
	usb_callback_func callback, void *callbackCookie)
{
	Transfer *transfer = new(std::nothrow) Transfer(this);
	if (!transfer)
		return B_NO_MEMORY;

	transfer->SetData((uint8 *)data, dataLength);
	transfer->SetCallback(callback, callbackCookie);

	status_t result = GetBusManager()->SubmitTransfer(transfer);
	if (result < B_OK)
		delete transfer;
	return result;
}


//
// #pragma mark -
//


BulkPipe::BulkPipe(Object *parent, int8 deviceAddress, uint8 endpointAddress,
	pipeDirection direction, usb_speed speed, size_t maxPacketSize)
	:	Pipe(parent, deviceAddress, endpointAddress, direction, speed,
			maxPacketSize)
{
}


status_t
BulkPipe::QueueBulk(void *data, size_t dataLength, usb_callback_func callback,
	void *callbackCookie)
{
	Transfer *transfer = new(std::nothrow) Transfer(this);
	if (!transfer)
		return B_NO_MEMORY;

	transfer->SetData((uint8 *)data, dataLength);
	transfer->SetCallback(callback, callbackCookie);

	status_t result = GetBusManager()->SubmitTransfer(transfer);
	if (result < B_OK)
		delete transfer;
	return result;
}


status_t
BulkPipe::QueueBulkV(iovec *vector, size_t vectorCount,
	usb_callback_func callback, void *callbackCookie)
{
	Transfer *transfer = new(std::nothrow) Transfer(this);
	if (!transfer)
		return B_NO_MEMORY;

	transfer->SetVector(vector, vectorCount);
	transfer->SetCallback(callback, callbackCookie);

	status_t result = GetBusManager()->SubmitTransfer(transfer);
	if (result < B_OK)
		delete transfer;
	return result;
}


//
// #pragma mark -
//


IsochronousPipe::IsochronousPipe(Object *parent, int8 deviceAddress,
	uint8 endpointAddress, pipeDirection direction, usb_speed speed,
	size_t maxPacketSize)
	:	Pipe(parent, deviceAddress, endpointAddress, direction, speed,
			maxPacketSize),
		fMaxQueuedPackets(0),
		fMaxBufferDuration(0),
		fSampleSize(0)
{
}


status_t
IsochronousPipe::QueueIsochronous(void *data, size_t dataLength,
	usb_iso_packet_descriptor *packetDesc, uint32 packetCount,
	uint32 *startingFrameNumber, uint32 flags, usb_callback_func callback,
	void *callbackCookie)
{
	return B_ERROR;
}


status_t
IsochronousPipe::SetPipePolicy(uint8 maxQueuedPackets,
	uint16 maxBufferDurationMS, uint16 sampleSize)
{
	if (maxQueuedPackets == fMaxQueuedPackets
		|| maxBufferDurationMS == fMaxBufferDuration
		|| sampleSize == fSampleSize)
		return B_OK;

	fMaxQueuedPackets = maxQueuedPackets;
	fMaxBufferDuration = maxBufferDurationMS;
	fSampleSize = sampleSize;

	GetBusManager()->NotifyPipeChange(this, USB_CHANGE_PIPE_POLICY_CHANGED);
	return B_OK;
}


status_t
IsochronousPipe::GetPipePolicy(uint8 *maxQueuedPackets,
	uint16 *maxBufferDurationMS, uint16 *sampleSize)
{
	if (maxQueuedPackets)
		*maxQueuedPackets = fMaxQueuedPackets;
	if (maxBufferDurationMS)
		*maxBufferDurationMS = fMaxBufferDuration;
	if (sampleSize)
		*sampleSize = fSampleSize;
	return B_OK;
}


//
// #pragma mark -
//


typedef struct transfer_result_data_s {
	sem_id		notify_sem;
	status_t	status;
	size_t		actual_length;
} transfer_result_data;


ControlPipe::ControlPipe(Object *parent, int8 deviceAddress,
	uint8 endpointAddress, usb_speed speed, size_t maxPacketSize)
	:	Pipe(parent, deviceAddress, endpointAddress, Default, speed,
			maxPacketSize)
{
}


status_t
ControlPipe::SendRequest(uint8 requestType, uint8 request, uint16 value,
	uint16 index, uint16 length, void *data, size_t dataLength,
	size_t *actualLength)
{
	transfer_result_data transferResult;
	transferResult.notify_sem = create_sem(0, "usb send request notify");
	if (transferResult.notify_sem < B_OK)
		return B_NO_MORE_SEMS;

	status_t result = QueueRequest(requestType, request, value, index, length,
		data, dataLength, SendRequestCallback, &transferResult);
	if (result < B_OK) {
		delete_sem(transferResult.notify_sem);
		return result;
	}

	// the sem will be released in the callback after the result data was
	// filled into the provided struct. use a 5 seconds timeout to avoid
	// hanging applications.
	if (acquire_sem_etc(transferResult.notify_sem, 1, B_RELATIVE_TIMEOUT, 5000000) < B_OK) {
		TRACE_ERROR(("USB ControlPipe: timeout waiting for queued request to complete\n"));

		delete_sem(transferResult.notify_sem);
		if (actualLength)
			*actualLength = 0;

		// ToDo: cancel the transfer at the bus manager
		return B_TIMED_OUT;
	}

	delete_sem(transferResult.notify_sem);
	if (actualLength)
		*actualLength = transferResult.actual_length;

	return transferResult.status;
}


void
ControlPipe::SendRequestCallback(void *cookie, status_t status, void *data,
	size_t actualLength)
{
	transfer_result_data *transferResult = (transfer_result_data *)cookie;
	transferResult->status = status;
	transferResult->actual_length = actualLength;
	release_sem(transferResult->notify_sem);
}


status_t
ControlPipe::QueueRequest(uint8 requestType, uint8 request, uint16 value,
	uint16 index, uint16 length, void *data, size_t dataLength,
	usb_callback_func callback, void *callbackCookie)
{
	usb_request_data *requestData = new(std::nothrow) usb_request_data;
	if (!requestData)
		return B_NO_MEMORY;

	requestData->RequestType = requestType;
	requestData->Request = request;
	requestData->Value = value;
	requestData->Index = index;
	requestData->Length = length;

	Transfer *transfer = new(std::nothrow) Transfer(this);
	if (!transfer) {
		delete requestData;
		return B_NO_MEMORY;
	}

	transfer->SetRequestData(requestData);
	transfer->SetData((uint8 *)data, dataLength);
	transfer->SetCallback(callback, callbackCookie);

	status_t result = GetBusManager()->SubmitTransfer(transfer);
	if (result < B_OK)
		delete transfer;
	return result;
}
