/*
 * Copyright 2004-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Niels S. Reedijk
 */

#include "usb_p.h"


Pipe::Pipe(Device *device, pipeDirection direction, uint8 endpointAddress,
	uint32 maxPacketSize)
{
	fDirection = direction;
	fDevice = device;
	fEndpoint = endpointAddress;
	fMaxPacketSize = maxPacketSize;

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
	pipeSpeed speed, uint8 endpointAddress, uint32 maxPacketSize)
	:	Pipe(device, direction, endpointAddress, maxPacketSize)
{
	fSpeed = speed;
}


ControlPipe::ControlPipe(BusManager *bus, int8 deviceAddress,
	pipeDirection direction, pipeSpeed speed, uint8 endpointAddress,
	uint32 maxPacketSize)
	:	Pipe(NULL, direction, endpointAddress, maxPacketSize)
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
	// builds an usb packet with the needed data
	Transfer transfer(this, true);

	transfer.SetRequestData(command);
	transfer.SetBuffer((uint8 *)data);
	transfer.SetBufferLength(dataLength);
	transfer.SetActualLength(actualLength);

	fBus->SubmitTransfer(&transfer);
	return transfer.WaitForFinish();
}
