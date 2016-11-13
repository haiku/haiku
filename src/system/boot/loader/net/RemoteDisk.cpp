/*
 * Copyright 2005, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include <boot/net/RemoteDisk.h>

#include <new>

#include <endian.h>
#include <stdio.h>

#include <OS.h>
#include <SupportDefs.h>

#include <boot/net/UDP.h>


static const bigtime_t kRequestTimeout = 100000LL;


using std::nothrow;


#if __BYTE_ORDER == __LITTLE_ENDIAN

static inline
uint64_t swap_uint64(uint64_t data)
{
	return ((data & 0xff) << 56)
		| ((data & 0xff00) << 40)
		| ((data & 0xff0000) << 24)
		| ((data & 0xff000000) << 8)
		| ((data >> 8) & 0xff000000)
		| ((data >> 24) & 0xff0000)
		| ((data >> 40) & 0xff00)
		| ((data >> 56) & 0xff);
}

#define host_to_net64(data)	swap_uint64(data)
#define net_to_host64(data)	swap_uint64(data)

#endif

#if __BYTE_ORDER == __BIG_ENDIAN
#define host_to_net64(data)	(data)
#define net_to_host64(data)	(data)
#endif

#undef htonll
#undef ntohll
#define htonll(data)	host_to_net64(data)
#define ntohll(data)	net_to_host64(data)


// constructor
RemoteDisk::RemoteDisk()
	: fServerAddress(INADDR_ANY),
	  fServerPort(0),
	  fImageSize(0),
	  fRequestID(0),
	  fSocket(NULL),
	  fPacket(NULL)
{
}

// destructor
RemoteDisk::~RemoteDisk()
{
	delete fSocket;
	delete fPacket;
}

// Init
status_t
RemoteDisk::Init(ip_addr_t serverAddress, uint16 serverPort, off_t imageSize)
{
	fServerAddress = serverAddress;
	fServerPort = serverPort;
	fImageSize = imageSize;

	// create and bind socket
	fSocket = new(nothrow) UDPSocket;
	if (!fSocket)
		return B_NO_MEMORY;
	
	status_t error = fSocket->Bind(INADDR_ANY, 6666);
	if (error != B_OK)
		return error;

	return B_OK;
}

// ReadAt
ssize_t
RemoteDisk::ReadAt(void */*cookie*/, off_t pos, void *_buffer,
	size_t bufferSize)
{
	if (!fSocket)
		return B_NO_INIT;

	uint8 *buffer = (uint8*)_buffer;
	if (!buffer || pos < 0)
		return B_BAD_VALUE;

	if (bufferSize == 0)
		return 0;

	// Check whether the current packet already contains the beginning of the
	// data to read.
	ssize_t bytesRead = _ReadFromPacket(pos, buffer, bufferSize);

	// If there still remains something to be read, we need to get it from the
	// server.
	status_t error = B_OK;
	while (bufferSize > 0) {
		// prepare request
		remote_disk_header request;
		request.offset = htonll(pos);
		uint32 toRead = min_c(bufferSize, REMOTE_DISK_BLOCK_SIZE);
		request.size = htons(toRead);
		request.command = REMOTE_DISK_READ_REQUEST;

		// send request
		UDPPacket *packet;
		error = _SendRequest(&request, sizeof(request), REMOTE_DISK_READ_REPLY,
			&packet);
		if (error != B_OK)
			break;

		// check for errors
		int16 packetSize = ntohs(((remote_disk_header*)packet->Data())->size);
		if (packetSize < 0) {
			if (packetSize == REMOTE_DISK_IO_ERROR)
				error = B_IO_ERROR;
			else if (packetSize == REMOTE_DISK_BAD_REQUEST)
				error = B_BAD_VALUE;
			break;
		}

		// make the reply packet the current packet
		delete fPacket;
		fPacket = packet;

		// read from the packet
		size_t packetBytesRead = _ReadFromPacket(pos, buffer, bufferSize);
		if (packetBytesRead == 0)
			break;
		bytesRead += packetBytesRead;
	}

	// only return an error, when we were not able to read anything at all
	return (bytesRead == 0 ? error : bytesRead);
}

// WriteAt
ssize_t
RemoteDisk::WriteAt(void */*cookie*/, off_t pos, const void *buffer,
	size_t bufferSize)
{
	// Not needed in the boot loader.
	return B_PERMISSION_DENIED;
}

// GetName
status_t
RemoteDisk::GetName(char *nameBuffer, size_t bufferSize) const
{
	if (!nameBuffer)
		return B_BAD_VALUE;

	snprintf(nameBuffer, bufferSize,
		"RemoteDisk:%" B_PRIu32 ".%" B_PRIu32 ".%" B_PRIu32 ".%" B_PRIu32 ":%hd",
		(fServerAddress >> 24) & 0xff, (fServerAddress >> 16) & 0xff,
		(fServerAddress >> 8) & 0xff, fServerAddress & 0xff, fServerPort);

	return B_OK;
}

// Size
off_t
RemoteDisk::Size() const
{
	return fImageSize;
}

ip_addr_t
RemoteDisk::ServerIPAddress() const
{
	return fServerAddress;
}

uint16
RemoteDisk::ServerPort() const
{
	return fServerPort;
}

// FindAnyRemoteDisk
RemoteDisk *
RemoteDisk::FindAnyRemoteDisk()
{
	// create a socket and bind it
	UDPSocket socket;
	status_t error = socket.Bind(INADDR_ANY, 6665);
	if (error != B_OK) {
		printf("RemoteDisk::GetAnyRemoteDisk(): Failed to bind socket.\n");
		return NULL;
	}

	// prepare request
	remote_disk_header request;
	request.command = REMOTE_DISK_HELLO_REQUEST;

	// send request
	UDPPacket *packet;
	error = _SendRequest(&socket, INADDR_BROADCAST, REMOTE_DISK_SERVER_PORT,
		&request, sizeof(request), REMOTE_DISK_HELLO_REPLY, &packet);
	if (error != B_OK) {
		printf("RemoteDisk::GetAnyRemoteDisk(): Got no server reply.\n");
		return NULL;
	}
	remote_disk_header *reply = (remote_disk_header*)packet->Data();

	// create a RemoteDisk object
	RemoteDisk *remoteDisk = new(nothrow) RemoteDisk;
	if (remoteDisk) {
		error = remoteDisk->Init(packet->SourceAddress(), ntohs(reply->port),
			ntohll(reply->offset));
		if (error != B_OK) {
			delete remoteDisk;
			remoteDisk = NULL;
		}
	}

	delete packet;

	return remoteDisk;
}

// _ReadFromPacket
ssize_t
RemoteDisk::_ReadFromPacket(off_t &pos, uint8 *&buffer, size_t &bufferSize)
{
	if (!fPacket)
		return 0;

	remote_disk_header *header = (remote_disk_header*)fPacket->Data();
	uint64 packetOffset = ntohll(header->offset);
	uint32 packetSize = ntohs(header->size);
	if (packetOffset > (uint64)pos || packetOffset + packetSize <= (uint64)pos)
		return 0;
		
	// we do indeed have some bytes already
	size_t toCopy = size_t(packetOffset + packetSize - (uint64)pos);
	if (toCopy > bufferSize)
		toCopy = bufferSize;
	memcpy(buffer, header->data + (pos - packetOffset), toCopy);
	
	pos += toCopy;
	buffer += toCopy;
	bufferSize -= toCopy;
	return toCopy;
}

// _SendRequest
status_t
RemoteDisk::_SendRequest(UDPSocket *socket, ip_addr_t serverAddress,
	uint16 serverPort, remote_disk_header *request, size_t size,
	uint8 expectedReply, UDPPacket **_packet)
{
	request->port = htons(socket->Port());

	// try sending the request 3 times at most
	for (int i = 0; i < 3; i++) {
		// send request
		status_t error = socket->Send(serverAddress, serverPort, request, size);
		if (error != B_OK)
			return error;

		// receive reply
		bigtime_t timeout = system_time() + kRequestTimeout;
		do {
			UDPPacket *packet;
			error = socket->Receive(&packet, timeout - system_time());
			if (error == B_OK) {
				// got something; check, if it is looks good
				if (packet->DataSize() >= sizeof(remote_disk_header)) {
					remote_disk_header *reply
						= (remote_disk_header*)packet->Data();
					if (reply->request_id == request->request_id
						&& reply->command == expectedReply) {
						*_packet = packet;
						return B_OK;
					}
				}

				// reply not OK
				delete packet;
			} else if (error != B_TIMED_OUT && error != B_WOULD_BLOCK)
				return error;

		} while (timeout > system_time());
	}

	// no reply
	return B_ERROR;
}

// _SendRequest
status_t
RemoteDisk::_SendRequest(remote_disk_header *request, size_t size,
	uint8 expectedReply, UDPPacket **packet)
{
	request->request_id = fRequestID++;
	return _SendRequest(fSocket, fServerAddress, fServerPort, request, size,
		expectedReply, packet);
}
