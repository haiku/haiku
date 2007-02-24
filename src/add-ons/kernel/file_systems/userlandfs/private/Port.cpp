// Port.cpp

#include <new>

#include "AreaSupport.h"
#include "Compatibility.h"
#include "Port.h"

// minimal and maximal port size
static const int32 kMinPortSize = 1024;			// 1 kB
static const int32 kMaxPortSize = 64 * 1024;	// 64 kB

// constructor
Port::Port(int32 size)
	: fBuffer(NULL),
	  fCapacity(0),
	  fMessageSize(0),
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
	: fBuffer(NULL),
	  fCapacity(0),
	  fMessageSize(0),
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

// GetBuffer
void*
Port::GetBuffer() const
{
	return fBuffer;
}

// GetCapacity
int32
Port::GetCapacity() const
{
	return fCapacity;
}

// GetMessage
void*
Port::GetMessage() const
{
	return (fInitStatus == B_OK && fMessageSize > 0 ? fBuffer : NULL);
}

// GetMessageSize
int32
Port::GetMessageSize() const
{
	return (fInitStatus == B_OK ? fMessageSize : 0);
}

// Send
status_t
Port::Send(int32 size)
{
	if (fInitStatus != B_OK)
		return fInitStatus;
	if (size <= 0 || size > fCapacity)
		return B_BAD_VALUE;
	fMessageSize = 0;
	port_id port = (fOwner ? fInfo.client_port : fInfo.owner_port);
	status_t error;
	do {
		error = write_port(port, 0, fBuffer, size);
	} while (error == B_INTERRUPTED);
	return (fInitStatus = error);
}

// SendAndReceive
status_t
Port::SendAndReceive(int32 size)
{
	status_t error = Send(size);
	if (error != B_OK)
		return error;
	return Receive();
}

// Receive
status_t
Port::Receive(bigtime_t timeout)
{
	if (fInitStatus != B_OK)
		return fInitStatus;
	port_id port = (fOwner ? fInfo.owner_port : fInfo.client_port);
	status_t error = B_OK;
	do {
		int32 code;
		ssize_t bytesRead;
		if (timeout >= 0) {
			bytesRead = read_port_etc(port, &code, fBuffer, fCapacity,
				B_RELATIVE_TIMEOUT, timeout);
		} else
			bytesRead = read_port(port, &code, fBuffer, fCapacity);
		if (bytesRead < 0)
			error = bytesRead;
		else
			fMessageSize = bytesRead;
	} while (error == B_INTERRUPTED);
	if (error == B_TIMED_OUT || error == B_WOULD_BLOCK) {
		return error;
	}
	if (error != B_OK)
		return (fInitStatus = error);
	if (fMessageSize <= 0 || fMessageSize > fCapacity) {
		fMessageSize = 0;
		return B_BAD_DATA;
	}
	return B_OK;
}

