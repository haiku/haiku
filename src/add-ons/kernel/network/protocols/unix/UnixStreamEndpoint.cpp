/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "UnixStreamEndpoint.h"

#include <stdio.h>
#include <sys/stat.h>

#include <AutoDeleter.h>

#include <vfs.h>

#include "UnixAddressManager.h"
#include "UnixFifo.h"


#define UNIX_STREAM_ENDPOINT_DEBUG_LEVEL	0
#define UNIX_DEBUG_LEVEL					UNIX_STREAM_ENDPOINT_DEBUG_LEVEL
#include "UnixDebug.h"


// Note on locking order (outermost -> innermost):
// UnixStreamEndpoint: connecting -> listening -> child
// -> UnixFifo (never lock more than one at a time)
// -> UnixAddressManager


UnixStreamEndpoint::UnixStreamEndpoint(net_socket* socket)
	:
	UnixEndpoint(socket),
	fPeerEndpoint(NULL),
	fReceiveFifo(NULL),
	fState(unix_stream_endpoint_state::Closed),
	fAcceptSemaphore(-1),
	fIsChild(false),
	fWasConnected(false)
{
	TRACE("[%" B_PRId32 "] %p->UnixStreamEndpoint::UnixStreamEndpoint()\n",
		find_thread(NULL), this);
}


UnixStreamEndpoint::~UnixStreamEndpoint()
{
	TRACE("[%" B_PRId32 "] %p->UnixStreamEndpoint::~UnixStreamEndpoint()\n",
		find_thread(NULL), this);
}


status_t
UnixStreamEndpoint::Init()
{
	TRACE("[%" B_PRId32 "] %p->UnixStreamEndpoint::Init()\n", find_thread(NULL),
		this);

	RETURN_ERROR(B_OK);
}


void
UnixStreamEndpoint::Uninit()
{
	TRACE("[%" B_PRId32 "] %p->UnixStreamEndpoint::Uninit()\n", find_thread(NULL),
		this);

	// check whether we're closed
	UnixStreamEndpointLocker locker(this);
	bool closed = (fState == unix_stream_endpoint_state::Closed);
	locker.Unlock();

	if (!closed) {
		// That probably means, we're a child endpoint of a listener and
		// have been fully connected, but not yet accepted. Our Close()
		// hook isn't called in this case. Do it manually.
		Close();
	}

	ReleaseReference();
}


status_t
UnixStreamEndpoint::Open()
{
	TRACE("[%" B_PRId32 "] %p->UnixStreamEndpoint::Open()\n", find_thread(NULL),
		this);

	status_t error = ProtocolSocket::Open();
	if (error != B_OK)
		RETURN_ERROR(error);

	fState = unix_stream_endpoint_state::NotConnected;

	RETURN_ERROR(B_OK);
}


status_t
UnixStreamEndpoint::Close()
{
	TRACE("[%" B_PRId32 "] %p->UnixStreamEndpoint::Close()\n", find_thread(NULL),
		this);

	UnixStreamEndpointLocker locker(this);

	if (fState == unix_stream_endpoint_state::Connected) {
		UnixStreamEndpointLocker peerLocker;
		if (_LockConnectedEndpoints(locker, peerLocker) == B_OK) {
			// We're still connected. Disconnect both endpoints!
			fPeerEndpoint->_Disconnect();
			_Disconnect();
		}
	}

	if (fState == unix_stream_endpoint_state::Listening)
		_StopListening();

	_Unbind();

	fState = unix_stream_endpoint_state::Closed;
	RETURN_ERROR(B_OK);
}


status_t
UnixStreamEndpoint::Free()
{
	TRACE("[%" B_PRId32 "] %p->UnixStreamEndpoint::Free()\n", find_thread(NULL),
		this);

	UnixStreamEndpointLocker locker(this);

	_UnsetReceiveFifo();

	RETURN_ERROR(B_OK);
}


status_t
UnixStreamEndpoint::Bind(const struct sockaddr* _address)
{
	if (_address->sa_family != AF_UNIX)
		RETURN_ERROR(EAFNOSUPPORT);

	TRACE("[%" B_PRId32 "] %p->UnixStreamEndpoint::Bind(\"%s\")\n",
		find_thread(NULL), this,
		ConstSocketAddress(&gAddressModule, _address).AsString().Data());

	const sockaddr_un* address = (const sockaddr_un*)_address;

	UnixStreamEndpointLocker endpointLocker(this);

	if (fState != unix_stream_endpoint_state::NotConnected || IsBound())
		RETURN_ERROR(B_BAD_VALUE);

	RETURN_ERROR(_Bind(address));
}


status_t
UnixStreamEndpoint::Unbind()
{
	TRACE("[%" B_PRId32 "] %p->UnixStreamEndpoint::Unbind()\n", find_thread(NULL),
		this);

	UnixStreamEndpointLocker endpointLocker(this);

	RETURN_ERROR(_Unbind());
}


status_t
UnixStreamEndpoint::Listen(int backlog)
{
	TRACE("[%" B_PRId32 "] %p->UnixStreamEndpoint::Listen(%d)\n", find_thread(NULL),
		this, backlog);

	UnixStreamEndpointLocker endpointLocker(this);

	if (!IsBound())
		RETURN_ERROR(EDESTADDRREQ);
	if (fState != unix_stream_endpoint_state::NotConnected
		&& fState != unix_stream_endpoint_state::Listening)
		RETURN_ERROR(EINVAL);

	gSocketModule->set_max_backlog(socket, backlog);

	if (fState == unix_stream_endpoint_state::NotConnected) {
		fAcceptSemaphore = create_sem(0, "unix accept");
		if (fAcceptSemaphore < 0)
			RETURN_ERROR(ENOBUFS);

		_UnsetReceiveFifo();

		fCredentials.pid = getpid();
		fCredentials.uid = geteuid();
		fCredentials.gid = getegid();

		fState = unix_stream_endpoint_state::Listening;
	}

	RETURN_ERROR(B_OK);
}


status_t
UnixStreamEndpoint::Connect(const struct sockaddr* _address)
{
	if (_address->sa_family != AF_UNIX)
		RETURN_ERROR(EAFNOSUPPORT);

	TRACE("[%" B_PRId32 "] %p->UnixStreamEndpoint::Connect(\"%s\")\n",
		find_thread(NULL), this,
		ConstSocketAddress(&gAddressModule, _address).AsString().Data());

	const sockaddr_un* address = (const sockaddr_un*)_address;

	UnixStreamEndpointLocker endpointLocker(this);

	if (fState == unix_stream_endpoint_state::Connected)
		RETURN_ERROR(EISCONN);

	if (fState != unix_stream_endpoint_state::NotConnected)
		RETURN_ERROR(B_BAD_VALUE);
// TODO: If listening, we could set the backlog to 0 and connect.

	// check the address first
	UnixAddress unixAddress;

	if (address->sun_path[0] == '\0') {
		// internal address space (or empty address)
		int32 internalID;
		if (UnixAddress::IsEmptyAddress(*address))
			RETURN_ERROR(B_BAD_VALUE);

		internalID = UnixAddress::InternalID(*address);
		if (internalID < 0)
			RETURN_ERROR(internalID);

		unixAddress.SetTo(internalID);
	} else {
		// FS address space
		size_t pathLen = strnlen(address->sun_path, sizeof(address->sun_path));
		if (pathLen == 0 || pathLen == sizeof(address->sun_path))
			RETURN_ERROR(B_BAD_VALUE);

		struct stat st;
		status_t error = vfs_read_stat(-1, address->sun_path, true, &st,
			!gStackModule->is_syscall());
		if (error != B_OK)
			RETURN_ERROR(error);

		if (!S_ISSOCK(st.st_mode))
			RETURN_ERROR(B_BAD_VALUE);

		unixAddress.SetTo(st.st_dev, st.st_ino, NULL);
	}

	// get the peer endpoint
	UnixAddressManagerLocker addressLocker(gAddressManager);
	UnixEndpoint* listeningUnixEndpoint = gAddressManager.Lookup(unixAddress);
	if (listeningUnixEndpoint == NULL)
		RETURN_ERROR(ECONNREFUSED);
	UnixStreamEndpoint* listeningEndpoint
		= dynamic_cast<UnixStreamEndpoint*>(listeningUnixEndpoint);
	if (listeningEndpoint == NULL)
		RETURN_ERROR(EPROTOTYPE);
	BReference<UnixStreamEndpoint> peerReference(listeningEndpoint);
	addressLocker.Unlock();

	UnixStreamEndpointLocker peerLocker(listeningEndpoint);

	if (!listeningEndpoint->IsBound()
		|| listeningEndpoint->fState != unix_stream_endpoint_state::Listening
		|| listeningEndpoint->fAddress != unixAddress) {
		RETURN_ERROR(ECONNREFUSED);
	}

	// Allocate FIFOs for us and the socket we're going to spawn. We do that
	// now, so that the mess we need to cleanup, if allocating them fails, is
	// harmless.
	UnixFifo* fifo = new(nothrow) UnixFifo(UNIX_MAX_TRANSFER_UNIT, UnixFifoType::Stream);
	UnixFifo* peerFifo = new(nothrow) UnixFifo(UNIX_MAX_TRANSFER_UNIT, UnixFifoType::Stream);
	ObjectDeleter<UnixFifo> fifoDeleter(fifo);
	ObjectDeleter<UnixFifo> peerFifoDeleter(peerFifo);

	status_t error;
	if ((error = fifo->Init()) != B_OK || (error = peerFifo->Init()) != B_OK)
		return error;

	// spawn new endpoint for accept()
	net_socket* newSocket;
	error = gSocketModule->spawn_pending_socket(listeningEndpoint->socket,
		&newSocket);
	if (error != B_OK)
		RETURN_ERROR(error);

	// init connected peer endpoint
	UnixStreamEndpoint* connectedEndpoint = (UnixStreamEndpoint*)newSocket->first_protocol;

	UnixStreamEndpointLocker connectedLocker(connectedEndpoint);

	connectedEndpoint->_Spawn(this, listeningEndpoint, peerFifo);

	// update our attributes
	_UnsetReceiveFifo();

	fPeerEndpoint = connectedEndpoint;
	PeerAddress().SetTo(&connectedEndpoint->socket->address);
	fPeerEndpoint->AcquireReference();
	fReceiveFifo = fifo;

	fCredentials.pid = getpid();
	fCredentials.uid = geteuid();
	fCredentials.gid = getegid();

	fifoDeleter.Detach();
	peerFifoDeleter.Detach();

	fState = unix_stream_endpoint_state::Connected;
	fWasConnected = true;

	gSocketModule->set_connected(Socket());

	release_sem(listeningEndpoint->fAcceptSemaphore);

	connectedLocker.Unlock();
	peerLocker.Unlock();
	endpointLocker.Unlock();

	RETURN_ERROR(B_OK);
}


status_t
UnixStreamEndpoint::Accept(net_socket** _acceptedSocket)
{
	TRACE("[%" B_PRId32 "] %p->UnixStreamEndpoint::Accept()\n", find_thread(NULL),
		this);

	bigtime_t timeout = absolute_timeout(socket->receive.timeout);
	if (gStackModule->is_restarted_syscall())
		timeout = gStackModule->restore_syscall_restart_timeout();
	else
		gStackModule->store_syscall_restart_timeout(timeout);

	UnixStreamEndpointLocker locker(this);

	status_t error;
	do {
		locker.Unlock();

		error = acquire_sem_etc(fAcceptSemaphore, 1,
			B_ABSOLUTE_TIMEOUT | B_CAN_INTERRUPT, timeout);
		if (error < B_OK)
			break;

		locker.Lock();
		error = gSocketModule->dequeue_connected(socket, _acceptedSocket);
	} while (error != B_OK);

	if (error == B_TIMED_OUT && timeout == 0) {
		// translate non-blocking timeouts to the correct error code
		error = B_WOULD_BLOCK;
	}

	RETURN_ERROR(error);
}


ssize_t
UnixStreamEndpoint::Send(const iovec* vecs, size_t vecCount,
	ancillary_data_container* ancillaryData,
	const struct sockaddr* address, socklen_t addressLength, int flags)
{
	TRACE("[%" B_PRId32 "] %p->UnixStreamEndpoint::Send(%p, %ld, %p)\n",
		find_thread(NULL), this, vecs, vecCount, ancillaryData);

	bigtime_t timeout = 0;
	if ((flags & MSG_DONTWAIT) == 0) {
		timeout = absolute_timeout(socket->send.timeout);
		if (gStackModule->is_restarted_syscall())
			timeout = gStackModule->restore_syscall_restart_timeout();
		else
			gStackModule->store_syscall_restart_timeout(timeout);
	}

	UnixStreamEndpointLocker locker(this);

	BReference<UnixStreamEndpoint> peerReference;
	UnixStreamEndpointLocker peerLocker;

	status_t error = _LockConnectedEndpoints(locker, peerLocker);
	if (error != B_OK)
		RETURN_ERROR(error);

	UnixStreamEndpoint* peerEndpoint = fPeerEndpoint;
	peerReference.SetTo(peerEndpoint);

	// lock the peer's FIFO
	UnixFifo* peerFifo = peerEndpoint->fReceiveFifo;
	BReference<UnixFifo> _(peerFifo);
	UnixFifoLocker fifoLocker(peerFifo);

	// unlock endpoints
	locker.Unlock();
	peerLocker.Unlock();

	ssize_t result = peerFifo->Write(vecs, vecCount, ancillaryData, NULL, timeout);

	// Notify select()ing readers, if we successfully wrote anything.
	size_t readable = peerFifo->Readable();
	bool notifyRead = (error == B_OK && readable > 0
		&& !peerFifo->IsReadShutdown());

	// Notify select()ing writers, if we failed to write anything and there's
	// still room to write.
	size_t writable = peerFifo->Writable();
	bool notifyWrite = (error != B_OK && writable > 0
		&& !peerFifo->IsWriteShutdown());

	// re-lock our endpoint (unlock FIFO to respect locking order)
	fifoLocker.Unlock();
	locker.Lock();

	bool peerLocked = (fPeerEndpoint == peerEndpoint
		&& _LockConnectedEndpoints(locker, peerLocker) == B_OK);

	// send notifications
	if (peerLocked && notifyRead)
		gSocketModule->notify(peerEndpoint->socket, B_SELECT_READ, readable);
	if (notifyWrite)
		gSocketModule->notify(socket, B_SELECT_WRITE, writable);

	switch (result) {
		case UNIX_FIFO_SHUTDOWN:
			if (fPeerEndpoint == peerEndpoint
					&& fState == unix_stream_endpoint_state::Connected) {
				// Orderly write shutdown on our side.
				// Note: Linux and Solaris also send a SIGPIPE, but according
				// the send() specification that shouldn't be done.
				result = EPIPE;
			} else {
				// The FD has been closed.
				result = EBADF;
			}
			break;
		case EPIPE:
			// The peer closed connection or shutdown its read side. Reward
			// the caller with a SIGPIPE.
			if (gStackModule->is_syscall())
				send_signal(find_thread(NULL), SIGPIPE);
			break;
		case B_TIMED_OUT:
			// Translate non-blocking timeouts to the correct error code.
			if (timeout == 0)
				result = B_WOULD_BLOCK;
			break;
	}

	RETURN_ERROR(result);
}


ssize_t
UnixStreamEndpoint::Receive(const iovec* vecs, size_t vecCount,
	ancillary_data_container** _ancillaryData, struct sockaddr* _address,
	socklen_t* _addressLength, int flags)
{
	TRACE("[%" B_PRId32 "] %p->UnixStreamEndpoint::Receive(%p, %ld)\n",
		find_thread(NULL), this, vecs, vecCount);

	bigtime_t timeout = 0;
	if ((flags & MSG_DONTWAIT) == 0) {
		timeout = absolute_timeout(socket->receive.timeout);
		if (gStackModule->is_restarted_syscall())
			timeout = gStackModule->restore_syscall_restart_timeout();
		else
			gStackModule->store_syscall_restart_timeout(timeout);
	}

	UnixStreamEndpointLocker locker(this);

	// We can read as long as we have a FIFO. I.e. we are still connected, or
	// disconnected and not yet reconnected/listening/closed.
	if (fReceiveFifo == NULL)
		RETURN_ERROR(ENOTCONN);

	UnixStreamEndpoint* peerEndpoint = fPeerEndpoint;
	BReference<UnixStreamEndpoint> peerReference(peerEndpoint);

	// Copy the peer address upfront. This way, if we read something, we don't
	// get into a potential race with Close().
	if (_address != NULL) {
		socklen_t addrLen = min_c(*_addressLength, socket->peer.ss_len);
		memcpy(_address, &socket->peer, addrLen);
		*_addressLength = addrLen;
	}

	// lock our FIFO
	UnixFifo* fifo = fReceiveFifo;
	BReference<UnixFifo> _(fifo);
	UnixFifoLocker fifoLocker(fifo);

	// unlock endpoint
	locker.Unlock();

	ssize_t result = fifo->Read(vecs, vecCount, _ancillaryData, NULL, timeout);

	// Notify select()ing writers, if we successfully read anything.
	size_t writable = fifo->Writable();
	bool notifyWrite = (result >= 0 && writable > 0
		&& !fifo->IsWriteShutdown());

	// Notify select()ing readers, if we failed to read anything and there's
	// still something left to read.
	size_t readable = fifo->Readable();
	bool notifyRead = (result < 0 && readable > 0
		&& !fifo->IsReadShutdown());

	// re-lock our endpoint (unlock FIFO to respect locking order)
	fifoLocker.Unlock();
	locker.Lock();

	UnixStreamEndpointLocker peerLocker;
	bool peerLocked = (peerEndpoint != NULL && fPeerEndpoint == peerEndpoint
		&& _LockConnectedEndpoints(locker, peerLocker) == B_OK);

	// send notifications
	if (notifyRead)
		gSocketModule->notify(socket, B_SELECT_READ, readable);
	if (peerLocked && notifyWrite)
		gSocketModule->notify(peerEndpoint->socket, B_SELECT_WRITE, writable);

	switch (result) {
		case UNIX_FIFO_SHUTDOWN:
			// Either our socket was closed or read shutdown.
			if (fState == unix_stream_endpoint_state::Closed) {
				// The FD has been closed.
				result = EBADF;
			} else {
				// if (fReceiveFifo == fifo) {
				// 		Orderly shutdown or the peer closed the connection.
				// } else {
				//		Weird case: Peer closed connection and we are already
				// 		reconnected (or listening).
				// }
				result = 0;
			}
			break;
		case B_TIMED_OUT:
			// translate non-blocking timeouts to the correct error code
			if (timeout == 0)
				result = B_WOULD_BLOCK;
			break;
	}

	RETURN_ERROR(result);
}


ssize_t
UnixStreamEndpoint::Sendable()
{
	TRACE("[%" B_PRId32 "] %p->UnixStreamEndpoint::Sendable()\n", find_thread(NULL),
		this);

	UnixStreamEndpointLocker locker(this);
	UnixStreamEndpointLocker peerLocker;

	status_t error = _LockConnectedEndpoints(locker, peerLocker);
	if (error != B_OK)
		RETURN_ERROR(error);

	// lock the peer's FIFO
	UnixFifo* peerFifo = fPeerEndpoint->fReceiveFifo;
	UnixFifoLocker fifoLocker(peerFifo);

	RETURN_ERROR(peerFifo->Writable());
}


ssize_t
UnixStreamEndpoint::Receivable()
{
	TRACE("[%" B_PRId32 "] %p->UnixStreamEndpoint::Receivable()\n", find_thread(NULL),
		this);

	UnixStreamEndpointLocker locker(this);

	if (fState == unix_stream_endpoint_state::Listening)
		return gSocketModule->count_connected(socket);

	if (fState != unix_stream_endpoint_state::Connected)
		RETURN_ERROR(ENOTCONN);

	UnixFifoLocker fifoLocker(fReceiveFifo);
	ssize_t readable = fReceiveFifo->Readable();
	if (readable == 0 && (fReceiveFifo->IsWriteShutdown()
			|| fReceiveFifo->IsReadShutdown())) {
		RETURN_ERROR(ENOTCONN);
	}
	RETURN_ERROR(readable);
}


status_t
UnixStreamEndpoint::SetReceiveBufferSize(size_t size)
{
	TRACE("[%" B_PRId32 "] %p->UnixStreamEndpoint::SetReceiveBufferSize(%lu)\n",
		find_thread(NULL), this, size);

	UnixStreamEndpointLocker locker(this);

	if (fReceiveFifo == NULL)
		return B_BAD_VALUE;

	UnixFifoLocker fifoLocker(fReceiveFifo);
	return fReceiveFifo->SetBufferCapacity(size);
}


status_t
UnixStreamEndpoint::GetPeerCredentials(ucred* credentials)
{
	UnixStreamEndpointLocker locker(this);
	UnixStreamEndpointLocker peerLocker;

	status_t error = _LockConnectedEndpoints(locker, peerLocker);
	if (error != B_OK)
		RETURN_ERROR(error);

	*credentials = fPeerEndpoint->fCredentials;

	return B_OK;
}


status_t
UnixStreamEndpoint::Shutdown(int direction)
{
	TRACE("[%" B_PRId32 "] %p->UnixStreamEndpoint::Shutdown(%d)\n",
		find_thread(NULL), this, direction);

	uint32 shutdown;
	uint32 peerShutdown;

	// translate the direction into shutdown flags for our and the peer fifo
	switch (direction) {
		case SHUT_RD:
			shutdown = UNIX_FIFO_SHUTDOWN_READ;
			peerShutdown = 0;
			break;
		case SHUT_WR:
			shutdown = 0;
			peerShutdown = UNIX_FIFO_SHUTDOWN_WRITE;
			break;
		case SHUT_RDWR:
			shutdown = UNIX_FIFO_SHUTDOWN_READ;
			peerShutdown = UNIX_FIFO_SHUTDOWN_WRITE;
			break;
		default:
			RETURN_ERROR(B_BAD_VALUE);
	}

	// lock endpoints
	UnixStreamEndpointLocker locker(this);
	UnixStreamEndpointLocker peerLocker;

	status_t error = _LockConnectedEndpoints(locker, peerLocker);
	if (error != B_OK)
		RETURN_ERROR(error);

	// shutdown our FIFO
	fReceiveFifo->Lock();
	fReceiveFifo->Shutdown(shutdown);
	fReceiveFifo->Unlock();

	// shutdown peer FIFO
	fPeerEndpoint->fReceiveFifo->Lock();
	fPeerEndpoint->fReceiveFifo->Shutdown(peerShutdown);
	fPeerEndpoint->fReceiveFifo->Unlock();

	// send select notifications
	if (direction == SHUT_RD || direction == SHUT_RDWR) {
		gSocketModule->notify(socket, B_SELECT_READ, EPIPE);
		gSocketModule->notify(fPeerEndpoint->socket, B_SELECT_WRITE, EPIPE);
	}
	if (direction == SHUT_WR || direction == SHUT_RDWR) {
		gSocketModule->notify(socket, B_SELECT_WRITE, EPIPE);
		gSocketModule->notify(fPeerEndpoint->socket, B_SELECT_READ, EPIPE);
	}

	RETURN_ERROR(B_OK);
}


void
UnixStreamEndpoint::_Spawn(UnixStreamEndpoint* connectingEndpoint,
	UnixStreamEndpoint* listeningEndpoint, UnixFifo* fifo)
{
	ProtocolSocket::Open();

	fIsChild = true;
	fPeerEndpoint = connectingEndpoint;
	fPeerEndpoint->AcquireReference();

	fReceiveFifo = fifo;

	PeerAddress().SetTo(&connectingEndpoint->socket->address);

	fCredentials = listeningEndpoint->fCredentials;

	fState = unix_stream_endpoint_state::Connected;

	gSocketModule->set_connected(Socket());
}


void
UnixStreamEndpoint::_Disconnect()
{
	// Both endpoints must be locked.

	// Write shutdown the receive FIFO.
	fReceiveFifo->Lock();
	fReceiveFifo->Shutdown(UNIX_FIFO_SHUTDOWN_WRITE);
	fReceiveFifo->Unlock();

	// select() notification.
	gSocketModule->notify(socket, B_SELECT_READ, ECONNRESET);
	gSocketModule->notify(socket, B_SELECT_WRITE, ECONNRESET);

	// Unset the peer endpoint.
	fPeerEndpoint->ReleaseReference();
	fPeerEndpoint = NULL;

	// We're officially disconnected.
// TODO: Deal with non accept()ed connections correctly!
	fIsChild = false;
	fState = unix_stream_endpoint_state::NotConnected;
}


status_t
UnixStreamEndpoint::_LockConnectedEndpoints(UnixStreamEndpointLocker& locker,
	UnixStreamEndpointLocker& peerLocker)
{
	if (fState != unix_stream_endpoint_state::Connected)
		RETURN_ERROR(fWasConnected ? EPIPE : ENOTCONN);

	// We need to lock the peer, too. Get a reference -- we might need to
	// unlock ourselves to get the locking order right.
	BReference<UnixStreamEndpoint> peerReference(fPeerEndpoint);
	UnixStreamEndpoint* peerEndpoint = fPeerEndpoint;

	if (fIsChild) {
		// We're the child, but locking order is the other way around.
		locker.Unlock();
		peerLocker.SetTo(peerEndpoint, false);

		locker.Lock();

		// recheck our state, also whether the peer is still the same
		if (fState != unix_stream_endpoint_state::Connected || peerEndpoint != fPeerEndpoint)
			RETURN_ERROR(ENOTCONN);
	} else
		peerLocker.SetTo(peerEndpoint, false);

	RETURN_ERROR(B_OK);
}


status_t
UnixStreamEndpoint::_Unbind()
{
	if (fState == unix_stream_endpoint_state::Connected
		|| fState == unix_stream_endpoint_state::Listening)
		RETURN_ERROR(B_BAD_VALUE);

	if (IsBound())
		RETURN_ERROR(UnixEndpoint::_Unbind());

	RETURN_ERROR(B_OK);
}


void
UnixStreamEndpoint::_UnsetReceiveFifo()
{
	if (fReceiveFifo) {
		fReceiveFifo->ReleaseReference();
		fReceiveFifo = NULL;
	}
}


void
UnixStreamEndpoint::_StopListening()
{
	if (fState == unix_stream_endpoint_state::Listening) {
		delete_sem(fAcceptSemaphore);
		fAcceptSemaphore = -1;
		fState = unix_stream_endpoint_state::NotConnected;
	}
}
