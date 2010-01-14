// PortChannel.cpp

#include <new>
#include <string.h>

#include "PortChannel.h"
#include "Utils.h"

static const int32 kMaxPortMessageSize = 102400;	// 100 KB
static const int32 kDefaultBufferSize	= 10240;	// 10 KB

// constructor
PortChannel::PortChannel()
	: Channel(),
	  fSendPort(-1),
	  fReceivePort(-1),
	  fBuffer(new(std::nothrow) uint8[kDefaultBufferSize]),
	  fBufferSize(kDefaultBufferSize),
	  fBufferOffset(0),
	  fBufferContentSize(0)
{
	fSendPort = create_port(10, "port connection send port");
	fReceivePort = create_port(10, "port connection receive port");
}

// constructor
PortChannel::PortChannel(const Info* info, bool inverse)
	: Channel(),
	  fSendPort(inverse ? info->receivePort : info->sendPort),
	  fReceivePort(inverse ? info->sendPort : info->receivePort),
	  fBuffer(new(std::nothrow) uint8[kDefaultBufferSize]),
	  fBufferSize(kDefaultBufferSize),
	  fBufferOffset(0),
	  fBufferContentSize(0)
{
}

// constructor
PortChannel::PortChannel(port_id sendPort, port_id receivePort)
	: Channel(),
	  fSendPort(sendPort),
	  fReceivePort(receivePort),
	  fBuffer(new(std::nothrow) uint8[kDefaultBufferSize]),
	  fBufferSize(kDefaultBufferSize),
	  fBufferOffset(0),
	  fBufferContentSize(0)
{
}

// destructor
PortChannel::~PortChannel()
{
	delete[] fBuffer;
}

// InitCheck
status_t
PortChannel::InitCheck() const
{
	if (fSendPort < 0)
		return fSendPort;
	if (fReceivePort < 0)
		return fReceivePort;
	if (!fBuffer)
		return B_NO_MEMORY;
	return B_OK;
}

// GetInfo
void
PortChannel::GetInfo(Info* info) const
{
	if (info) {
		info->sendPort = fSendPort;
		info->receivePort = fReceivePort;
	}
}

// Close
void
PortChannel::Close()
{
	if (fSendPort >= 0) {
		delete_port(fSendPort);
		fSendPort = -1;
	}
	if (fReceivePort >= 0) {
		delete_port(fReceivePort);
		fReceivePort = -1;
	}
}

// Send
status_t
PortChannel::Send(const void* _buffer, int32 size)
{
	if (size == 0)
		return B_OK;
	if (!_buffer || size < 0)
		return B_BAD_VALUE;
	const uint8* buffer = static_cast<const uint8*>(_buffer);
	while (size > 0) {
		int32 sendSize = min(size, fBufferSize);
		status_t error = write_port(fSendPort, 0, buffer, size);
		if (error != B_OK)
			return error;
		size -= sendSize;
		buffer += sendSize;
	}
	return B_OK;
}

// Receive
status_t
PortChannel::Receive(void* _buffer, int32 size)
{
	if (size == 0)
		return B_OK;
	if (!_buffer || size < 0)
		return B_BAD_VALUE;
	uint8* buffer = static_cast<uint8*>(_buffer);
	while (size > 0) {
		if (fBufferContentSize > 0) {
			// there's something in our buffer: just read from there
			int32 bytesRead = min(size, fBufferContentSize);
			memcpy(buffer, fBuffer + fBufferOffset, bytesRead);
			fBufferOffset += bytesRead;
			fBufferContentSize -= bytesRead;
			size -= bytesRead;
			buffer += bytesRead;
		} else {
			// nothing left in the buffer: we read from the port
			if (size > fBufferSize) {
				// we shall read more than what would fit into our buffer, so
				// we can directly read into the user buffer
				ssize_t bytesRead = read_port(fReceivePort, 0, buffer, size);
				if (bytesRead < 0)
					return bytesRead;
				size -= bytesRead;
				buffer += bytesRead;
			} else {
				// we read into our buffer
				ssize_t bytesRead = read_port(fReceivePort, 0, fBuffer,
					fBufferSize);
				if (bytesRead < 0)
					return bytesRead;
				fBufferOffset = 0;
				fBufferContentSize = bytesRead;
			}
		}
	}
	return B_OK;
}

