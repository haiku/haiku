/*
 * Copyright 2004-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels S. Reedijk
 */

#include "usb_p.h"


Pipe::Pipe(Device *device, pipeDirection &direction, uint8 &endpointAddress)
{
	fDirection = direction;
	fDevice = device;
	fEndpoint = endpointAddress;

	fBus = NULL;
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


//
// #pragma mark -
//


ControlPipe::ControlPipe(Device *device, pipeDirection direction,
	pipeSpeed speed, uint8 endpointAddress)
	:	Pipe(device, direction, endpointAddress)
{
	fSpeed = speed;
}


ControlPipe::ControlPipe(BusManager *bus, int8 deviceAddress,
	pipeDirection direction, pipeSpeed speed, uint8 endpointAddress)
	:	Pipe(NULL, direction, endpointAddress)
{
	fBus = bus;
	fDeviceAddress = deviceAddress;
	fSpeed = speed;
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

	return SendControlMessage(&requestData, data, dataLength, actualLength,
		3 * 1000 * 1000);
}


status_t
ControlPipe::SendControlMessage(usb_request_data *command, void *data,
	size_t dataLength, size_t *actualLength, bigtime_t timeout)
{
	// this method should build an usb packet (new class) with the needed data
	Transfer *transfer = new Transfer(this, true);

	transfer->SetRequestData(command);
	transfer->SetBuffer((uint8 *)data);
	transfer->SetBufferLength(dataLength);
	transfer->SetActualLength(actualLength);

	return fBus->SubmitTransfer(transfer, timeout);
}
