/*
 * Copyright 2004-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 */

#include "usb_p.h"


Pipe::Pipe(Device *device, pipeDirection direction, pipeSpeed speed,
	uint8 endpointAddress, uint32 maxPacketSize)
	:	fDevice(device),
		fBus(NULL),
		fDirection(direction),
		fSpeed(speed),
		fEndpoint(endpointAddress),
		fMaxPacketSize(maxPacketSize),
		fDataToggle(false)
{
	if (fDevice)
		fBus = fDevice->Manager();
}


Pipe::~Pipe()
{
}


int8
Pipe::DeviceAddress()
{
	if (!fDevice)
		return -1;

	return fDevice->Address();
}


status_t
Pipe::SubmitTransfer(Transfer *transfer)
{
	// ToDo: keep track of all submited transfers to be able to cancel them
	return fBus->SubmitTransfer(transfer);
}


status_t
Pipe::CancelQueuedTransfers()
{
	return B_ERROR;
}


//
// #pragma mark -
//


InterruptPipe::InterruptPipe(Device *device, pipeDirection direction,
	pipeSpeed speed, uint8 endpointAddress, uint32 maxPacketSize)
	:	Pipe(device, direction, speed, endpointAddress, maxPacketSize)
{
}


status_t
InterruptPipe::QueueInterrupt(void *data, size_t dataLength,
	usb_callback_func callback, void *callbackCookie)
{
	Transfer *transfer = new(std::nothrow) Transfer(this, false);
	if (!transfer)
		return B_NO_MEMORY;

	transfer->SetData((uint8 *)data, dataLength);
	transfer->SetCallback(callback, callbackCookie);

	status_t result = SubmitTransfer(transfer);
	if (result == B_OK || result == EINPROGRESS)
		return B_OK;

	delete transfer;
	return result;
}


//
// #pragma mark -
//


BulkPipe::BulkPipe(Device *device, pipeDirection direction,
	pipeSpeed speed, uint8 endpointAddress, uint32 maxPacketSize)
	:	Pipe(device, direction, speed, endpointAddress, maxPacketSize)
{
}


status_t
BulkPipe::QueueBulk(void *data, size_t dataLength, usb_callback_func callback,
	void *callbackCookie)
{
	Transfer *transfer = new(std::nothrow) Transfer(this, false);
	if (!transfer)
		return B_NO_MEMORY;

	transfer->SetData((uint8 *)data, dataLength);
	transfer->SetCallback(callback, callbackCookie);

	status_t result = SubmitTransfer(transfer);
	if (result == B_OK || result == EINPROGRESS)
		return B_OK;

	delete transfer;
	return result;
}


//
// #pragma mark -
//


IsochronousPipe::IsochronousPipe(Device *device, pipeDirection direction,
	pipeSpeed speed, uint8 endpointAddress, uint32 maxPacketSize)
	:	Pipe(device, direction, speed, endpointAddress, maxPacketSize)
{
}


status_t
IsochronousPipe::QueueIsochronous(void *data, size_t dataLength,
	rlea *rleArray, uint16 bufferDurationMS, usb_callback_func callback,
	void *callbackCookie)
{
	return B_ERROR;
}


//
// #pragma mark -
//


ControlPipe::ControlPipe(Device *device, pipeSpeed speed,
	uint8 endpointAddress, uint32 maxPacketSize)
	:	Pipe(device, Pipe::Default, speed, endpointAddress, maxPacketSize)
{
}


ControlPipe::ControlPipe(BusManager *bus, int8 deviceAddress, pipeSpeed speed,
	uint8 endpointAddress, uint32 maxPacketSize)
	:	Pipe(NULL, Pipe::Default, speed, endpointAddress, maxPacketSize),
		fDeviceAddress(deviceAddress)
{
	fBus = bus;
}


int8
ControlPipe::DeviceAddress()
{
	if (!fDevice)
		return fDeviceAddress;

	return fDevice->Address();
}


status_t
ControlPipe::SendRequest(uint8 requestType, uint8 request, uint16 value,
	uint16 index, uint16 length, void *data, size_t dataLength,
	size_t *actualLength)
{
	usb_request_data requestData;
	requestData.RequestType = requestType;
	requestData.Request = request;
	requestData.Value = value;
	requestData.Index = index;
	requestData.Length = length;

	Transfer *transfer = new(std::nothrow) Transfer(this, true);
	if (!transfer)
		return B_NO_MEMORY;

	transfer->SetRequestData(&requestData);
	transfer->SetData((uint8 *)data, dataLength);
	transfer->SetActualLength(actualLength);

	status_t result = SubmitTransfer(transfer);
	if (result == EINPROGRESS)
		return transfer->WaitForFinish();

	if (result < B_OK)
		delete transfer;
	return result;
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

	Transfer *transfer = new(std::nothrow) Transfer(this, false);
	if (!transfer) {
		delete requestData;
		return B_NO_MEMORY;
	}

	transfer->SetRequestData(requestData);
	transfer->SetData((uint8 *)data, dataLength);
	transfer->SetCallback(callback, callbackCookie);

	status_t result = SubmitTransfer(transfer);
	if (result == B_OK || result == EINPROGRESS)
		return B_OK;

	delete transfer;
	return result;
}


status_t
ControlPipe::SetFeature(uint16 selector)
{
	return SendRequest(
		USB_REQTYPE_STANDARD | USB_REQTYPE_ENDPOINT_OUT,
		USB_REQUEST_SET_FEATURE,
		selector,
		0,
		0,
		NULL,
		0,
		NULL);
}


status_t
ControlPipe::ClearFeature(uint16 selector)
{
	return SendRequest(
		USB_REQTYPE_STANDARD | USB_REQTYPE_ENDPOINT_OUT,
		USB_REQUEST_CLEAR_FEATURE,
		selector,
		0,
		0,
		NULL,
		0,
		NULL);
}


status_t
ControlPipe::GetStatus(uint16 *status)
{
	return SendRequest(
		USB_REQTYPE_STANDARD | USB_REQTYPE_ENDPOINT_IN,
		USB_REQUEST_GET_STATUS,
		0,
		0,
		2,
		(void *)status,
		2,
		NULL);
}
