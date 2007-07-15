/*
 * Copyright 2005-2007, Ingo Weinhold <bonefish@cs.tu-berlin.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "RemoteDisk.h"

#include <new>

#include <endian.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <KernelExport.h>
#include <OS.h>

#include <kernel.h>		// for IS_USER_ADDRESS()


//#define TRACE_REMOTE_DISK
#ifdef TRACE_REMOTE_DISK
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) do {} while (false)
#endif


static const bigtime_t kReceiveTimeout = 30000LL;
static const bigtime_t kRequestTimeout = 100000LL;

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


enum {
	BUFFER_SIZE	= 2048
};


// constructor
RemoteDisk::RemoteDisk()
	: fImageSize(0),
	  fRequestID(0),
	  fSocket(-1),
	  fPacket(NULL),
	  fPacketSize(0)
{
}


// destructor
RemoteDisk::~RemoteDisk()
{
	if (fSocket >= 0)
		close(fSocket);

	free(fPacket);
}


// Init
status_t
RemoteDisk::Init(uint32 serverAddress, uint16 serverPort, off_t imageSize)
{
	status_t error = _Init();
	if (error != B_OK)
		return error;

	fServerAddress.sin_family = AF_INET;
	fServerAddress.sin_port = htons(serverPort);
	fServerAddress.sin_addr.s_addr = htonl(serverAddress);

	fImageSize = imageSize;

	return B_OK;
}


// FindAnyRemoteDisk
status_t
RemoteDisk::FindAnyRemoteDisk()
{
	status_t error = _Init();
	if (error != B_OK)
		return error;

	// prepare request
	remote_disk_header request;
	request.command = REMOTE_DISK_HELLO_REQUEST;

	// init server address to broadcast
	fServerAddress.sin_family = AF_INET;
	fServerAddress.sin_port = htons(REMOTE_DISK_SERVER_PORT);
	fServerAddress.sin_addr.s_addr = htonl(INADDR_BROADCAST);

	// set SO_BROADCAST on socket
	int soBroadcastValue = 1;
	if (fSocketModule->setsockopt(fSocket, SOL_SOCKET, SO_BROADCAST,
		&soBroadcastValue, sizeof(soBroadcastValue)) < 0) {
		dprintf("RemoteDisk::Init(): Failed to set SO_BROADCAST on socket: "
			"%s\n", strerror(errno));
	}

	// send request
	sockaddr_in serverAddress;
	error = _SendRequest(&request, sizeof(request), REMOTE_DISK_HELLO_REPLY,
		&serverAddress);
	if (error != B_OK) {
		dprintf("RemoteDisk::FindAnyRemoteDisk(): Got no server reply.\n");
		return error;
	}
	remote_disk_header* reply = (remote_disk_header*)fPacket;

	// unset SO_BROADCAST on socket
	soBroadcastValue = 0;
	if (fSocketModule->setsockopt(fSocket, SOL_SOCKET, SO_BROADCAST,
		&soBroadcastValue, sizeof(soBroadcastValue)) < 0) {
		dprintf("RemoteDisk::Init(): Failed to unset SO_BROADCAST on socket: "
			"%s\n", strerror(errno));
	}

	// init server address and size
	fServerAddress = serverAddress;
	fServerAddress.sin_port = reply->port;

	fImageSize = ntohll(reply->offset);

	return B_OK;
}


// ReadAt
ssize_t
RemoteDisk::ReadAt(off_t pos, void *_buffer, size_t bufferSize)
{
	if (fSocket < 0)
		return B_NO_INIT;

	uint8 *buffer = (uint8*)_buffer;
	if (!buffer || pos < 0)
		return B_BAD_VALUE;

	if (bufferSize == 0)
		return 0;

	// Check whether the current packet already contains the beginning of the
	// data to read.
	ssize_t bytesRead = _ReadFromPacket(pos, buffer, bufferSize);
	if (bytesRead < 0)
		return bytesRead;

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
		error = _SendRequest(&request, sizeof(request), REMOTE_DISK_READ_REPLY);
		if (error != B_OK)
			break;

		// check for errors
		int16 packetSize = ntohs(((remote_disk_header*)fPacket)->size);
		if (packetSize < 0) {
			if (packetSize == REMOTE_DISK_IO_ERROR)
				error = B_IO_ERROR;
			else if (packetSize == REMOTE_DISK_BAD_REQUEST)
				error = B_BAD_VALUE;
			fPacketSize = 0;
			break;
		}

		// read from the packet
		size_t packetBytesRead = _ReadFromPacket(pos, buffer, bufferSize);
		if (packetBytesRead <= 0) {
			if (packetBytesRead < 0)
				error = packetBytesRead;
			break;
		}
		bytesRead += packetBytesRead;
	}

	// only return an error, when we were not able to read anything at all
	return (bytesRead == 0 ? error : bytesRead);
}


// WriteAt
ssize_t
RemoteDisk::WriteAt(off_t pos, const void *buffer, size_t bufferSize)
{
	// TODO: Implement!
	return B_PERMISSION_DENIED;
}


// _Init
status_t
RemoteDisk::_Init()
{
	// get the socket module
	status_t error = get_module(B_SOCKET_MODULE_NAME,
		(module_info**)&fSocketModule);
	if (error != B_OK) {
		dprintf("RemoteDisk::Init(): Failed to load socket module: %s\n",
			strerror(error));
		return error;
	}

	// open a control socket for playing with the stack
	fSocket = fSocketModule->socket(AF_INET, SOCK_DGRAM, 0);
	if (fSocket < 0) {
		dprintf("RemoteDisk::Init(): Failed to open socket: %s\n",
			strerror(errno));
		return errno;
	}

	// bind socket
	fSocketAddress.sin_family = AF_INET;
	fSocketAddress.sin_port = 0;
	fSocketAddress.sin_addr.s_addr = INADDR_ANY;
	if (fSocketModule->bind(fSocket, (sockaddr*)&fSocketAddress,
			sizeof(fSocketAddress)) < 0) {
		dprintf("RemoteDisk::Init(): Failed to bind socket: %s\n",
			strerror(errno));
		return errno;
	}

   // get the port
    socklen_t addrSize = sizeof(fSocketAddress);
    if (fSocketModule->getsockname(fSocket, (sockaddr*)&fSocketAddress,
			&addrSize) < 0) {
		dprintf("RemoteDisk::Init(): Failed to get socket address: %s\n",
			strerror(errno));
        return errno;
    }

	// set receive timeout 
	timeval timeout;
	timeout.tv_sec = time_t(kReceiveTimeout / 1000000LL);
	timeout.tv_usec = suseconds_t(kReceiveTimeout % 1000000LL);
	if (fSocketModule->setsockopt(fSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout,
			sizeof(timeout)) < 0) {
		dprintf("RemoteDisk::Init(): Failed to set socket receive timeout: "
			"%s\n", strerror(errno));
        return errno;
	}

	// allocate buffer
	fPacket = malloc(BUFFER_SIZE);
	if (!fPacket)
		return B_NO_MEMORY;

	return B_OK;
}


// _ReadFromPacket
ssize_t
RemoteDisk::_ReadFromPacket(off_t& pos, uint8*& buffer, size_t& bufferSize)
{
	if (fPacketSize == 0)
		return 0;

	// check whether the cached packet is indeed a read reply
	remote_disk_header* header = (remote_disk_header*)fPacket;
	if (header->command != REMOTE_DISK_READ_REPLY)
		return 0;

	uint64 packetOffset = ntohll(header->offset);
	uint32 packetSize = ntohs(header->size);
	if (packetOffset > (uint64)pos || packetOffset + packetSize <= (uint64)pos)
		return 0;
		
	// we have something to copy
	size_t toCopy = size_t(packetOffset + packetSize - (uint64)pos);
	if (toCopy > bufferSize)
		toCopy = bufferSize;

	if (IS_USER_ADDRESS(buffer)) {
		status_t error = user_memcpy(buffer,
			header->data + (pos - packetOffset), toCopy);
		if (error != B_OK)
			return error;
	} else
		memcpy(buffer, header->data + (pos - packetOffset), toCopy);

	pos += toCopy;
	buffer += toCopy;
	bufferSize -= toCopy;
	return toCopy;
}


// _SendRequest
status_t
RemoteDisk::_SendRequest(remote_disk_header* request, size_t size,
	uint8 expectedReply, sockaddr_in* peerAddress)
{
	request->request_id = fRequestID++;
	request->port = fSocketAddress.sin_port;

	// try sending the request 3 times at most
	for (int i = 0; i < 3; i++) {
		// send request
//bool debug = false;
//{
//thread_info threadInfo;
//get_thread_info(find_thread(NULL), &threadInfo);
////debug = (strcmp(threadInfo.name, "AddOnMonitor") == 0);
//debug = (find_thread(NULL) >= 60);
//if (debug) {
////panic("RemoteDisk::_SendRequest(): AddOnMonitor\n");
//TRACE(("[%ld: %s] RemoteDisk::_SendRequest(): sendto(%d, %p, %lu, 0, %p, %lu)\n",
//find_thread(NULL), threadInfo.name, fSocket, request, size, (sockaddr*)&fServerAddress, sizeof(fServerAddress)));
//}
//}
		ssize_t bytesSent = fSocketModule->sendto(fSocket, request, size,
        	0, (sockaddr*)&fServerAddress, sizeof(fServerAddress));
		if (bytesSent < 0)
			return errno;
		if (bytesSent != (ssize_t)size) {
			dprintf("RemoteDisk::_SendRequest(): sent less bytes than desired");
			return B_ERROR;
		}

		// receive reply
		bigtime_t timeout = system_time() + kRequestTimeout;
		do {
			fPacketSize = 0;
			socklen_t addrSize = sizeof(sockaddr_in);
//if (debug) {
//TRACE(("[%ld] RemoteDisk::_SendRequest(): recvfrom(%d, %p, %u, 0, %p, %p)\n",
//find_thread(NULL), fSocket, fPacket, BUFFER_SIZE, (sockaddr*)&peerAddress, (peerAddress ? &addrSize : 0)));
//}
			ssize_t bytesReceived = fSocketModule->recvfrom(fSocket,
				fPacket, BUFFER_SIZE, 0, (sockaddr*)peerAddress,
				(peerAddress ? &addrSize : 0));
			if (bytesReceived < 0) {
//if (debug) {
//TRACE(("[%ld] RemoteDisk::_SendRequest(): recvfrom() failed: %s\n", find_thread(NULL), strerror(errno)));
//}
				status_t error = errno;
				if (error != B_TIMED_OUT && error != B_WOULD_BLOCK)
					return error;
				continue;
			}

			// got something; check, if it is looks good
			if (bytesReceived >= (ssize_t)sizeof(remote_disk_header)) {
				remote_disk_header* reply = (remote_disk_header*)fPacket;
				if (reply->request_id == request->request_id
					&& reply->command == expectedReply) {
					fPacketSize = bytesReceived;
//if (debug) {
//TRACE(("[%ld] RemoteDisk::_SendRequest(): done: packet size: %ld\n", find_thread(NULL), bytesReceived));
//}
					return B_OK;
				}
			}
		} while (timeout > system_time());
	}

	// no reply
	return B_ERROR;
}
