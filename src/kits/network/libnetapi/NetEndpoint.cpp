/******************************************************************************
/
/	File:			NetEndpoint.cpp
/
/   Description:    The Network API.
/
/	Copyright 2002, OpenBeOS Project, All Rights Reserved.
/
******************************************************************************/

#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include "NetEndpoint.h"

BNetEndpoint::BNetEndpoint(int protocol)
	:	fStatus(B_OK),
		fSocket(0),
		fTimeout(B_INFINITE_TIMEOUT),
		fLocalAddr(),
		fRemoteAddr()
{
	if ((fSocket = socket(AF_INET, protocol, 0)) < 0)
		fStatus = B_ERROR;
}

BNetEndpoint::BNetEndpoint(int handle, bigtime_t timeout)
	:   fStatus(B_OK),
		fSocket(0),
		fTimeout(timeout),
		fLocalAddr(),
		fRemoteAddr()
{
	if ((fSocket = dup(handle)) < 0)
		fStatus = B_ERROR;
}

BNetEndpoint::BNetEndpoint(const BNetEndpoint & endpoint)
	:	fStatus(endpoint.fStatus),
		fSocket(endpoint.fSocket),
		fTimeout(endpoint.fTimeout),
		fLocalAddr(endpoint.fLocalAddr),
		fRemoteAddr(endpoint.fRemoteAddr)
{
}

BNetEndpoint::~BNetEndpoint()
{
	Close();
}

BNetEndpoint::BNetEndpoint(BMessage *archive)
	:	fStatus(B_OK),
		fSocket(0),
		fTimeout(B_INFINITE_TIMEOUT),
		fLocalAddr(),
		fRemoteAddr()
{
	// TODO
	fStatus = B_ERROR;
}

status_t BNetEndpoint::Archive(BMessage *into, bool deep = true) const
{
	// TODO
	return B_ERROR;
}

BArchivable *BNetEndpointInstantiate(BMessage *archive)
{
	// TODO
	return NULL;
}

status_t BNetEndpoint::InitCheck() const
{
	return fStatus;
}

int BNetEndpoint::Socket() const
{
	return fSocket;
}

const BNetAddress & BNetEndpoint::LocalAddr() const
{
	return fLocalAddr;
}

const BNetAddress & BNetEndpoint::RemoteAddr() const
{
	return fRemoteAddr;
}

status_t BNetEndpoint::SetProtocol(int protocol)
{
	Close();
	if ((fSocket = ::socket(AF_INET, protocol, 0)) < 0)
		return B_ERROR;
	return B_OK;
}

status_t BNetEndpoint::SetOption(int32 option, int32 level,
							 	 const void * data, unsigned int length)
{
	if (setsockopt(fSocket, level, option, data, length) < 0)
		return B_ERROR;
	return B_OK;
}

status_t BNetEndpoint::SetNonBlocking(bool enable)
{
	int flags = fcntl(fSocket, F_GETFL);
	if (enable)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;
	if (fcntl(fSocket, F_SETFL, flags) < 0)
		return B_ERROR;
	return B_OK;
}

status_t BNetEndpoint::SetReuseAddr(bool enable)
{
	return SetOption(SO_REUSEADDR, SOL_SOCKET, &enable, sizeof(enable));
}

void BNetEndpoint::SetTimeout(bigtime_t timeout)
{
	fTimeout = timeout;
}

status_t BNetEndpoint::Bind(const BNetAddress & address)
{
	sockaddr_in addr;
	if (address.GetAddr(addr) == B_OK) {
		if (bind(fSocket, (sockaddr *) &addr, sizeof(addr)) >= 0) {
			sockaddr_in name;
			int length = sizeof(name);

			if (getsockname(fSocket, (sockaddr *) &name, &length) >= 0)
				fLocalAddr.SetTo(name);

			return B_OK;
		}
	}
	return B_ERROR;
}

status_t BNetEndpoint::Bind(int port)
{
	char hostname[256];

	if (gethostname(hostname, sizeof(hostname)) >= 0)
		return BNetEndpoint::Bind(BNetAddress(hostname, port));
	return B_ERROR;
}

void BNetEndpoint::Close()
{
	close(fSocket);
	fSocket = 0;
}

status_t BNetEndpoint::Connect(const BNetAddress & address)
{
	sockaddr_in addr;
	if (address.GetAddr(addr) == B_OK) {
		if (connect(fSocket, (sockaddr *) &addr, sizeof(addr)) >= 0) {
			sockaddr_in name;
			int length = sizeof(name);

			if (getpeername(fSocket, (sockaddr *) &name, &length) >= 0)
				fRemoteAddr.SetTo(name);
			return B_OK;
		}
	}
	return B_ERROR;
}

status_t BNetEndpoint::Connect(const char * hostname, int port)
{
	BNetAddress address(hostname, port);

	return BNetEndpoint::Connect(address);
}

status_t BNetEndpoint::Listen(int backlog)
{
	if (listen(fSocket, backlog) < 0)
		return B_ERROR;
	return B_OK;
}

BNetEndpoint *BNetEndpoint::Accept(int32 timeout)
{
	if (IsDataPending(timeout < 0 ? B_INFINITE_TIMEOUT : 1000LL * timeout)) {
		sockaddr_in name;
		int length = sizeof(name);

		int handle = accept(fSocket, (sockaddr *) &name, &length);
		if (handle >= 0) {
			BNetEndpoint * endpoint = new BNetEndpoint(handle, fTimeout);
			if (endpoint != NULL) {
				endpoint->fRemoteAddr.SetTo(name);
				if (getsockname(handle, (sockaddr *) &name, &length) >= 0)
					endpoint->fLocalAddr.SetTo(name);
			}
			return endpoint;
		}
	}
	return NULL;
}

bool BNetEndpoint::IsDataPending(bigtime_t timeout)
{	
	fd_set fds;
	struct timeval tv;
	
	FD_ZERO(&fds);
	FD_SET(fSocket, &fds);
	
	tv.tv_sec = timeout / 1000000;
	tv.tv_usec = (timeout % 1000000);
	
	return (select(fSocket + 1, &fds, NULL, NULL, &tv) > 0);
}

int32 BNetEndpoint::Receive(void * buffer, size_t length, int flags)
{
	if (IsDataPending(fTimeout))
		return recv(fSocket, buffer, length, flags);
	return -1;
}

int32 BNetEndpoint::Receive(BNetBuffer & buffer, size_t length, int flags)
{
	BNetBuffer chunk(length);
	length = Receive(chunk.Data(), chunk.Size(), flags);
	buffer.AppendData(chunk.Data(), length);
	return length;
}

int32 BNetEndpoint::ReceiveFrom(void * buffer, size_t length,
							    const BNetAddress & address, int flags)
{
	sockaddr_in addr;
	int addrlen = sizeof(addr);
	if (address.GetAddr(addr) == B_OK && IsDataPending(fTimeout))
		return recvfrom(fSocket, buffer, length, flags,
						(sockaddr *) &addr, &addrlen);
	return -1;
}

int32 BNetEndpoint::ReceiveFrom(BNetBuffer & buffer,
							    const BNetAddress & address, int flags)
{
	return ReceiveFrom(buffer.Data(), buffer.Size(), address, flags);
}

int32 BNetEndpoint::Send(const void * buffer, size_t length, int flags)
{
	return send(fSocket, (const char *) buffer, length, flags);
}

int32 BNetEndpoint::Send(const BNetBuffer & buffer, int flags)
{
	return Send(buffer.Data(), buffer.Size(), flags);
}

int32 BNetEndpoint::SendTo(const void * buffer, size_t length,
						   const BNetAddress & address, int flags)
{
	sockaddr_in addr;
	address.GetAddr(addr);
	return sendto(fSocket, buffer, length, flags,
				  (sockaddr *) &addr, sizeof(addr));
}

int32 BNetEndpoint::SendTo(const BNetBuffer & buffer,
						   const BNetAddress & address, int flags)
{
	return SendTo(buffer.Data(), buffer.Size(), address, flags);
}
