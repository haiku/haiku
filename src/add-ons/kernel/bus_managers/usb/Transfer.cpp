/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 */


#include "usb_private.h"

#include <kernel.h>


Transfer::Transfer(Pipe *pipe)
	:	fPipe(pipe),
		fVector(&fData),
		fVectorCount(0),
		fBaseAddress(NULL),
		fPhysical(false),
		fFragmented(false),
		fActualLength(0),
		fUserArea(-1),
		fClonedArea(-1),
		fCallback(NULL),
		fCallbackCookie(NULL),
		fRequestData(NULL),
		fIsochronousData(NULL),
		fBandwidth(0)
{
}


Transfer::~Transfer()
{
	// we take ownership of the request data
	if (fRequestData)
		delete fRequestData;

	if (fVector && fVector != &fData)
		delete[] fVector;

	if (fClonedArea >= B_OK)
		delete_area(fClonedArea);
}


void
Transfer::SetRequestData(usb_request_data *data)
{
	fRequestData = data;
}


void
Transfer::SetIsochronousData(usb_isochronous_data *data)
{
	fIsochronousData = data;
}


void
Transfer::SetData(uint8 *data, size_t dataLength)
{
	fPhysical = false;
	fBaseAddress = data;
	fData.base = (generic_addr_t)data;
	fData.length = dataLength;

	if (data && dataLength > 0)
		fVectorCount = 1;

	fFragmented = dataLength > USB_MAX_FRAGMENT_SIZE;

	// Calculate the bandwidth (only if it is not a bulk transfer)
	if (!(fPipe->Type() & USB_OBJECT_BULK_PIPE)) {
		if (_CalculateBandwidth() < B_OK)
			TRACE_ERROR("can't calculate bandwidth\n");
	}
}


void
Transfer::SetVector(iovec *vector, size_t vectorCount)
{
	fPhysical = false;

	fVector = new(std::nothrow) generic_io_vec[vectorCount];
	for (size_t i = 0; i < vectorCount; i++) {
		fVector[i].base = (generic_addr_t)vector[i].iov_base;
		fVector[i].length = vector[i].iov_len;
	}
	fVectorCount = vectorCount;
	fBaseAddress = vector[0].iov_base;

	_CheckFragmented();
}


void
Transfer::SetVector(physical_entry *vector, size_t vectorCount)
{
	fPhysical = true;

	fVector = new(std::nothrow) generic_io_vec[vectorCount];
	for (size_t i = 0; i < vectorCount; i++) {
		fVector[i].base = (generic_addr_t)vector[i].address;
		fVector[i].length = vector[i].size;
	}
	fVectorCount = vectorCount;
	fBaseAddress = NULL;

	_CheckFragmented();
}


void
Transfer::_CheckFragmented()
{
	size_t length = 0;
	for (size_t i = 0; i < fVectorCount && length <= USB_MAX_FRAGMENT_SIZE; i++)
		length += fVector[i].length;

	fFragmented = length > USB_MAX_FRAGMENT_SIZE;
}


size_t
Transfer::FragmentLength() const
{
	size_t length = 0;
	for (size_t i = 0; i < fVectorCount; i++)
		length += fVector[i].length;

	if (length > USB_MAX_FRAGMENT_SIZE)
		length = USB_MAX_FRAGMENT_SIZE;

	return length;
}


void
Transfer::AdvanceByFragment(size_t actualLength)
{
	size_t length = USB_MAX_FRAGMENT_SIZE;
	for (size_t i = 0; i < fVectorCount; i++) {
		if (fVector[i].length <= length) {
			length -= fVector[i].length;
			fVector[i].length = 0;
			continue;
		}

		fVector[i].base = fVector[i].base + length;
		fVector[i].length -= length;
		break;
	}

	fActualLength += actualLength;
}


status_t
Transfer::InitKernelAccess()
{
	// nothing to do if we are already prepared, or have a physical request
	if (fClonedArea >= B_OK || fPhysical)
		return B_OK;

	// we might need to access a buffer in userspace. this will not
	// be possible in the kernel space finisher thread unless we
	// get the proper area id for the space we need and then clone it
	// before reading from or writing to it.
	generic_io_vec *vector = fVector;
	for (size_t i = 0; i < fVectorCount; i++) {
		if (IS_USER_ADDRESS(vector[i].base)) {
			fUserArea = area_for((void*)vector[i].base);
			if (fUserArea < B_OK) {
				TRACE_ERROR("failed to find area for user space buffer!\n");
				return B_BAD_ADDRESS;
			}
			break;
		}
	}

	// no user area required for access
	if (fUserArea < B_OK)
		return B_OK;

	area_info areaInfo;
	if (fUserArea < B_OK || get_area_info(fUserArea, &areaInfo) < B_OK) {
		TRACE_ERROR("couldn't get user area info\n");
		return B_BAD_ADDRESS;
	}

	for (size_t i = 0; i < fVectorCount; i++) {
		vector[i].base = vector[i].base - (addr_t)areaInfo.address;

		if (vector[i].base > areaInfo.size
				|| (vector[i].base + vector[i].length) > areaInfo.size) {
			TRACE_ERROR("data buffer spans across multiple areas!\n");
			return B_BAD_ADDRESS;
		}
	}
	return B_OK;
}


status_t
Transfer::PrepareKernelAccess()
{
	// done if there is no userspace buffer or if we already cloned its area
	if (fUserArea < B_OK || fClonedArea >= B_OK)
		return B_OK;

	void *clonedMemory = NULL;
	// we got a userspace buffer, need to clone the area for that
	// space first and map the iovecs to this cloned area.
	fClonedArea = clone_area("userspace accessor", &clonedMemory,
		B_ANY_ADDRESS, B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA, fUserArea);

	if (fClonedArea < B_OK)
		return fClonedArea;

	for (size_t i = 0; i < fVectorCount; i++)
		fVector[i].base = fVector[i].base + (addr_t)clonedMemory;
	return B_OK;
}


void
Transfer::SetCallback(usb_callback_func callback, void *cookie)
{
	fCallback = callback;
	fCallbackCookie = cookie;
}


void
Transfer::Finished(uint32 status, size_t actualLength)
{
	if (fCallback)
		fCallback(fCallbackCookie, status, fBaseAddress,
			fActualLength + actualLength);
}


/*
 * USB 2.0 Spec function, pag 64.
 * This function sets fBandwidth in microsecond
 * to the bandwidth needed to transfer fData.iov_len bytes.
 * The calculation is based on
 * 1. Speed of the transfer
 * 2. Pipe direction
 * 3. Type of pipe
 * 4. Number of bytes to transfer
 */
status_t
Transfer::_CalculateBandwidth()
{
	uint16 bandwidthNS;
	uint32 type = fPipe->Type();

	switch (fPipe->Speed()) {
		case USB_SPEED_HIGHSPEED:
		{
			// Direction doesn't matter for highspeed
			if (type & USB_OBJECT_ISO_PIPE)
				bandwidthNS = (uint16)((38 * 8 * 2.083)
					+ (2.083 * ((uint32)(3.167 * (1.1667 * 8 * fData.length))))
					+ USB_BW_HOST_DELAY);
			else
				bandwidthNS = (uint16)((55 * 8 * 2.083)
					+ (2.083 * ((uint32)(3.167 * (1.1667 * 8 * fData.length))))
					+ USB_BW_HOST_DELAY);
			break;
		}
		case USB_SPEED_FULLSPEED:
		{
			// Direction does matter this time for isochronous
			if (type & USB_OBJECT_ISO_PIPE)
				bandwidthNS = (uint16)
					(((fPipe->Direction() == Pipe::In) ? 7268 : 6265)
					+ (83.54 * ((uint32)(3.167 + (1.1667 * 8 * fData.length))))
					+ USB_BW_HOST_DELAY);
			else
				bandwidthNS = (uint16)(9107
					+ (83.54 * ((uint32)(3.167 + (1.1667 * 8 * fData.length))))
					+ USB_BW_HOST_DELAY);
			break;
		}
		case USB_SPEED_LOWSPEED:
		{
			if (fPipe->Direction() == Pipe::In)
				bandwidthNS = (uint16) (64060 + (2 * USB_BW_SETUP_LOW_SPEED_PORT_DELAY)
					+ (676.67 * ((uint32)(3.167 + (1.1667 * 8 * fData.length))))
					+ USB_BW_HOST_DELAY);
			else
				bandwidthNS = (uint16)(64107 + (2 * USB_BW_SETUP_LOW_SPEED_PORT_DELAY)
					+ (667.0 * ((uint32)(3.167 + (1.1667 * 8 * fData.length))))
					+ USB_BW_HOST_DELAY);
			break;
		}

		case USB_SPEED_SUPERSPEED:
		case USB_SPEED_SUPERSPEEDPLUS:
		{
			// TODO it should only be useful for isochronous type
			bandwidthNS = 0;
			break;
		}

		default:
			// We should never get here
			TRACE("speed unknown");
			return B_ERROR;
	}

	// Round up and set the value in microseconds
	fBandwidth = (bandwidthNS + 500) / 1000;

	return B_OK;
}
