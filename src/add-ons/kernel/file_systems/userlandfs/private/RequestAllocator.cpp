/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <stdlib.h>

#include "AreaSupport.h"
#include "Compatibility.h"
#include "Debug.h"
#include "Port.h"
#include "RequestAllocator.h"


// constructor
RequestAllocator::RequestAllocator(Port* port)
	:
	fError(B_NO_INIT),
	fPort(NULL),
	fRequest(NULL),
	fRequestSize(0),
	fPortReservedOffset(0),
	fAllocatedAreaCount(0),
	fDeferredInitInfoCount(0),
	fRequestInPortBuffer(false)
{
	Init(port);
}

// destructor
RequestAllocator::~RequestAllocator()
{
	Uninit();
}

// Init
status_t
RequestAllocator::Init(Port* port)
{
	Uninit();
	if (port) {
		fPort = port;
		fError = fPort->InitCheck();
		fPortReservedOffset = fPort->ReservedSize();
	}
	return fError;
}

// Uninit
void
RequestAllocator::Uninit()
{
	if (fRequestInPortBuffer)
		fPort->Unreserve(fPortReservedOffset);
	else
		free(fRequest);

	for (int32 i = 0; i < fAllocatedAreaCount; i++)
		delete_area(fAllocatedAreas[i]);
	fAllocatedAreaCount = 0;

	for (int32 i = 0; i < fDeferredInitInfoCount; i++) {
		if (fDeferredInitInfos[i].inPortBuffer)
			free(fDeferredInitInfos[i].data);
	}

	fDeferredInitInfoCount = 0;
	fError = B_NO_INIT;
	fPort = NULL;
	fRequest = NULL;
	fRequestSize = 0;
	fPortReservedOffset = 0;
}

// Error
status_t
RequestAllocator::Error() const
{
	return fError;
}

// FinishDeferredInit
void
RequestAllocator::FinishDeferredInit()
{
	if (fError != B_OK)
		return;
	for (int32 i = 0; i < fDeferredInitInfoCount; i++) {
		DeferredInitInfo& info = fDeferredInitInfos[i];
		if (info.inPortBuffer) {
			if (info.size > 0)
				memcpy((uint8*)fRequest + info.offset, info.data, info.size);
			free(info.data);
		}
		PRINT(("RequestAllocator::FinishDeferredInit(): area: %ld, "
			"offset: %ld, size: %ld\n", info.area, info.offset, info.size));
		info.target->SetTo(info.area, info.offset, info.size);
	}
	fDeferredInitInfoCount = 0;
}

// AllocateRequest
status_t
RequestAllocator::AllocateRequest(int32 size)
{
	if (fError != B_OK)
		RETURN_ERROR(fError);

	fRequestOffset = (fPortReservedOffset + 7) / 8 * 8;

	if (size < (int32)sizeof(Request)
		|| fRequestOffset + size > fPort->GetCapacity()) {
		RETURN_ERROR(fError = B_BAD_VALUE);
	}

	fRequest = (Request*)((uint8*)fPort->GetBuffer() + fRequestOffset);
	fRequestSize = size;
	fRequestInPortBuffer = true;
	fPort->Reserve(fRequestOffset + fRequestSize);
	return B_OK;
}

// ReadRequest
status_t
RequestAllocator::ReadRequest(bigtime_t timeout)
{
	if (fError != B_OK)
		RETURN_ERROR(fError);

	// read the message from the port
	void* message;
	size_t messageSize;
	status_t error = fPort->Receive(&message, &messageSize, timeout);
	if (error != B_OK) {
		if (error != B_TIMED_OUT && error != B_WOULD_BLOCK)
			RETURN_ERROR(fError = error);
		return error;
	}

	// shouldn't be shorter than the base Request
	if (messageSize < (int32)sizeof(Request)) {
		free(message);
		RETURN_ERROR(fError = B_BAD_DATA);
	}

	// init the request
	fRequest = (Request*)message;
	fRequestOffset = 0;
	fRequestSize = messageSize;
	fRequestInPortBuffer = false;

	// relocate the request
	fError = relocate_request(fRequest, fRequestSize, fAllocatedAreas,
		&fAllocatedAreaCount);
	RETURN_ERROR(fError);
}

// GetRequest
Request*
RequestAllocator::GetRequest() const
{
	return fRequest;
}

// GetRequestSize
int32
RequestAllocator::GetRequestSize() const
{
	return fRequestSize;
}

// AllocateAddress
status_t
RequestAllocator::AllocateAddress(Address& address, int32 size, int32 align,
	void** data, bool deferredInit)
{
	if (fError != B_OK)
		return fError;
	if (!fRequest)
		RETURN_ERROR(B_NO_INIT);
	if (size < 0)
		RETURN_ERROR(B_BAD_VALUE);
	if (fDeferredInitInfoCount >= MAX_REQUEST_ADDRESS_COUNT)
		RETURN_ERROR(B_BAD_VALUE);
	// fix the alignment -- valid is 1, 2, 4, 8
	if (align <= 0 || size == 0 || (align & 0x1))
		align = 1;
	else if (align & 0x2)
		align = 2;
	else if (align & 0x4)
		align = 4;
	else
		align = 8;
	// check address location
	// Currently we only support relocation of addresses inside the
	// port buffer.
	int32 addressOffset = (uint8*)&address - (uint8*)fRequest;
	if (addressOffset < (int32)sizeof(Request)
		|| addressOffset + (int32)sizeof(Address) > fRequestSize) {
		RETURN_ERROR(B_BAD_VALUE);
	}
	// get the next free aligned offset in the port buffer
	int32 offset = (fRequestSize + align - 1) / align * align;
	// allocate the data
	if (fRequestOffset + offset + size <= fPort->GetCapacity()) {
		// there's enough free space in the port buffer
		fRequestSize = offset + size;
		fPort->Reserve(fRequestOffset + fRequestSize);
		if (deferredInit) {
			DeferredInitInfo& info
				= fDeferredInitInfos[fDeferredInitInfoCount];
			if (size > 0) {
				info.data = (uint8*)malloc(size);
				if (!info.data)
					RETURN_ERROR(B_NO_MEMORY);
			} else
				info.data = NULL;
			info.area = -1;
			info.offset = offset;
			info.size = size;
			info.inPortBuffer = true;
			info.target = &address;
			*data = info.data;
			fDeferredInitInfoCount++;
		} else {
			*data = (uint8*)fRequest + offset;
			address.SetTo(-1, offset, size);
		}
	} else {
		// not enough room in the port's buffer: we need to allocate an area
		if (fAllocatedAreaCount >= MAX_REQUEST_ADDRESS_COUNT)
			RETURN_ERROR(B_ERROR);
		int32 areaSize = (size + B_PAGE_SIZE - 1) / B_PAGE_SIZE * B_PAGE_SIZE;
		area_id area = create_area("request data", data,
#ifdef _KERNEL_MODE
			B_ANY_KERNEL_ADDRESS,
#else
			B_ANY_ADDRESS,
#endif
			areaSize, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		if (area < 0)
			RETURN_ERROR(area);
		fAllocatedAreas[fAllocatedAreaCount++] = area;
		if (deferredInit) {
			DeferredInitInfo& info
				= fDeferredInitInfos[fDeferredInitInfoCount];
			info.data = NULL;
			info.area = area;
			info.offset = 0;
			info.size = size;
			info.inPortBuffer = false;
			info.target = &address;
			fDeferredInitInfoCount++;
PRINT(("  RequestAllocator::AllocateAddress(): deferred allocated area: "
"%ld, size: %ld (%ld), data: %p\n", area, size, areaSize, *data));
		} else
			address.SetTo(area, 0, size);
	}
	return B_OK;
}

// AllocateData
status_t
RequestAllocator::AllocateData(Address& address, const void* data, int32 size,
	int32 align, bool deferredInit)
{
	status_t error = B_OK;
	if (data != NULL) {
		void* destination;
		error = AllocateAddress(address, size, align, &destination,
			deferredInit);
		if (error != B_OK)
			return error;
		if (size > 0)
			memcpy(destination, data, size);
	} else
		address.SetTo(-1, 0, 0);
	return error;
}

// AllocateString
status_t
RequestAllocator::AllocateString(Address& address, const char* data,
	bool deferredInit)
{
	int32 size = (data ? strlen(data) + 1 : 0);
	return AllocateData(address, data, size, 1, deferredInit);
}

// SetAddress
/*status_t
RequestAllocator::SetAddress(Address& address, void* data, int32 size)
{
	if (fError != B_OK)
		return fError;
	if (!fRequest)
		return (fError = B_NO_INIT);
	// check address location
	// Currently we only support relocation of addresses inside the
	// port buffer.
	int32 addressOffset = (uint8*)&address - (uint8*)fRequest;
	if (addressOffset < (int32)sizeof(Request)
		|| addressOffset + (int32)sizeof(Address) > fRequestSize) {
		return (fError = B_BAD_VALUE);
	}
	// if data does itself lie within the port buffer, we store only the
	// request relative offset
	int32 inRequestOffset = (uint8*)data - (uint8*)fRequest;
	if (!data) {
		address.SetTo(-1, 0, 0);
	} else if (inRequestOffset >= (int32)sizeof(Request)
		&& inRequestOffset <= fRequestSize) {
		if (inRequestOffset + size > fRequestSize)
			return (fError = B_BAD_VALUE);
		address.SetTo(-1, inRequestOffset, size);
	} else {
		// get the area and in-area offset for the address
		area_id area;
		int32 offset;
		fError = get_area_for_address(data, size, &area, &offset);
		if (fError != B_OK)
			return fError;
		// set the address
		address.SetTo(area, offset, size);
	}
	return fError;
}*/

