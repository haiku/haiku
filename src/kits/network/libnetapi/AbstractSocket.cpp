/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <AbstractSocket.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/poll.h>


//#define TRACE_SOCKET
#ifdef TRACE_SOCKET
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...) ;
#endif


BAbstractSocket::BAbstractSocket()
	:
	fInitStatus(B_NO_INIT),
	fSocket(-1),
	fIsBound(false),
	fIsConnected(false)
{
}


BAbstractSocket::BAbstractSocket(const BAbstractSocket& other)
	:
	fInitStatus(other.fInitStatus),
	fLocal(other.fLocal),
	fPeer(other.fPeer),
	fIsConnected(other.fIsConnected)
{
	fSocket = dup(other.fSocket);
	if (fSocket < 0)
		fInitStatus = errno;
}


BAbstractSocket::~BAbstractSocket()
{
	Disconnect();
}


status_t
BAbstractSocket::InitCheck() const
{
	return fInitStatus;
}


bool
BAbstractSocket::IsBound() const
{
	return fIsBound;
}


bool
BAbstractSocket::IsConnected() const
{
	return fIsConnected;
}


void
BAbstractSocket::Disconnect()
{
	if (fSocket < 0)
		return;

	TRACE("%p: BAbstractSocket::Disconnect()\n", this);

	close(fSocket);
	fSocket = -1;
	fIsConnected = false;
	fIsBound = false;
}


status_t
BAbstractSocket::SetTimeout(bigtime_t timeout)
{
	if (timeout < 0)
		timeout = 0;

	struct timeval tv;
	tv.tv_sec = timeout / 1000000LL;
	tv.tv_usec = timeout % 1000000LL;

	if (setsockopt(fSocket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(timeval)) != 0
		|| setsockopt(fSocket, SOL_SOCKET, SO_RCVTIMEO, &tv,
				sizeof(timeval)) != 0)
		return errno;

	return B_OK;
}


bigtime_t
BAbstractSocket::Timeout() const
{
	struct timeval tv;
	socklen_t size = sizeof(tv);
	if (getsockopt(fSocket, SOL_SOCKET, SO_SNDTIMEO, &tv, &size) != 0)
		return B_INFINITE_TIMEOUT;

	return tv.tv_sec * 1000000LL + tv.tv_usec;
}


const BNetworkAddress&
BAbstractSocket::Local() const
{
	return fLocal;
}


const BNetworkAddress&
BAbstractSocket::Peer() const
{
	return fPeer;
}


size_t
BAbstractSocket::MaxTransmissionSize() const
{
	return SSIZE_MAX;
}


status_t
BAbstractSocket::WaitForReadable(bigtime_t timeout) const
{
	return _WaitFor(POLLIN, timeout);
}


status_t
BAbstractSocket::WaitForWritable(bigtime_t timeout) const
{
	return _WaitFor(POLLOUT, timeout);
}


int
BAbstractSocket::Socket() const
{
	return fSocket;
}


//	#pragma mark - protected


status_t
BAbstractSocket::Bind(const BNetworkAddress& local, int type)
{
	fInitStatus = _OpenIfNeeded(local.Family(), type);
	if (fInitStatus != B_OK)
		return fInitStatus;

	if (bind(fSocket, local, local.Length()) != 0)
		return fInitStatus = errno;

	fIsBound = true;
	_UpdateLocalAddress();
	return B_OK;
}


status_t
BAbstractSocket::Connect(const BNetworkAddress& peer, int type,
	bigtime_t timeout)
{
	Disconnect();

	fInitStatus = _OpenIfNeeded(peer.Family(), type);
	if (fInitStatus == B_OK)
		fInitStatus = SetTimeout(timeout);

	if (fInitStatus == B_OK && !IsBound()) {
		BNetworkAddress local;
		local.SetToWildcard(peer.Family());
		fInitStatus = Bind(local);
	}
	if (fInitStatus != B_OK)
		return fInitStatus;

	BNetworkAddress normalized = peer;
	if (connect(fSocket, normalized, normalized.Length()) != 0) {
		TRACE("%p: connecting to %s: %s\n", this,
			normalized.ToString().c_str(), strerror(errno));
		return fInitStatus = errno;
	}

	fIsConnected = true;
	fPeer = normalized;
	_UpdateLocalAddress();

	TRACE("%p: connected to %s (local %s)\n", this, peer.ToString().c_str(),
		fLocal.ToString().c_str());

	return fInitStatus = B_OK;
}


//	#pragma mark - private


status_t
BAbstractSocket::_OpenIfNeeded(int family, int type)
{
	if (fSocket >= 0)
		return B_OK;

	fSocket = socket(family, type, 0);
	if (fSocket < 0)
		return errno;

	TRACE("%p: socket opened FD %d\n", this, fSocket);
	return B_OK;
}


status_t
BAbstractSocket::_UpdateLocalAddress()
{
	socklen_t localLength = sizeof(sockaddr_storage);
	if (getsockname(fSocket, fLocal, &localLength) != 0)
		return errno;

	return B_OK;
}


status_t
BAbstractSocket::_WaitFor(int flags, bigtime_t timeout) const
{
	if (fInitStatus != B_OK)
		return fInitStatus;

	int millis = 0;
	if (timeout == B_INFINITE_TIMEOUT)
		millis = -1;
	if (timeout > 0)
		millis = timeout / 1000;

	struct pollfd entry;
	entry.fd = Socket();
	entry.events = flags;

	int result = poll(&entry, 1, millis);
	if (result < 0)
		return errno;
	if (result == 0)
		return millis > 0 ? B_TIMED_OUT : B_WOULD_BLOCK;

	return B_OK;
}
