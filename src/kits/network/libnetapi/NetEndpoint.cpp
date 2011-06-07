/*
 * Copyright 2002-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

#include <Message.h>
#include <NetEndpoint.h>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <new>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


BNetEndpoint::BNetEndpoint(int type)
	:
	fStatus(B_NO_INIT),
	fFamily(AF_INET),
	fType(type),
	fProtocol(0),
	fSocket(-1),
	fTimeout(B_INFINITE_TIMEOUT)
{
	_SetupSocket();
}


BNetEndpoint::BNetEndpoint(int family, int type, int protocol)
	:
	fStatus(B_NO_INIT),
	fFamily(family),
	fType(type),
	fProtocol(protocol),
	fSocket(-1),
	fTimeout(B_INFINITE_TIMEOUT)
{
	_SetupSocket();
}


BNetEndpoint::BNetEndpoint(BMessage* archive)
	:
	fStatus(B_NO_INIT),
	fFamily(AF_INET),
	fProtocol(0),
	fSocket(-1),
	fTimeout(B_INFINITE_TIMEOUT)
{
	if (!archive)
		return;

	in_addr addr, peer;
	unsigned short addrPort = 0, peerPort = 0;

	fStatus = archive->FindInt32("_BNetEndpoint_addr_addr",
		(int32 *)&addr.s_addr);
	if (fStatus == B_OK) {
		fStatus = archive->FindInt16("_BNetEndpoint_addr_port",
			(int16 *)&addrPort);
		if (fStatus == B_OK)
			fStatus = fAddr.SetTo(addr, addrPort);
	}

	fStatus = archive->FindInt32("_BNetEndpoint_peer_addr",
		(int32 *)&peer.s_addr);
	if (fStatus == B_OK) {
		fStatus = archive->FindInt16("_BNetEndpoint_peer_port",
			(int16 *)&peerPort);
		if (fStatus == B_OK)
			fStatus = fPeer.SetTo(peer, peerPort);
	}

	fStatus = archive->FindInt64("_BNetEndpoint_timeout", (int64 *)&fTimeout);
	if (fStatus == B_OK)
		fStatus = archive->FindInt32("_BNetEndpoint_proto", (int32 *)&fType);

	if (fStatus == B_OK)
		_SetupSocket();
}


BNetEndpoint::BNetEndpoint(const BNetEndpoint& endpoint)
	:
	fStatus(endpoint.fStatus),
	fFamily(endpoint.fFamily),
	fType(endpoint.fType),
	fProtocol(endpoint.fProtocol),
	fSocket(-1),
	fTimeout(endpoint.fTimeout),
	fAddr(endpoint.fAddr),
	fPeer(endpoint.fPeer)

{
	if (endpoint.fSocket >= 0) {
		fSocket = dup(endpoint.fSocket);
		if (fSocket < 0)
			fStatus = errno;
	}
}


BNetEndpoint&
BNetEndpoint::operator=(const BNetEndpoint& endpoint)
{
	if (this == &endpoint)
		return *this;

	Close();

	fStatus = endpoint.fStatus;
	fFamily = endpoint.fFamily;
	fType = endpoint.fType;
	fProtocol = endpoint.fProtocol;
	fTimeout = endpoint.fTimeout;
	fAddr = endpoint.fAddr;
	fPeer = endpoint.fPeer;

	fSocket = -1;
	if (endpoint.fSocket >= 0) {
		fSocket = dup(endpoint.fSocket);
		if (fSocket < 0)
			fStatus = errno;
	}

    return *this;
}


BNetEndpoint::~BNetEndpoint()
{
	if (fSocket >= 0)
		Close();
}


// #pragma mark -


status_t
BNetEndpoint::Archive(BMessage* into, bool deep) const
{
	if (!into)
		return B_ERROR;

	status_t status = BArchivable::Archive(into, deep);
	if (status != B_OK)
		return status;

	in_addr addr, peer;
	unsigned short addrPort, peerPort;

	status = fAddr.GetAddr(addr, &addrPort);
	if (status == B_OK) {
		status = into->AddInt32("_BNetEndpoint_addr_addr", addr.s_addr);
		if (status == B_OK)
			status = into->AddInt16("_BNetEndpoint_addr_port", addrPort);
		if (status != B_OK)
			return status;
	}
	status = fPeer.GetAddr(peer, &peerPort);
	if (status == B_OK) {
		status = into->AddInt32("_BNetEndpoint_peer_addr", peer.s_addr);
		if (status == B_OK)
			status = into->AddInt16("_BNetEndpoint_peer_port", peerPort);
		if (status != B_OK)
			return status;
	}

	status = into->AddInt64("_BNetEndpoint_timeout", fTimeout);
	if (status == B_OK)
		status = into->AddInt32("_BNetEndpoint_proto", fType);

	return status;
}


BArchivable*
BNetEndpoint::Instantiate(BMessage* archive)
{
	if (!archive)
		return NULL;

	if (!validate_instantiation(archive, "BNetEndpoint"))
		return NULL;

	BNetEndpoint* endpoint = new BNetEndpoint(archive);
	if (endpoint && endpoint->InitCheck() == B_OK)
		return endpoint;

	delete endpoint;
	return NULL;
}


// #pragma mark -


status_t
BNetEndpoint::InitCheck() const
{
	return fSocket == -1 ? B_NO_INIT : B_OK;
}


int
BNetEndpoint::Socket() const
{
	return fSocket;
}


const BNetAddress&
BNetEndpoint::LocalAddr() const
{
	return fAddr;
}


const BNetAddress&
BNetEndpoint::RemoteAddr() const
{
	return fPeer;
}


status_t
BNetEndpoint::SetProtocol(int protocol)
{
	Close();
	fType = protocol;	// sic (protocol is SOCK_DGRAM or SOCK_STREAM)
	return _SetupSocket();
}


int
BNetEndpoint::SetOption(int32 option, int32 level,
	const void* data, unsigned int length)
{
	if (fSocket < 0 && _SetupSocket() != B_OK)
		return fStatus;

	if (setsockopt(fSocket, level, option, data, length) < 0) {
		fStatus = errno;
		return B_ERROR;
	}

	return B_OK;
}


int
BNetEndpoint::SetNonBlocking(bool enable)
{
	if (fSocket < 0 && _SetupSocket() != B_OK)
		return fStatus;

	int flags = fcntl(fSocket, F_GETFL);
	if (flags < 0) {
		fStatus = errno;
		return B_ERROR;
	}

	if (enable)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;

	if (fcntl(fSocket, F_SETFL, flags) < 0) {
		fStatus = errno;
		return B_ERROR;
	}

	return B_OK;
}


int
BNetEndpoint::SetReuseAddr(bool enable)
{
	if (fSocket < 0 && _SetupSocket() != B_OK)
		return fStatus;

	int onoff = (int) enable;
	return SetOption(SO_REUSEADDR, SOL_SOCKET, &onoff, sizeof(onoff));
}


void
BNetEndpoint::SetTimeout(bigtime_t timeout)
{
	fTimeout = timeout;
	// TODO: Map value < 0 to B_INFINITE_TIMEOUT or use -1 by default.
}


int
BNetEndpoint::Error() const
{
	return (int)fStatus;
}


char*
BNetEndpoint::ErrorStr() const
{
	return strerror(fStatus);
}


// #pragma mark -


void
BNetEndpoint::Close()
{
	if (fSocket >= 0)
		close(fSocket);

	fSocket = -1;
	fStatus = B_NO_INIT;
}


status_t
BNetEndpoint::Bind(const BNetAddress& address)
{
	if (fSocket < 0 && _SetupSocket() != B_OK)
		return fStatus;

	struct sockaddr_in addr;
	status_t status = address.GetAddr(addr);
	if (status != B_OK)
		return status;

	if (bind(fSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		fStatus = errno;
		Close();
		return B_ERROR;
	}

	socklen_t addrSize = sizeof(addr);
	if (getsockname(fSocket, (struct sockaddr *)&addr, &addrSize) < 0) {
		fStatus = errno;
		Close();
		return B_ERROR;
	}

	fAddr.SetTo(addr);
	return B_OK;
}


status_t
BNetEndpoint::Bind(int port)
{
	BNetAddress addr(INADDR_ANY, port);
	return Bind(addr);
}


status_t
BNetEndpoint::Connect(const BNetAddress& address)
{
	if (fSocket < 0 && _SetupSocket() != B_OK)
		return fStatus;

	sockaddr_in addr;
	if (address.GetAddr(addr) != B_OK)
		return B_ERROR;

	if (connect(fSocket, (sockaddr *) &addr, sizeof(addr)) < 0) {
		Close();
		fStatus = errno;
		return B_ERROR;
	}

	socklen_t addrSize = sizeof(addr);
	if (getpeername(fSocket, (sockaddr *) &addr, &addrSize) < 0) {
		Close();
		fStatus = errno;
		return B_ERROR;
	}
	fPeer.SetTo(addr);
	return B_OK;
}


status_t
BNetEndpoint::Connect(const char *hostname, int port)
{
	BNetAddress addr(hostname, port);
	return Connect(addr);
}


status_t
BNetEndpoint::Listen(int backlog)
{
	if (fSocket < 0 && _SetupSocket() != B_OK)
		return fStatus;

	if (listen(fSocket, backlog) < 0) {
		Close();
		fStatus = errno;
		return B_ERROR;
	}
	return B_OK;
}


BNetEndpoint*
BNetEndpoint::Accept(int32 timeout)
{
	// TODO: IsDataPending() expects 0 as special value for infinite timeout,
	// hence the following call is broken for timeout < 0 and timeout == 0.
	if (!IsDataPending(timeout < 0 ? B_INFINITE_TIMEOUT : 1000LL * timeout))
		return NULL;

	struct sockaddr_in addr;
	socklen_t addrSize = sizeof(addr);

	int socket = accept(fSocket, (struct sockaddr *) &addr, &addrSize);
	if (socket < 0) {
		Close();
		fStatus = errno;
		return NULL;
	}

	BNetEndpoint* endpoint = new (std::nothrow) BNetEndpoint(*this);
	if (endpoint == NULL) {
		close(socket);
		fStatus = B_NO_MEMORY;
		return NULL;
	}

	endpoint->fSocket = socket;
	endpoint->fPeer.SetTo(addr);

	if (getsockname(socket, (struct sockaddr *)&addr, &addrSize) < 0) {
		delete endpoint;
		fStatus = errno;
		return NULL;
	}

	endpoint->fAddr.SetTo(addr);
	return endpoint;
}


// #pragma mark -


bool
BNetEndpoint::IsDataPending(bigtime_t timeout)
{
	struct timeval tv;
	fd_set fds;

	FD_ZERO(&fds);
	FD_SET(fSocket, &fds);

	if (timeout > 0) {
		tv.tv_sec = timeout / 1000000;
			// TODO: A big value (e.g. B_INFINITE_TIMEOUT) will overflow!
		tv.tv_usec = (timeout % 1000000);
	}

	if (select(fSocket + 1, &fds, NULL, NULL, timeout > 0 ? &tv : NULL) < 0) {
		fStatus = errno;
		return false;
	}

	return FD_ISSET(fSocket, &fds);
}


int32
BNetEndpoint::Receive(void* buffer, size_t length, int flags)
{
	if (fSocket < 0 && _SetupSocket() != B_OK)
		return fStatus;

	// TODO: For fTimeout == 0 this is broken as IsDataPending(0) means wait
	// without timeout. Furthermore the default fTimeout is B_INFINITE_TIMEOUT.
	if (fTimeout >= 0 && IsDataPending(fTimeout) == false)
		return 0;

	ssize_t bytesReceived = recv(fSocket, buffer, length, flags);
	if (bytesReceived < 0)
		fStatus = errno;

	return bytesReceived;
}


int32
BNetEndpoint::Receive(BNetBuffer& buffer, size_t length, int flags)
{
	BNetBuffer chunk(length);
	length = Receive(chunk.Data(), length, flags);
	buffer.AppendData(chunk.Data(), length);
	return length;
}


int32
BNetEndpoint::ReceiveFrom(void* buffer, size_t length,
	BNetAddress& address, int flags)
{
	if (fSocket < 0 && _SetupSocket() != B_OK)
		return fStatus;

	// TODO: For fTimeout == 0 this is broken as IsDataPending(0) means wait
	// without timeout. Furthermore the default fTimeout is B_INFINITE_TIMEOUT.
	if (fTimeout >= 0 && IsDataPending(fTimeout) == false)
		return 0;

	struct sockaddr_in addr;
	socklen_t addrSize = sizeof(addr);

	length = recvfrom(fSocket, buffer, length, flags,
		(struct sockaddr *)&addr, &addrSize);
	if (length < 0)
		fStatus = errno;
	else
		address.SetTo(addr);

	return length;
}


int32
BNetEndpoint::ReceiveFrom(BNetBuffer& buffer, size_t length,
	BNetAddress& address, int flags)
{
	BNetBuffer chunk(length);
	length = ReceiveFrom(chunk.Data(), length, address, flags);
	buffer.AppendData(chunk.Data(), length);
	return length;
}


int32
BNetEndpoint::Send(const void* buffer, size_t length, int flags)
{
	if (fSocket < 0 && _SetupSocket() != B_OK)
		return fStatus;

	ssize_t bytesSent = send(fSocket, (const char *) buffer, length, flags);
	if (bytesSent < 0)
		fStatus = errno;

	return bytesSent;
}


int32
BNetEndpoint::Send(BNetBuffer& buffer, int flags)
{
	return Send(buffer.Data(), buffer.Size(), flags);
}


int32
BNetEndpoint::SendTo(const void* buffer, size_t length,
	const BNetAddress& address, int flags)
{
	if (fSocket < 0 && _SetupSocket() != B_OK)
		return fStatus;

	struct sockaddr_in addr;
	if (address.GetAddr(addr) != B_OK)
		return B_ERROR;

	ssize_t	bytesSent = sendto(fSocket, buffer, length, flags,
		(struct sockaddr *) &addr, sizeof(addr));
	if (bytesSent < 0)
		fStatus = errno;

	return bytesSent;
}


int32
BNetEndpoint::SendTo(BNetBuffer& buffer,
	const BNetAddress& address, int flags)
{
	return SendTo(buffer.Data(), buffer.Size(), address, flags);
}


// #pragma mark -


status_t
BNetEndpoint::_SetupSocket()
{
	if ((fSocket = socket(fFamily, fType, fProtocol)) < 0)
		fStatus = errno;
	else
		fStatus = B_OK;
	return fStatus;
}


// #pragma mark -

status_t BNetEndpoint::InitCheck()
{
	return const_cast<const BNetEndpoint*>(this)->InitCheck();
}


const BNetAddress& BNetEndpoint::LocalAddr()
{
	return const_cast<const BNetEndpoint*>(this)->LocalAddr();
}


const BNetAddress& BNetEndpoint::RemoteAddr()
{
	return const_cast<const BNetEndpoint*>(this)->RemoteAddr();
}


// #pragma mark -


// These are virtuals, implemented for binary compatibility purpose
void BNetEndpoint::_ReservedBNetEndpointFBCCruft1() {}
void BNetEndpoint::_ReservedBNetEndpointFBCCruft2() {}
void BNetEndpoint::_ReservedBNetEndpointFBCCruft3() {}
void BNetEndpoint::_ReservedBNetEndpointFBCCruft4() {}
void BNetEndpoint::_ReservedBNetEndpointFBCCruft5() {}
void BNetEndpoint::_ReservedBNetEndpointFBCCruft6() {}

