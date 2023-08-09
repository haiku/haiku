/*
 * Copyright 2023, Trung Nguyen, trungnt282910@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include "UnixDatagramEndpoint.h"

#include <new>

#include "unix.h"
#include "UnixAddressManager.h"
#include "UnixFifo.h"


#define UNIX_DATAGRAM_ENDPOINT_DEBUG_LEVEL	0
#define UNIX_DEBUG_LEVEL					UNIX_DATAGRAM_ENDPOINT_DEBUG_LEVEL
#include "UnixDebug.h"


typedef AutoLocker<UnixDatagramEndpoint> UnixDatagramEndpointLocker;


UnixDatagramEndpoint::UnixDatagramEndpoint(net_socket* socket)
	:
	UnixEndpoint(socket),
	fTargetEndpoint(NULL),
	fReceiveFifo(NULL),
	fShutdownWrite(false)
{
	TRACE("[%" B_PRId32 "] %p->UnixDatagramEndpoint::UnixDatagramEndpoint()\n",
		find_thread(NULL), this);
}


UnixDatagramEndpoint::~UnixDatagramEndpoint()
{
	TRACE("[%" B_PRId32 "] %p->UnixDatagramEndpoint::~UnixDatagramEndpoint()\n",
		find_thread(NULL), this);
}


status_t
UnixDatagramEndpoint::Init()
{
	TRACE("[%" B_PRId32 "] %p->UnixDatagramEndpoint::Init()\n",
		find_thread(NULL), this);

	RETURN_ERROR(B_OK);
}


void
UnixDatagramEndpoint::Uninit()
{
	TRACE("[%" B_PRId32 "] %p->UnixDatagramEndpoint::Uninit()\n",
		find_thread(NULL), this);

	ReleaseReference();
}


status_t
UnixDatagramEndpoint::Open()
{
	TRACE("[%" B_PRId32 "] %p->UnixDatagramEndpoint::Open()\n",
		find_thread(NULL), this);

	status_t error = ProtocolSocket::Open();
	if (error != B_OK)
		RETURN_ERROR(error);

	RETURN_ERROR(B_OK);
}


status_t
UnixDatagramEndpoint::Close()
{
	TRACE("[%" B_PRId32 "] %p->UnixDatagramEndpoint::Close()\n",
		find_thread(NULL), this);

	UnixDatagramEndpointLocker endpointLocker(this);

	fShutdownRead = fShutdownWrite = true;

	if (IsBound()) {
		status_t status = UnixEndpoint::_Unbind();
		if (status != B_OK)
			RETURN_ERROR(status);
	}

	_UnsetReceiveFifo();

	RETURN_ERROR(_Disconnect());
}


status_t
UnixDatagramEndpoint::Free()
{
	TRACE("[%" B_PRId32 "] %p->UnixDatagramEndpoint::Free()\n",
		find_thread(NULL), this);

	UnixDatagramEndpointLocker endpointLocker(this);

	ASSERT(fReceiveFifo == NULL);
	ASSERT(fTargetEndpoint == NULL);

	return B_OK;
}


status_t
UnixDatagramEndpoint::Bind(const struct sockaddr* _address)
{
	TRACE("[%" B_PRId32 "] %p->UnixDatagramEndpoint::Bind(\"%s\")\n",
		find_thread(NULL), this,
		ConstSocketAddress(&gAddressModule, _address).AsString().Data());

	if (_address->sa_family != AF_UNIX)
		RETURN_ERROR(EAFNOSUPPORT);

	UnixDatagramEndpointLocker endpointLocker(this);

	if (IsBound())
		RETURN_ERROR(B_BAD_VALUE);

	const sockaddr_un* address = (const sockaddr_un*)_address;

	RETURN_ERROR(_Bind(address));
}


status_t
UnixDatagramEndpoint::Unbind()
{
	TRACE("[%" B_PRId32 "] %p->UnixDatagramEndpoint::Unbind()\n",
		find_thread(NULL), this);

	UnixDatagramEndpointLocker endpointLocker(this);

	if (IsBound())
		RETURN_ERROR(UnixEndpoint::_Unbind());

	RETURN_ERROR(B_OK);
}


status_t
UnixDatagramEndpoint::Listen(int backlog)
{
	TRACE("[%" B_PRId32 "] %p->UnixDatagramEndpoint::Listen(%d)\n", find_thread(NULL),
		this, backlog);

	RETURN_ERROR(EOPNOTSUPP);
}


status_t
UnixDatagramEndpoint::Connect(const struct sockaddr* _address)
{
	TRACE("[%" B_PRId32 "] %p->UnixDatagramEndpoint::Connect(\"%s\")\n",
		find_thread(NULL), this,
		ConstSocketAddress(&gAddressModule, _address).AsString().Data());

	UnixDatagramEndpointLocker endpointLocker(this);

	BReference<UnixDatagramEndpoint> targetEndpointReference;
	status_t status = _InitializeEndpoint(_address, targetEndpointReference);
	if (status != B_OK)
		RETURN_ERROR(status);

	endpointLocker.Unlock();

	UnixDatagramEndpoint* targetEndpoint = targetEndpointReference.Get();
	UnixDatagramEndpointLocker targetLocker(targetEndpoint);

	if (targetEndpoint->fTargetEndpoint != NULL && targetEndpoint->fTargetEndpoint != this)
		RETURN_ERROR(EPERM);

	targetLocker.Unlock();
	endpointLocker.Lock();

	status = _Disconnect();
	if (status != B_OK)
		RETURN_ERROR(status);

	fTargetEndpoint = targetEndpoint;
	fTargetEndpoint->AcquireReference();

	// Required by the socket layer.
	PeerAddress().SetTo(&fTargetEndpoint->socket->address);
	gSocketModule->set_connected(Socket());

	RETURN_ERROR(B_OK);
}


status_t
UnixDatagramEndpoint::Accept(net_socket** _acceptedSocket)
{
	TRACE("[%" B_PRId32 "] %p->UnixDatagramEndpoint::Accept()\n",
		find_thread(NULL), this);

	RETURN_ERROR(EOPNOTSUPP);
}


ssize_t
UnixDatagramEndpoint::Send(const iovec* vecs, size_t vecCount,
	ancillary_data_container* ancillaryData, const struct sockaddr* address,
	socklen_t addressLength, int flags)
{
	TRACE("[%" B_PRId32 "] %p->UnixDatagramEndpoint::Send()\n",
		find_thread(NULL), this);

	bigtime_t timeout = 0;
	if ((flags & MSG_DONTWAIT) == 0) {
		timeout = absolute_timeout(socket->send.timeout);
		if (gStackModule->is_restarted_syscall())
			timeout = gStackModule->restore_syscall_restart_timeout();
		else
			gStackModule->store_syscall_restart_timeout(timeout);
	}
	UnixDatagramEndpointLocker endpointLocker(this);

	if (fShutdownWrite)
		RETURN_ERROR(EPIPE);

	status_t status;

	BReference<UnixDatagramEndpoint> targetEndpointReference;
	if (address == NULL) {
		if (fTargetEndpoint == NULL)
			RETURN_ERROR(ENOTCONN);

		targetEndpointReference.SetTo(fTargetEndpoint);
	} else {
		status = _InitializeEndpoint(address, targetEndpointReference);
		if (status != B_OK)
			RETURN_ERROR(status);
	}

	// Get the address before unlocking the sending endpoint.
	struct sockaddr_storage sourceAddress;
	memcpy(&sourceAddress, &socket->address, sizeof(struct sockaddr_storage));
	endpointLocker.Unlock();

	UnixDatagramEndpoint* targetEndpoint = targetEndpointReference.Get();
	UnixDatagramEndpointLocker targetLocker(targetEndpoint);

	if (targetEndpoint->fTargetEndpoint != NULL && targetEndpoint->fTargetEndpoint != this)
		RETURN_ERROR(EPERM);

	if (targetEndpoint->fShutdownRead)
		RETURN_ERROR(EPIPE);

	if (targetEndpoint->fReceiveFifo == NULL) {
		targetEndpoint->fReceiveFifo
			= new (std::nothrow) UnixFifo(UNIX_MAX_TRANSFER_UNIT, UnixFifoType::Datagram);
		if (targetEndpoint->fReceiveFifo == NULL)
			RETURN_ERROR(B_NO_MEMORY);

		status = targetEndpoint->fReceiveFifo->Init();
		if (status != B_OK) {
			targetEndpoint->_UnsetReceiveFifo();
			RETURN_ERROR(status);
		}
	}

	UnixFifo* targetFifo = targetEndpoint->fReceiveFifo;
	BReference<UnixFifo> targetFifoReference(targetFifo);
	UnixFifoLocker fifoLocker(targetFifo);

	targetLocker.Unlock();

	ssize_t result = targetFifo->Write(vecs, vecCount, ancillaryData, &sourceAddress,
		timeout);

	// Notify select()ing readers, if we successfully wrote anything.
	size_t readable = targetFifo->Readable();
	bool notifyRead = (readable > 0 && result >= 0);

	// Notify select()ing writers, if we failed to write anything and there's
	// still room to write.
	size_t writable = targetFifo->Writable();
	bool notifyWrite = (writable > 0 && result < 0);

	fifoLocker.Unlock();
	targetLocker.Lock();

	// We must recheck fShutdownRead after reacquiring the lock, as it may have changed.
	if (notifyRead && !targetEndpoint->fShutdownRead)
		gSocketModule->notify(targetEndpoint->socket, B_SELECT_READ, readable);

	targetLocker.Unlock();

	if (notifyWrite) {
		endpointLocker.Lock();
		if (!fShutdownWrite)
			gSocketModule->notify(socket, B_SELECT_WRITE, writable);
	}

	switch (result) {
		case EPIPE:
			if (gStackModule->is_syscall())
				send_signal(find_thread(NULL), SIGPIPE);
			break;
		case B_TIMED_OUT:
			if (timeout == 0)
				result = B_WOULD_BLOCK;
			break;
	}

	RETURN_ERROR(result);
}


ssize_t
UnixDatagramEndpoint::Receive(const iovec* vecs, size_t vecCount,
	ancillary_data_container** _ancillaryData, struct sockaddr* _address,
	socklen_t* _addressLength, int flags)
{
	TRACE("[%" B_PRId32 "] %p->UnixDatagramEndpoint::Receive()\n",
		find_thread(NULL), this);

	bigtime_t timeout = 0;
	if ((flags & MSG_DONTWAIT) == 0) {
		timeout = absolute_timeout(socket->receive.timeout);
		if (gStackModule->is_restarted_syscall())
			timeout = gStackModule->restore_syscall_restart_timeout();
		else
			gStackModule->store_syscall_restart_timeout(timeout);
	}

	UnixDatagramEndpointLocker endpointLocker(this);

	// It is not clearly specified in POSIX how to treat pending
	// datagrams when a socket has been shut down for reading.
	// On Linux, pending messages are still read.
	if (fShutdownRead)
		RETURN_ERROR(0);

	status_t status;

	if (fReceiveFifo == NULL) {
		fReceiveFifo = new (std::nothrow) UnixFifo(UNIX_MAX_TRANSFER_UNIT,
			UnixFifoType::Datagram);
		if (fReceiveFifo == NULL)
			RETURN_ERROR(B_NO_MEMORY);

		status = fReceiveFifo->Init();
		if (status != B_OK) {
			_UnsetReceiveFifo();
			RETURN_ERROR(status);
		}
	}

	UnixFifo* fifo = fReceiveFifo;
	BReference<UnixFifo> fifoReference(fifo);
	UnixFifoLocker fifoLocker(fifo);

	endpointLocker.Unlock();

	struct sockaddr_storage addressStorage;

	ssize_t result = fifo->Read(vecs, vecCount, _ancillaryData, &addressStorage, timeout);

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
	endpointLocker.Lock();

	// send notifications
	if (notifyRead)
		gSocketModule->notify(socket, B_SELECT_READ, readable);

	if (notifyWrite) {
		BReference<UnixDatagramEndpoint> originEndpointReference;
		status = _InitializeEndpoint((struct sockaddr*)&addressStorage,
			originEndpointReference);
		if (status == B_OK) {
			UnixDatagramEndpoint* originEndpoint = originEndpointReference.Get();
			endpointLocker.Unlock();
			UnixDatagramEndpointLocker originLocker(originEndpoint);
			gSocketModule->notify(originEndpoint->socket, B_SELECT_WRITE, writable);
			originLocker.Unlock();
			endpointLocker.Lock();
		}
	}

	if (result < 0) {
		switch (result) {
			case B_TIMED_OUT:
				if (timeout == 0)
					result = B_WOULD_BLOCK;
				break;
		}
	} else {
		if (_address != NULL) {
			if (_addressLength == NULL)
				RETURN_ERROR(B_BAD_ADDRESS);
			struct sockaddr_un* address = (struct sockaddr_un*)&addressStorage;
			socklen_t memoryLength = min_c(*_addressLength, address->sun_len);
			memcpy(_address, address, memoryLength);
			*_addressLength = address->sun_len;
		}
	}

	RETURN_ERROR(result);
}


ssize_t
UnixDatagramEndpoint::Sendable()
{
	TRACE("[%" B_PRId32 "] %p->UnixDatagramEndpoint::Sendable()\n",
		find_thread(NULL), this);

	RETURN_ERROR(EOPNOTSUPP);
}


ssize_t
UnixDatagramEndpoint::Receivable()
{
	TRACE("[%" B_PRId32 "] %p->UnixDatagramEndpoint::Receivable()\n",
		find_thread(NULL), this);

	UnixDatagramEndpointLocker locker(this);

	if (fReceiveFifo == NULL)
		RETURN_ERROR(0);

	UnixFifoLocker fifoLocker(fReceiveFifo);
	ssize_t readable = fReceiveFifo->Readable();

	RETURN_ERROR(readable);
}


status_t
UnixDatagramEndpoint::SetReceiveBufferSize(size_t size)
{
	TRACE("[%" B_PRId32 "] %p->UnixDatagramEndpoint::SetReceiveBufferSize()\n",
		find_thread(NULL), this);

	UnixDatagramEndpointLocker locker(this);

	if (fReceiveFifo == NULL)
		RETURN_ERROR(0);

	UnixFifoLocker fifoLocker(fReceiveFifo);
	RETURN_ERROR(fReceiveFifo->SetBufferCapacity(size));
}


status_t
UnixDatagramEndpoint::GetPeerCredentials(ucred* credentials)
{
	TRACE("[%" B_PRId32 "] %p->UnixDatagramEndpoint::GetPeerCredentials()\n",
		find_thread(NULL), this);

	RETURN_ERROR(EOPNOTSUPP);
}


status_t
UnixDatagramEndpoint::Shutdown(int direction)
{
	TRACE("[%" B_PRId32 "] %p->UnixDatagramEndpoint::Shutdown()\n",
		find_thread(NULL), this);

	UnixDatagramEndpointLocker endpointLocker(this);

	if (direction != SHUT_RD && direction != SHUT_WR && direction != SHUT_RDWR)
		RETURN_ERROR(B_BAD_VALUE);

	if (direction != SHUT_RD)
		fShutdownWrite = true;

	if (direction != SHUT_WR)
		fShutdownRead = true;

	RETURN_ERROR(B_OK);
}


status_t
UnixDatagramEndpoint::_InitializeEndpoint(const struct sockaddr* _address,
	BReference<UnixDatagramEndpoint>& outEndpoint)
{
	if (_address->sa_family != AF_UNIX)
		RETURN_ERROR(EAFNOSUPPORT);

	UnixAddress unixAddress;

	const struct sockaddr_un* address = (const struct sockaddr_un*)_address;

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

	UnixAddressManagerLocker addressLocker(gAddressManager);
	UnixEndpoint* targetUnixEndpoint = gAddressManager.Lookup(unixAddress);
	if (targetUnixEndpoint == NULL)
		RETURN_ERROR(ECONNREFUSED);
	UnixDatagramEndpoint* targetEndpoint
		= dynamic_cast<UnixDatagramEndpoint*>(targetUnixEndpoint);
	if (targetEndpoint == NULL)
		RETURN_ERROR(EPROTOTYPE);

	outEndpoint.SetTo(targetEndpoint);
	addressLocker.Unlock();

	RETURN_ERROR(B_OK);
}


status_t
UnixDatagramEndpoint::_Disconnect()
{
	if (fTargetEndpoint != NULL)
		fTargetEndpoint->ReleaseReference();

	fTargetEndpoint = NULL;
	RETURN_ERROR(B_OK);
}


void
UnixDatagramEndpoint::_UnsetReceiveFifo()
{
	if (fReceiveFifo != NULL) {
		fReceiveFifo->ReleaseReference();
		fReceiveFifo = NULL;
	}
}
