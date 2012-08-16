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

#define NFS_MIN_PORT	665


bool
PeerAddress::operator==(const PeerAddress& address)
{
	return memcmp(&fAddress, &address.fAddress, sizeof(fAddress)) == 0
		&& fProtocol == address.fProtocol;
}

bool
PeerAddress::operator<(const PeerAddress& address)
{
	int compare = memcmp(&fAddress, &address.fAddress, sizeof(fAddress));
	return compare < 0 || (compare == 0 && fProtocol < address.fProtocol);
}


PeerAddress&
PeerAddress::operator=(const PeerAddress& address)
{
	fAddress = address.fAddress;
	fProtocol = address.fProtocol;
	return *this;
}


PeerAddress::PeerAddress()
	:
	fProtocol(0)
{
	memset(&fAddress, 0, sizeof(fAddress));
}


const char*
PeerAddress::ProtocolString() const
{
	static const char* tcpName = "tcp";
	static const char* udpName = "udp";
	static const char* unknown = "";

	switch (fProtocol) {
		case IPPROTO_TCP:
			return tcpName;
		case IPPROTO_UDP:
			return udpName;
		default:
			return unknown;
	}
}


char*
PeerAddress::UniversalAddress() const
{
	const sockaddr* address = reinterpret_cast<const sockaddr*>(&fAddress);

	char* uAddr = reinterpret_cast<char*>(malloc(INET6_ADDRSTRLEN + 16));
	if (uAddr == NULL)
		return NULL;

	if (inet_ntop(address->sa_family, InAddr(), uAddr, AddressSize()) == NULL)
		return NULL;

	char port[16];
	sprintf(port, ".%d.%d", Port() >> 8, Port() & 0xff);
	strcat(uAddr, port);

	return uAddr;
}


socklen_t
PeerAddress::AddressSize() const
{
	switch (reinterpret_cast<const sockaddr*>(&fAddress)->sa_family) {
		case AF_INET:
			return sizeof(sockaddr_in);
		case AF_INET6:
			return sizeof(sockaddr_in6);
		default:
			return 0;
	}
}


uint16
PeerAddress::Port() const
{
	uint16 port;

	switch (reinterpret_cast<const sockaddr*>(&fAddress)->sa_family) {
		case AF_INET:
			port = reinterpret_cast<const sockaddr_in*>(&fAddress)->sin_port;
			break;
		case AF_INET6:
			port = reinterpret_cast<const sockaddr_in6*>(&fAddress)->sin6_port;
			break;
		default:
			port = 0;
	}

	return ntohs(port);
}


void
PeerAddress::SetPort(uint16 port)
{
	port = htons(port);

	switch (reinterpret_cast<sockaddr*>(&fAddress)->sa_family) {
		case AF_INET:
			reinterpret_cast<sockaddr_in*>(&fAddress)->sin_port = port;
			break;
		case AF_INET6:
			reinterpret_cast<sockaddr_in6*>(&fAddress)->sin6_port = port;
			break;
	}
}


const void*
PeerAddress::InAddr() const
{
	switch (reinterpret_cast<const sockaddr*>(&fAddress)->sa_family) {
		case AF_INET:
			return &reinterpret_cast<const sockaddr_in*>(&fAddress)->sin_addr;
		case AF_INET6:
			return &reinterpret_cast<const sockaddr_in6*>(&fAddress)->sin6_addr;
		default:
			return NULL;
	}
}


size_t
PeerAddress::InAddrSize() const
{
	switch (reinterpret_cast<const sockaddr*>(&fAddress)->sa_family) {
		case AF_INET:
			return sizeof(in_addr);
		case AF_INET6:
			return sizeof(in6_addr);
		default:
			return 0;
	}
}


status_t
PeerAddress::ResolveName(const char* name, PeerAddress* address)
{
	address->fProtocol = IPPROTO_TCP;

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


Connection::Connection(const PeerAddress& address)
	:
	ConnectionBase(address)
{
}


ConnectionListener::ConnectionListener(const PeerAddress& address)
	:
	ConnectionBase(address)
{
}


ConnectionBase::ConnectionBase(const PeerAddress& address)
	:
	fWaitCancel(create_sem(0, NULL)),
	fSocket(-1),
	fPeerAddress(address)
{
	mutex_init(&fSocketLock, NULL);
}



ConnectionStream::ConnectionStream(const PeerAddress& address)
	:
	Connection(address)
{
}


ConnectionPacket::ConnectionPacket(const PeerAddress& address)
	:
	Connection(address)
{
}


ConnectionBase::~ConnectionBase()
{
	if (fSocket != -1)
		close(fSocket);
	mutex_destroy(&fSocketLock);
	delete_sem(fWaitCancel);
}


status_t
ConnectionBase::GetLocalAddress(PeerAddress* address)
{
	address->fProtocol = fPeerAddress.fProtocol;

	socklen_t addressSize = sizeof(address->fAddress);
	return getsockname(fSocket,	(struct sockaddr*)&address->fAddress,
		&addressSize);
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

	uint32 size = 0;
	void* buffer = NULL;

	uint32 record_size;
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
		last_one = static_cast<int32>(record_size) < 0;
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
		} while (result > 0 && received < record_size);
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


Connection*
Connection::CreateObject(const PeerAddress& address)
{
	switch (address.fProtocol) {
		case IPPROTO_TCP:
			return new(std::nothrow) ConnectionStream(address);
		case IPPROTO_UDP:
			return new(std::nothrow) ConnectionPacket(address);
		default:
			return NULL;
	}
}


status_t
Connection::Connect(Connection **_connection, const PeerAddress& address)
{
	Connection* conn = CreateObject(address);
	if (conn == NULL)
		return B_NO_MEMORY;

	status_t result;
	if (conn->fWaitCancel < B_OK) {
		result = conn->fWaitCancel;
		delete conn;
		return result;
	}

	result = conn->Connect();
	if (result != B_OK) {
		delete conn;
		return result;
	}

	*_connection = conn;

	return B_OK;
}


status_t
Connection::SetTo(Connection **_connection, int socket,
	const PeerAddress& address)
{
	Connection* conn = CreateObject(address);
	if (conn == NULL)
		return B_NO_MEMORY;

	status_t result;
	if (conn->fWaitCancel < B_OK) {
		result = conn->fWaitCancel;
		delete conn;
		return result;
	}

	conn->fSocket = socket;

	*_connection = conn;

	return B_OK;
}


status_t
Connection::Connect()
{
	const sockaddr& address =
		*reinterpret_cast<const sockaddr*>(&fPeerAddress);

	switch (fPeerAddress.fProtocol) {
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

	status_t result;
	uint16 port, attempt = 0;

	sockaddr_in addr;
	sockaddr_in6 addr6;
	switch (address.sa_family) {
		case AF_INET:
			memset(&addr, 0, sizeof(addr));
			addr.sin_len = sizeof(addr);
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = INADDR_ANY;
			break;
		case AF_INET6:
			memset(&addr6, 0, sizeof(addr6));
			addr6.sin6_len = sizeof(addr6);
			addr6.sin6_family = AF_INET6;
			break;
	}

	do {
		port = rand() % (IPPORT_RESERVED - NFS_MIN_PORT);
		port += NFS_MIN_PORT;

		if (attempt == 9)
			port = 0;
		attempt++;

		switch (address.sa_family) {
			case AF_INET:
				addr.sin_port = htons(port);
				result = bind(fSocket, (struct sockaddr*)&addr, sizeof(addr));
				break;
			case AF_INET6:
				addr6.sin6_port = htons(port);
				result = bind(fSocket, (struct sockaddr*)&addr6, sizeof(addr6));
				break;
		}
	} while (attempt <= 10 && result != B_OK);

	if (attempt > 10) {
		close(fSocket);
		return result;
	}

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

	result = connect(fSocket, &address, addressSize);
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
ConnectionBase::Disconnect()
{
	release_sem(fWaitCancel);

	close(fSocket);
	fSocket = -1;
}


status_t
ConnectionListener::Listen(ConnectionListener** listener, uint16 port)
{
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0)
		return errno;

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_len = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(port);
	if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) != B_OK) {
		close(sock);
		return errno;
	}

	if (listen(sock, 5) != B_OK) {
		close(sock);
		return errno;
	}

	PeerAddress address;
	address.fProtocol = IPPROTO_TCP;
	memset(&address.fAddress, 0, sizeof(address.fAddress));

	*listener = new(std::nothrow) ConnectionListener(address);
	if (*listener == NULL) {
		close(sock);
		return B_NO_MEMORY;
	}

	status_t result;
	if ((*listener)->fWaitCancel < B_OK) {
		result = (*listener)->fWaitCancel;
		close(sock);
		delete *listener;
		return result;
	}

	(*listener)->fSocket = sock;

	return B_OK;
}


status_t
ConnectionListener::AcceptConnection(Connection** connection)
{
	object_wait_info object[2];
	object[0].object = fWaitCancel;
	object[0].type = B_OBJECT_TYPE_SEMAPHORE;
	object[0].events = B_EVENT_ACQUIRE_SEMAPHORE;

	object[1].object = fSocket;
	object[1].type = B_OBJECT_TYPE_FD;
	object[1].events = B_EVENT_READ;

	do {
		status_t result = wait_for_objects(object, 2);
		if (result < B_OK ||
			(object[0].events & B_EVENT_ACQUIRE_SEMAPHORE) != 0) {
			return ECONNABORTED;
		} else if ((object[1].events & B_EVENT_READ) == 0)
			continue;
		break;
	} while (true);

	sockaddr_storage addr;
	socklen_t length = sizeof(addr);
	int sock = accept(fSocket, reinterpret_cast<sockaddr*>(&addr), &length);
	if (sock < 0)
		return errno;

	PeerAddress address;
	address.fProtocol = IPPROTO_TCP;
	address.fAddress = addr;

	status_t result = Connection::SetTo(connection, sock, address);
	if (result != B_OK) {
		close(sock);
		return result;
	}

	return B_OK;
}

