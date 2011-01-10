/*
 * Copyright 2009-2011, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <io_requests.h>

#include "IORequest.h"


// #pragma mark - static helpers


static status_t
transfer_io_request_data(io_request* request, void* buffer, size_t size,
	bool writeToRequest)
{
	if (writeToRequest == request->IsWrite()
		|| request->RemainingBytes() < size) {
		return B_BAD_VALUE;
	}

	// lock the request buffer memory, if it is user memory
	IOBuffer* ioBuffer = request->Buffer();
	if (ioBuffer->IsUser() && !ioBuffer->IsMemoryLocked()) {
		status_t error = ioBuffer->LockMemory(request->TeamID(),
			!writeToRequest);
		if (error != B_OK)
			return error;
	}

	// read/write
	off_t offset = request->Offset() + request->TransferredBytes();
	status_t error = writeToRequest
		? request->CopyData(buffer, offset, size)
		: request->CopyData(offset, buffer, size);
	if (error != B_OK)
		return error;

	request->Advance(size);
	return B_OK;
}


// #pragma mark - public API


/*!	Returns whether the given I/O request is a write request. */
bool
io_request_is_write(const io_request* request)
{
	return request->IsWrite();
}


/*!	Returns whether the I/O request has VIP status. */
bool
io_request_is_vip(const io_request* request)
{
	return (request->Flags() & B_VIP_IO_REQUEST) != 0;
}


/*!	Returns the read/write offset of the given I/O request.
	This is the immutable offset the request was created with;
	read_from_io_request() and write_to_io_request() don't change it.
*/
off_t
io_request_offset(const io_request* request)
{
	return request->Offset();
}


/*!	Returns the read/write length of the given I/O request.
	This is the immutable length the request was created with;
	read_from_io_request() and write_to_io_request() don't change it.
*/
off_t
io_request_length(const io_request* request)
{
	return request->Length();
}


/*!	Reads data from the given I/O request into the given buffer and advances
	the request's transferred data pointer.
	Multiple calls to read_from_io_request() are allowed, but the total size
	must not exceed io_request_length(request).
*/
status_t
read_from_io_request(io_request* request, void* buffer, size_t size)
{
	return transfer_io_request_data(request, buffer, size, false);
}


/*!	Writes data from the given buffer to the given I/O request and advances
	the request's transferred data pointer.
	Multiple calls to write_to_io_request() are allowed, but the total size
	must not exceed io_request_length(request).
*/
status_t
write_to_io_request(io_request* request, const void* buffer, size_t size)
{
	return transfer_io_request_data(request, (void*)buffer, size, true);
}


/*!	Sets the given I/O request's status and notifies listeners that the request
	is finished.
*/
void
notify_io_request(io_request* request, status_t status)
{
	request->SetStatusAndNotify(status);
}
