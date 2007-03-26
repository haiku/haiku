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
