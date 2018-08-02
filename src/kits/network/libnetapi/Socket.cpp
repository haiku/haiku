/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Distributed under the terms of the MIT License.
 */


#include <Socket.h>

#include <errno.h>


//#define TRACE_SOCKET
#ifdef TRACE_SOCKET
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...) ;
#endif


BSocket::BSocket()
{
}


BSocket::BSocket(const BNetworkAddress& peer, bigtime_t timeout)
{
	Connect(peer, timeout);
}


BSocket::BSocket(const BSocket& other)
	:
	BAbstractSocket(other)
{
}


BSocket::~BSocket()
{
}


status_t
BSocket::Bind(const BNetworkAddress& local, bool reuseAddr)
{
	return BAbstractSocket::Bind(local, SOCK_STREAM, reuseAddr);
}


status_t
BSocket::Connect(const BNetworkAddress& peer, bigtime_t timeout)
{
	return BAbstractSocket::Connect(peer, SOCK_STREAM, timeout);
}


status_t
BSocket::Accept(BAbstractSocket*& _socket)
{
	int fd = -1;
	BNetworkAddress peer;
	status_t error = AcceptNext(fd, peer);
	if (error != B_OK)
		return error;
	BSocket* socket = new(std::nothrow) BSocket();
	if (socket == NULL) {
		close(fd);
		return B_NO_MEMORY;
	}

	socket->_SetTo(fd, fLocal, peer);
	_socket = socket;
	return B_OK;
}


//	#pragma mark - BDataIO implementation


ssize_t
BSocket::Read(void* buffer, size_t size)
{
	ssize_t bytesReceived = recv(Socket(), buffer, size, 0);
	if (bytesReceived < 0) {
		TRACE("%p: BSocket::Read() error: %s\n", this, strerror(errno));
		return errno;
	}

	return bytesReceived;
}


ssize_t
BSocket::Write(const void* buffer, size_t size)
{
	ssize_t bytesSent = send(Socket(), buffer, size, 0);
	if (bytesSent < 0) {
		TRACE("%p: BSocket::Write() error: %s\n", this, strerror(errno));
		return errno;
	}

	return bytesSent;
}


//	#pragma mark - private


void
BSocket::_SetTo(int fd, const BNetworkAddress& local,
	const BNetworkAddress& peer)
{
	Disconnect();

	fInitStatus = B_OK;
	fSocket = fd;
	fLocal = local;
	fPeer = peer;
	fIsConnected = true;

	TRACE("%p: accepted from %s to %s\n", this, local.ToString().c_str(),
		peer.ToString().c_str());
}
