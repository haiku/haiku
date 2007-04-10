/*
 * Copyright 2002-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <r5_compatibility.h>

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
	fInit(B_NO_INIT),
	fSocket(-1),
	fTimeout(B_INFINITE_TIMEOUT),
	fLastError(0)
{
	if ((fSocket = socket(__gR5Compatibility ? R5_AF_INET : AF_INET, type, 0)) < 0)
		fLastError = errno;
	else
		fInit = B_OK;
}


BNetEndpoint::BNetEndpoint(int family, int type, int protocol)
	:
	fInit(B_NO_INIT),
	fSocket(-1),
	fTimeout(B_INFINITE_TIMEOUT),
	fLastError(0)
{
	if ((fSocket = socket(family, type, protocol)) < 0)
		fLastError = errno;
	else
		fInit = B_OK;
}


BNetEndpoint::BNetEndpoint(BMessage* archive)
	:
	fInit(B_NO_INIT),
	fSocket(-1),
	fTimeout(B_INFINITE_TIMEOUT),
	fLastError(0)
{
	// TODO
	if (! archive)
		return;

	BMessage msg;
	if (archive->FindMessage("bnendp_peer", &msg) != B_OK)
		return;
	fPeer = BNetAddress(&msg);
	
	if (archive->FindMessage("bnendp_addr", &msg) != B_OK)
		return;
	fAddr = BNetAddress(&msg);

	fInit = B_OK;
}


BNetEndpoint::BNetEndpoint(const BNetEndpoint& endpoint)
	:
	fInit(endpoint.fInit),
	fTimeout(endpoint.fTimeout),
	fLastError(endpoint.fLastError),
	fAddr(endpoint.fAddr),
	fPeer(endpoint.fPeer)
{
	fSocket = -1;
	if (endpoint.fSocket >= 0) {
		fSocket = dup(endpoint.fSocket);
		if (fSocket < 0) {
			fLastError = errno;
			fInit = B_NO_INIT;
		}
	}
}


BNetEndpoint&
BNetEndpoint::operator=(const BNetEndpoint& endpoint)
{
	Close();

	fInit = endpoint.fInit;
	fTimeout = endpoint.fTimeout;
	fLastError = endpoint.fLastError;
	fAddr = endpoint.fAddr;
	fPeer = endpoint.fPeer;

	fSocket = -1;
	if (endpoint.fSocket >= 0) {
		fSocket = dup(endpoint.fSocket);
		if (fSocket < 0) {
			fLastError = errno;
			fInit = B_NO_INIT;
		}
	}

    return *this;
}


BNetEndpoint::~BNetEndpoint()
{
	Close();
}


// #pragma mark -


status_t
BNetEndpoint::Archive(BMessage* into, bool deep) const
{
	// TODO
	if (into == 0)
		return B_ERROR;

	if (fInit != B_OK)
		return B_NO_INIT;

	BMessage msg;
	if (fPeer.Archive(&msg) != B_OK)
		return B_ERROR;
	if (into->AddMessage("bnendp_peer", &msg) != B_OK)
		return B_ERROR;

	if (fAddr.Archive(&msg) != B_OK)
		return B_ERROR;
	if (into->AddMessage("bnendp_addr", &msg) != B_OK)
		return B_ERROR;

	return B_OK;
}


BArchivable*
BNetEndpoint::Instantiate(BMessage* archive)
{
	if (!archive)
		return NULL;

	if (!validate_instantiation(archive, "BNetAddress"))
		return NULL;

	BNetEndpoint* endpoint = new BNetEndpoint(archive);
	if (endpoint && endpoint->InitCheck() == B_OK)
		return endpoint;

	delete endpoint;
	return NULL;
}


// #pragma mark -


status_t
BNetEndpoint::InitCheck()
{
	return fInit;
}


int
BNetEndpoint::Socket() const
{
	return fSocket;
}


const BNetAddress&
BNetEndpoint::LocalAddr()
{
	return fAddr;
}


const BNetAddress&
BNetEndpoint::RemoteAddr()
{
	return fPeer;
}


status_t
BNetEndpoint::SetProtocol(int protocol)
{
	Close();
	if ((fSocket = socket(AF_INET, protocol, 0)) < 0) {
		fLastError = errno;
		return fLastError;
	}
	fInit = B_OK;	
	return fInit;
}


int
BNetEndpoint::SetOption(int32 option, int32 level,
	const void* data, unsigned int length)
{
	if (fInit < B_OK)
		return fInit;

	if (setsockopt(fSocket, level, option, data, length) < 0) {
		fLastError = errno;
		return B_ERROR;
	}

	return B_OK;
}


int
BNetEndpoint::SetNonBlocking(bool enable)
{
	int flags = fcntl(fSocket, F_GETFL);
	if (flags < 0) {
		fLastError = errno;
		return B_ERROR;
	}

	if (enable)
		flags |= O_NONBLOCK;
	else
		flags &= ~O_NONBLOCK;

	if (fcntl(fSocket, F_SETFL, flags) < 0) {
		fLastError = errno;
		return B_ERROR;
	}

	return B_OK;
}


int
BNetEndpoint::SetReuseAddr(bool enable)
{
	int onoff = (int) enable;
	return SetOption(SO_REUSEADDR, SOL_SOCKET, &onoff, sizeof(onoff));
}


void
BNetEndpoint::SetTimeout(bigtime_t timeout)
{
	fTimeout = timeout;
}


int
BNetEndpoint::Error() const
{
	return fLastError;
}


char*
BNetEndpoint::ErrorStr() const
{
	return strerror(fLastError);
}


// #pragma mark -


void
BNetEndpoint::Close()
{
	if (fSocket >= 0)
		close(fSocket);

	fSocket = -1;
	fInit = B_NO_INIT;
}


status_t
BNetEndpoint::Bind(const BNetAddress& address)
{
	if (fInit < B_OK)
		return fInit;

	struct sockaddr_in addr;
	status_t status = address.GetAddr(addr);
	if (status != B_OK)
		return status;

	if (bind(fSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		fLastError = errno;
		Close();
		return B_ERROR;
	}

	socklen_t addrSize = sizeof(addr);
	if (getsockname(fSocket, (struct sockaddr *)&addr, &addrSize) < 0) {
		fLastError = errno;
		Close();
		return B_ERROR;
	}

	if (addr.sin_addr.s_addr == 0) {
		// TODO: does this still apply?
		// Grrr, buggy getsockname!
		char hostname[MAXHOSTNAMELEN];
		gethostname(hostname, sizeof(hostname));
		struct hostent *host = gethostbyname(hostname);
		if (host != NULL)
			memcpy(&addr.sin_addr.s_addr, host->h_addr, sizeof(addr.sin_addr.s_addr));
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
	if (fInit < B_OK)
		return fInit;

	sockaddr_in addr;
	if (address.GetAddr(addr) != B_OK)
		return B_ERROR;

	if (connect(fSocket, (sockaddr *) &addr, sizeof(addr)) < 0) {
		Close();
		fLastError = errno;
		return B_ERROR;
	}

	socklen_t addrSize = sizeof(addr);
	if (getpeername(fSocket, (sockaddr *) &addr, &addrSize) < 0) {
		Close();
		fLastError = errno;
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
	if (fInit < B_OK)
		return fInit;

	if (listen(fSocket, backlog) < 0) {
		Close();
		fLastError = errno;
		return B_ERROR;
	}
	return B_OK;
}


BNetEndpoint*
BNetEndpoint::Accept(int32 timeout)
{
	if (!IsDataPending(timeout < 0 ? B_INFINITE_TIMEOUT : 1000LL * timeout))
		return NULL;

	struct sockaddr_in addr;
	socklen_t addrSize = sizeof(addr);

	int socket = accept(fSocket, (struct sockaddr *) &addr, &addrSize);
	if (socket < 0) {
		Close();
		fLastError = errno;
		return NULL;
	}

	BNetEndpoint* endpoint = new (std::nothrow) BNetEndpoint(*this);
	if (endpoint == NULL) {
		close(socket);
		fLastError = B_NO_MEMORY;
		return NULL;
	}

	endpoint->fSocket = socket;
	endpoint->fPeer.SetTo(addr);

	if (getsockname(socket, (struct sockaddr *)&addr, &addrSize) < 0) {
		delete endpoint;
		fLastError = errno;
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
		tv.tv_usec = (timeout % 1000000);
	}

	if (select(fSocket + 1, &fds, NULL, NULL, timeout > 0 ? &tv : NULL) < 0) {
		fLastError = errno;
		return false;
	}

	return FD_ISSET(fSocket, &fds);
}


int32
BNetEndpoint::Receive(void* buffer, size_t length, int flags)
{
	if (fTimeout >= 0 && IsDataPending(fTimeout) == false)
		return 0;

	ssize_t bytesReceived = recv(fSocket, buffer, length, flags);
	if (bytesReceived < 0)
		fLastError = errno;

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
	if (fTimeout >= 0 && IsDataPending(fTimeout) == false)
		return 0;

	struct sockaddr_in addr;
	socklen_t addrSize = sizeof(addr);

	length = recvfrom(fSocket, buffer, length, flags,
		(struct sockaddr *)&addr, &addrSize);
	if (length < 0)
		fLastError = errno;
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
	ssize_t bytesSent = send(fSocket, (const char *) buffer, length, flags);
	if (bytesSent < 0)
		fLastError = errno;

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
	struct sockaddr_in addr;
	if (address.GetAddr(addr) != B_OK)
		return B_ERROR;

	ssize_t	bytesSent = sendto(fSocket, buffer, length, flags,
		(struct sockaddr *) &addr, sizeof(addr));
	if (bytesSent < 0)
		fLastError = errno;

	return bytesSent;
}


int32
BNetEndpoint::SendTo(BNetBuffer& buffer,
	const BNetAddress& address, int flags)
{
	return SendTo(buffer.Data(), buffer.Size(), address, flags);
}


// #pragma mark -


// These are virtuals, implemented for binary compatibility purpose
void BNetEndpoint::_ReservedBNetEndpointFBCCruft1() {}
void BNetEndpoint::_ReservedBNetEndpointFBCCruft2() {}
void BNetEndpoint::_ReservedBNetEndpointFBCCruft3() {}
void BNetEndpoint::_ReservedBNetEndpointFBCCruft4() {}
void BNetEndpoint::_ReservedBNetEndpointFBCCruft5() {}
void BNetEndpoint::_ReservedBNetEndpointFBCCruft6() {}

