/*
 * Copyright 2001-2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <new>

#include <AutoDeleter.h>

#include "AreaSupport.h"
#include "Compatibility.h"
#include "Port.h"


using std::nothrow;

// minimal and maximal port size
static const int32 kMinPortSize = 1024;			// 1 kB
static const int32 kMaxPortSize = 64 * 1024;	// 64 kB


// constructor
Port::Port(int32 size)
	:
	fBuffer(NULL),
	fCapacity(0),
	fReservedSize(0),
	fInitStatus(B_NO_INIT),
	fOwner(true)
{
	// adjust size to be within the sane bounds
	if (size < kMinPortSize)
		size = kMinPortSize;
	else if (size > kMaxPortSize)
		size = kMaxPortSize;
	// allocate the buffer
	fBuffer = new(nothrow) uint8[size];
	if (!fBuffer) {
		fInitStatus = B_NO_MEMORY;
		return;
	}
	// create the owner port
	fInfo.owner_port = create_port(1, "port owner port");
	if (fInfo.owner_port < 0) {
		fInitStatus = fInfo.owner_port;
		return;
	}
	// create the client port
	fInfo.client_port = create_port(1, "port client port");
	if (fInfo.client_port < 0) {
		fInitStatus = fInfo.client_port;
		return;
	}
	fInfo.size = size;
	fCapacity = size;
	fInitStatus = B_OK;
}


// constructor
Port::Port(const Info* info)
	:
	fBuffer(NULL),
	fCapacity(0),
	fReservedSize(0),
	fInitStatus(B_NO_INIT),
	fOwner(false)
{
	// check parameters
	if (!info || info->owner_port < 0 || info->client_port < 0
		|| info->size < kMinPortSize || info->size > kMaxPortSize) {
		return;
	}
	// allocate the buffer
	fBuffer = new(nothrow) uint8[info->size];
	if (!fBuffer) {
		fInitStatus = B_NO_MEMORY;
		return;
	}
	// init the info
	fInfo.owner_port = info->owner_port;
	fInfo.client_port = info->client_port;
	fInfo.size = info->size;
	// init the other members
	fCapacity = info->size;
	fInitStatus = B_OK;
}


// destructor
Port::~Port()
{
	Close();
	delete[] fBuffer;
}


// Close
void
Port::Close()
{
	if (fInitStatus != B_OK)
		return;
	fInitStatus = B_NO_INIT;
	// delete the ports only if we are the owner
	if (fOwner) {
		if (fInfo.owner_port >= 0)
			delete_port(fInfo.owner_port);
		if (fInfo.client_port >= 0)
			delete_port(fInfo.client_port);
	}
	fInfo.owner_port = -1;
	fInfo.client_port = -1;
}


// InitCheck
status_t
Port::InitCheck() const
{
	return fInitStatus;
}


// GetInfo
const Port::Info*
Port::GetInfo() const
{
	return &fInfo;
}


// Reserve
void
Port::Reserve(int32 endOffset)
{
	if (endOffset > fReservedSize)
		fReservedSize = endOffset;
}


// Unreserve
void
Port::Unreserve(int32 endOffset)
{
	if (endOffset < fReservedSize)
		fReservedSize = endOffset;
}


// Send
status_t
Port::Send(const void* message, int32 size)
{
	if (fInitStatus != B_OK)
		return fInitStatus;
	if (size <= 0)
		return B_BAD_VALUE;

	port_id port = (fOwner ? fInfo.client_port : fInfo.owner_port);
	status_t error;
	do {
		error = write_port(port, 0, message, size);
	} while (error == B_INTERRUPTED);

	return (fInitStatus = error);
}


// Receive
status_t
Port::Receive(void** _message, size_t* _size, bigtime_t timeout)
{
	if (fInitStatus != B_OK)
		return fInitStatus;

	// convert to timeout to flags + timeout we can use in the loop
	uint32 timeoutFlags = 0;
	if (timeout < 0) {
		timeout = 0;
	} else if (timeout == 0) {
		timeoutFlags = B_RELATIVE_TIMEOUT;
	} else if (timeout >= 0) {
		timeout += system_time();
		timeoutFlags = B_ABSOLUTE_TIMEOUT;
	}

	port_id port = (fOwner ? fInfo.owner_port : fInfo.client_port);

	// wait for the next message
	status_t error = B_OK;
	ssize_t bufferSize;
	do {
		// TODO: When compiling for userland, we might want to save this syscall
		// by using read_port_etc() directly, using a sufficiently large
		// on-stack buffer and copying onto the heap.
		bufferSize = port_buffer_size_etc(port, timeoutFlags, timeout);
		if (bufferSize < 0)
			error = bufferSize;
	} while (error == B_INTERRUPTED);

	if (error == B_TIMED_OUT || error == B_WOULD_BLOCK)
		return error;
	if (error != B_OK)
		return (fInitStatus = error);

	// allocate memory for the message
	void* message = malloc(bufferSize);
	if (message == NULL)
		return (fInitStatus = B_NO_MEMORY);
	MemoryDeleter messageDeleter(message);

	// read the message
	int32 code;
	ssize_t bytesRead = read_port_etc(port, &code, message, bufferSize,
		B_RELATIVE_TIMEOUT, 0);
	if (bytesRead < 0)
		return fInitStatus = bytesRead;
	if (bytesRead != bufferSize)
		return fInitStatus = B_BAD_DATA;

	messageDeleter.Detach();
	*_message = message;
	*_size = bytesRead;

	return B_OK;
}
