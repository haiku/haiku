/*
 * Copyright 2003-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Niels S. Reedijk
 */

#include "usb_p.h"


Transfer::Transfer(Pipe *pipe)
	:	fPipe(pipe),
		fVector(&fData),
		fVectorCount(0),
		fBaseAddress(NULL),
		fFragmented(false),
		fActualLength(0),
		fUserArea(-1),
		fClonedArea(-1),
		fCallback(NULL),
		fCallbackCookie(NULL),
		fRequestData(NULL)
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
Transfer::SetData(uint8 *data, size_t dataLength)
{
	fBaseAddress = data;
	fData.iov_base = data;
	fData.iov_len = dataLength;

	if (data && dataLength > 0)
		fVectorCount = 1;
}


void
Transfer::SetVector(iovec *vector, size_t vectorCount)
{
	fVector = new(std::nothrow) iovec[vectorCount];
	memcpy(fVector, vector, vectorCount * sizeof(iovec));
	fVectorCount = vectorCount;
	fBaseAddress = fVector[0].iov_base;
}


size_t
Transfer::VectorLength()
{
	size_t length = 0;
	for (size_t i = 0; i < fVectorCount; i++)
		length += fVector[i].iov_len;

	// the data is to large and would overflow the allocator
	if (length > USB_MAX_FRAGMENT_SIZE) {
		length = USB_MAX_FRAGMENT_SIZE;
		fFragmented = true;
	}

	return length;
}


void
Transfer::AdvanceByFragment(size_t actualLength)
{
	size_t length = USB_MAX_FRAGMENT_SIZE;
	for (size_t i = 0; i < fVectorCount; i++) {
		if (fVector[i].iov_len <= length) {
			length -= fVector[i].iov_len;
			fVector[i].iov_len = 0;
			continue;
		}

		fVector[i].iov_base = (void *)((uint8 *)fVector[i].iov_base + length);
		fVector[i].iov_len -= length;
		break;
	}

	fActualLength += actualLength;
}


status_t
Transfer::InitKernelAccess()
{
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
	// we might need to access a buffer in userspace. this will not
	// be possible in the kernel space finisher thread unless we
	// get the proper area id for the space we need and then clone it
	// before reading from or writing to it.
	iovec *vector = fVector;
	for (size_t i = 0; i < fVectorCount; i++) {
		if (IS_USER_ADDRESS(vector[i].iov_base)) {
			fUserArea = area_for(vector[i].iov_base);
			if (fUserArea < B_OK)
				return B_BAD_ADDRESS;
			break;
		}
	}

	// no user area required for access
	if (fUserArea < B_OK)
		return B_OK;

	area_info areaInfo;
	if (fUserArea < B_OK || get_area_info(fUserArea, &areaInfo) < B_OK)
		return B_BAD_ADDRESS;

	for (size_t i = 0; i < fVectorCount; i++) {
		(uint8 *)vector[i].iov_base -= (uint8 *)areaInfo.address;

		if ((size_t)vector[i].iov_base > areaInfo.size
			|| (size_t)vector[i].iov_base + vector[i].iov_len > areaInfo.size) {
			TRACE_ERROR(("USB Transfer: data buffer spans across multiple areas!\n"));
			return B_BAD_ADDRESS;
		}
	}
#endif // !HAIKU_TARGET_PLATFORM_HAIKU

	return B_OK;
}


status_t
Transfer::PrepareKernelAccess()
{
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
	// done if there is no userspace buffer or if we already cloned its area
	if (fUserArea < B_OK || fClonedArea >= B_OK)
		return B_OK;

	void *clonedMemory = NULL;
	// we got a userspace buffer, need to clone the area for that
	// space first and map the iovecs to this cloned area.
	fClonedArea = clone_area("userspace accessor", &clonedMemory,
		B_ANY_ADDRESS, B_WRITE_AREA | B_KERNEL_WRITE_AREA, fUserArea);

	if (fClonedArea < B_OK)
		return fClonedArea;

	for (size_t i = 0; i < fVectorCount; i++)
		(uint8 *)fVector[i].iov_base += (addr_t)clonedMemory;
#endif // !HAIKU_TARGET_PLATFORM_HAIKU

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
