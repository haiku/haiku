/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */


#include "Connection.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <util/kernel_cpp.h>
#include <net/dns_resolver.h>


#define NFS4_PORT		2049

#define	LAST_FRAGMENT	0x80000000
#define MAX_PACKET_SIZE	65535


bool
ServerAddress::operator==(const ServerAddress& address)
{
	return memcmp(&fAddress, &address.fAddress, sizeof(fAddress)) == 0
		&& fProtocol == address.fProtocol;
}

bool
ServerAddress::operator<(const ServerAddress& address)
{
	int compare = memcmp(&fAddress, &address.fAddress, sizeof(fAddress));
	return compare < 0 || (compare == 0 && fProtocol < address.fProtocol);
}


ServerAddress&
ServerAddress::operator=(const ServerAddress& address)
{
	fAddress = address.fAddress;
	fProtocol = address.fProtocol;
	return *this;
}


ServerAddress::ServerAddress()
	:
	fProtocol(0)
{
	memset(&fAddress, 0, sizeof(fAddress));
}


status_t
ServerAddress::ResolveName(const char* name, ServerAddress* address)
{
	address->fProtocol = IPPROTO_UDP;

	// getaddrinfo() is very expensive when called from kernel, so we do not
	// want to call it unless there is no other choice.

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	if (inet_aton(name, &addr.sin_addr) == 1) {
		addr.sin_len = sizeof(addr);
		addr.sin_family = AF_INET;
		addr.sin_port = htons(NFS4_PORT);

		memcpy(&address->fAddress, &addr, sizeof(addr));
		return B_OK;
	}

	addrinfo* ai;
	status_t result = getaddrinfo(name, NULL, NULL, &ai);
	if (result != B_OK)
		return result;

	addrinfo* current = ai;
	while (current != NULL) {
		if (current->ai_family == AF_INET) {
			memcpy(&address->fAddress, current->ai_addr, sizeof(sockaddr_in));
			reinterpret_cast<sockaddr_in*>(&address->fAddress)->sin_port
				= htons(NFS4_PORT);
		} else if (current->ai_family == AF_INET6) {
			memcpy(&address->fAddress, current->ai_addr, sizeof(sockaddr_in6));
			reinterpret_cast<sockaddr_in6*>(&address->fAddress)->sin6_port
				= htons(NFS4_PORT);
		} else {
			current = current->ai_next;
			continue;
		}

		freeaddrinfo(ai);
		return B_OK;
	}

	freeaddrinfo(ai);
	return B_NAME_NOT_FOUND;
}


Connection::Connection(const ServerAddress& address)
	:
	fWaitCancel(create_sem(0, NULL)),
	fSocket(-1),
	fServerAddress(address)
{
	mutex_init(&fSocketLock, NULL);
}


ConnectionStream::ConnectionStream(const ServerAddress& address)
	:
	Connection(address)
{
}


ConnectionPacket::ConnectionPacket(const ServerAddress& address)
	:
	Connection(address)
{
}


Connection::~Connection()
{
	if (fSocket != -1)
		close(fSocket);
	mutex_destroy(&fSocketLock);
	delete_sem(fWaitCancel);
}


status_t
Connection::GetLocalAddress(ServerAddress* address)
{
	address->fProtocol = fServerAddress.fProtocol;

	socklen_t addressSize;
	switch (reinterpret_cast<const sockaddr*>(&fServerAddress)->sa_family) {
		case AF_INET:
			addressSize = sizeof(sockaddr_in);
			break;
		case AF_INET6:
			addressSize = sizeof(sockaddr_in6);
			break;
		default:
			return B_BAD_VALUE;
	}

	return getsockname(fSocket,
		(struct sockaddr*)&address->fAddress, &addressSize);
}


status_t
ConnectionStream::Send(const void* buffer, uint32 size)
{
	status_t result;

	uint32* buf = (uint32*)malloc(size + sizeof(uint32));
	if (buf == NULL)
		return B_NO_MEMORY;

	buf[0] = htonl(size | LAST_FRAGMENT);
	memcpy(buf + 1, buffer, size);

	// More than one threads may send data and ksend is allowed to send partial
	// data. Need a lock here.
	uint32 sent = 0;
	mutex_lock(&fSocketLock);
	do {
		result = send(fSocket, buf + sent, size + sizeof(uint32) - sent, 0);
		sent += result;
	} while (result > 0 && sent < size + sizeof(uint32));
	mutex_unlock(&fSocketLock);
	if (result < 0) {
		result = errno;
		free(buf);
		return result;
	} else if (result == 0) {
		free(buf);
		return B_IO_ERROR;
	}

	free(buf);
	return B_OK;
}


status_t
ConnectionPacket::Send(const void* buffer, uint32 size)
{
	// send on DGRAM sockets is atomic. No need to lock.
	status_t result = send(fSocket, buffer,  size, 0);
	if (result < 0)
		return errno;
	return B_OK;
}


status_t
ConnectionStream::Receive(void** _buffer, uint32* _size)
{
	status_t result;

	int32 size = 0;
	void* buffer = NULL;

	int32 record_size;
	bool last_one;

	object_wait_info object[2];
	object[0].object = fWaitCancel;
	object[0].type = B_OBJECT_TYPE_SEMAPHORE;
	object[0].events = B_EVENT_ACQUIRE_SEMAPHORE;

	object[1].object = fSocket;
	object[1].type = B_OBJECT_TYPE_FD;
	object[1].events = B_EVENT_READ;

	do {
		result = wait_for_objects(object, 2);
		if (result < B_OK ||
			(object[0].events & B_EVENT_ACQUIRE_SEMAPHORE) != 0) {
			free(buffer);
			return ECONNABORTED;
		} else if ((object[1].events & B_EVENT_READ) == 0)
			continue;

		// There is only one listener thread per connection. No need to lock.
		uint32 received = 0;
		do {
			result = recv(fSocket, &record_size + received,
							sizeof(record_size) - received, 0);
			received += result;
		} while (result > 0 && received < sizeof(record_size));
		if (result < 0) {
			result = errno;
			free(buffer);
			return result;
		} else if (result == 0) {
			free(buffer);
			return ECONNABORTED;
		}

		record_size = ntohl(record_size);
		last_one = record_size < 0;
		record_size &= LAST_FRAGMENT - 1;

		void* ptr = realloc(buffer, size + record_size);
		if (ptr == NULL) {
			free(buffer);
			return B_NO_MEMORY;
		} else
			buffer = ptr;

		received = 0;
		do {
			result = recv(fSocket, (uint8*)buffer + size + received,
							record_size - received, 0);
			received += result;
		} while (result > 0 && received < sizeof(record_size));
		if (result < 0) {
			result = errno;
			free(buffer);
			return result;
		}

		size += record_size;
	} while (!last_one);


	*_buffer = buffer;
	*_size = size;

	return B_OK;
}


status_t
ConnectionPacket::Receive(void** _buffer, uint32* _size)
{
	status_t result;
	int32 size = MAX_PACKET_SIZE;
	void* buffer = malloc(size);

	if (buffer == NULL)
		return B_NO_MEMORY;

	object_wait_info object[2];
	object[0].object = fWaitCancel;
	object[0].type = B_OBJECT_TYPE_SEMAPHORE;
	object[0].events = B_EVENT_ACQUIRE_SEMAPHORE;

	object[1].object = fSocket;
	object[1].type = B_OBJECT_TYPE_FD;
	object[1].events = B_EVENT_READ;

	do {
		result = wait_for_objects(object, 2);
		if (result < B_OK ||
			(object[0].events & B_EVENT_ACQUIRE_SEMAPHORE) != 0) {
			free(buffer);
			return ECONNABORTED;
		} else if ((object[1].events & B_EVENT_READ) == 0)
			continue;
		break;
	} while (true);

	// There is only one listener thread per connection. No need to lock.
	size = recv(fSocket, buffer, size, 0);
	if (size < 0) {
		result = errno;
		free(buffer);
		return result;
	} else if (size == 0) {
		free(buffer);
		return ECONNABORTED;
	}

	*_buffer = buffer;
	*_size = size;

	return B_OK;
}


status_t
Connection::Connect(Connection **_connection, const ServerAddress& address)
{
	Connection* conn;
	switch (address.fProtocol) {
		case IPPROTO_TCP:
			conn = new(std::nothrow) ConnectionStream(address);
			break;
		case IPPROTO_UDP:
			conn = new(std::nothrow) ConnectionPacket(address);
			break;
		default:
			return B_BAD_VALUE;
	}
	if (conn == NULL)
		return B_NO_MEMORY;

	status_t result = conn->Connect();
	if (result != B_OK) {
		delete conn;
		return result;
	}

	*_connection = conn;

	return B_OK;
}


status_t
Connection::Connect()
{
	const sockaddr& address =
		*reinterpret_cast<const sockaddr*>(&fServerAddress);

	switch (fServerAddress.fProtocol) {
		case IPPROTO_TCP:
			fSocket = socket(address.sa_family, SOCK_STREAM, IPPROTO_TCP);
			break;
		case IPPROTO_UDP:
			fSocket = socket(address.sa_family, SOCK_DGRAM, IPPROTO_UDP);
			break;
		default:
			return B_BAD_VALUE;
	}
	if (fSocket < 0)
		return errno;

	socklen_t addressSize;
	switch (address.sa_family) {
		case AF_INET:
			addressSize = sizeof(sockaddr_in);
			break;
		case AF_INET6:
			addressSize = sizeof(sockaddr_in6);
			break;
		default:
			return B_BAD_VALUE;
	}

	status_t result = connect(fSocket, &address, addressSize);
	if (result != 0) {
		result = errno;
		close(fSocket);
		return result;
	}

	return B_OK;
}


status_t
Connection::Reconnect()
{
	release_sem(fWaitCancel);
	close(fSocket);
	acquire_sem(fWaitCancel);
	return Connect();
}


void
Connection::Disconnect()
{
	release_sem(fWaitCancel);

	close(fSocket);
	fSocket = -1;
}

