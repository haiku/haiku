/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <DatagramSocket.h>


//#define TRACE_SOCKET
#ifdef TRACE_SOCKET
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...) ;
#endif


BDatagramSocket::BDatagramSocket()
{
}


BDatagramSocket::BDatagramSocket(const BNetworkAddress& peer, bigtime_t timeout)
{
	Connect(peer, timeout);
}


BDatagramSocket::BDatagramSocket(const BDatagramSocket& other)
	:
	BAbstractSocket(other)
{
}


BDatagramSocket::~BDatagramSocket()
{
}


status_t
BDatagramSocket::Bind(const BNetworkAddress& local)
{
	return BAbstractSocket::Bind(local, SOCK_DGRAM);
}


status_t
BDatagramSocket::Connect(const BNetworkAddress& peer, bigtime_t timeout)
{
	return BAbstractSocket::Connect(peer, SOCK_DGRAM, timeout);
}


status_t
BDatagramSocket::SetBroadcast(bool broadcast)
{
	int value = broadcast ? 1 : 0;
	if (setsockopt(fSocket, SOL_SOCKET, SO_BROADCAST, &value, sizeof(value))
			!= 0)
		return errno;

	return B_OK;
}


void
BDatagramSocket::SetPeer(const BNetworkAddress& peer)
{
	fPeer = peer;
}


size_t
BDatagramSocket::MaxTransmissionSize() const
{
	// TODO: might vary on family!
	return 32768;
}


size_t
BDatagramSocket::SendTo(const BNetworkAddress& address, const void* buffer,
	size_t size)
{
	ssize_t bytesSent = sendto(fSocket, buffer, size, 0, address,
		address.Length());
	if (bytesSent < 0)
		return errno;

	return bytesSent;
}


size_t
BDatagramSocket::ReceiveFrom(void* buffer, size_t bufferSize,
	BNetworkAddress& from)
{
	socklen_t fromLength = sizeof(sockaddr_storage);
	ssize_t bytesReceived = recvfrom(fSocket, buffer, bufferSize, 0,
		from, &fromLength);
	if (bytesReceived < 0)
		return errno;

	return bytesReceived;
}


//	#pragma mark - BDataIO implementation


ssize_t
BDatagramSocket::Read(void* buffer, size_t size)
{
	ssize_t bytesReceived = recv(Socket(), buffer, size, 0);
	if (bytesReceived < 0) {
		TRACE("%p: BSocket::Read() error: %s\n", this, strerror(errno));
		return errno;
	}

	return bytesReceived;
}


ssize_t
BDatagramSocket::Write(const void* buffer, size_t size)
{
	ssize_t bytesSent;

	if (!fIsConnected)
		bytesSent = sendto(Socket(), buffer, size, 0, fPeer, fPeer.Length());
	else
		bytesSent = send(Socket(), buffer, size, 0);

	if (bytesSent < 0) {
		TRACE("%p: BDatagramSocket::Write() error: %s\n", this,
			strerror(errno));
		return errno;
	}

	return bytesSent;
}
