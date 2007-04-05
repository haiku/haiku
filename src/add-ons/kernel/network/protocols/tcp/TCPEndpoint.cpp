/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Galante, haiku.galante@gmail.com
 *		Axel Dörfler, axeld@pinc-software.de
 *		Hugo Santos, hugosantos@gmail.com
 */


#include "TCPEndpoint.h"
#include "EndpointManager.h"

#include <net_buffer.h>
#include <net_datalink.h>
#include <NetBufferUtilities.h>
#include <NetUtilities.h>

#include <lock.h>
#include <util/AutoLock.h>
#include <util/khash.h>
#include <util/list.h>

#include <KernelExport.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <new>
#include <stdlib.h>
#include <string.h>


// Things this implementation currently doesn't implement:
// TCP, RFC 793
// TCP Slow Start, Congestion Avoidance, Fast Retransmit, and Fast Recovery, RFC 2001, RFC 2581, RFC 3042
// NewReno Modification to TCP's Fast Recovery, RFC 2582
// Explicit Congestion Notification (ECN), RFC 3168
// SYN-Cache
// TCP Extensions for High Performance, RFC 1323
// SACK, Selective Acknowledgment - RFC 2018, RFC 2883, RFC 3517
// Forward RTO-Recovery, RFC 4138

//#define TRACE_TCP

#ifdef TRACE_TCP
// the space after 'this' is important in order for this to work with cpp 2.95
#	define TRACE(format, args...)	dprintf("TCP:%p:" format "\n", this , ##args)
#else
#	define TRACE(args...)
#endif

// Initial estimate for packet round trip time (RTT)
#define	TCP_INITIAL_RTT	120000000LL

// constants for the fFlags field
enum {
	FLAG_OPTION_WINDOW_SHIFT	= 0x01,
	FLAG_OPTION_TIMESTAMP		= 0x02,
	FLAG_NO_RECEIVE				= 0x04,
	FLAG_NO_SEND				= 0x08,
};


TCPEndpoint::TCPEndpoint(net_socket *socket)
	:
	fOptions(0),
	fSendWindowShift(0),
	fReceiveWindowShift(0),
	fSendUnacknowledged(0),
	fSendNext(fSendUnacknowledged),
	fSendWindow(0),
	fSendMaxWindow(0),
	fSendMaxSegmentSize(TCP_DEFAULT_MAX_SEGMENT_SIZE),
	fSendQueue(socket->send.buffer_size),
	fInitialSendSequence(0),
	fDuplicateAcknowledgeCount(0),
	fRoute(NULL),
	fReceiveNext(0),
	fReceiveMaxAdvertised(0),
	fReceiveWindow(socket->receive.buffer_size),
	fReceiveMaxSegmentSize(TCP_DEFAULT_MAX_SEGMENT_SIZE),
	fReceiveQueue(socket->receive.buffer_size),
	fRoundTripTime(TCP_INITIAL_RTT),
	fState(CLOSED),
	fFlags(0), //FLAG_OPTION_WINDOW_SHIFT),
	fError(B_OK)
{
	//gStackModule->init_timer(&fTimer, _TimeWait, this);

	recursive_lock_init(&fLock, "tcp lock");
		// TODO: to be replaced with a real locking strategy!
	//benaphore_init(&fReceiveLock, "tcp receive");
	//benaphore_init(&fSendLock, "tcp send");
	fSendLock = create_sem(0, "tcp send");
	fReceiveLock = create_sem(0, "tcp receive");

	gStackModule->init_timer(&fPersistTimer, TCPEndpoint::_PersistTimer, this);
	gStackModule->init_timer(&fRetransmitTimer, TCPEndpoint::_RetransmitTimer, this);
	gStackModule->init_timer(&fDelayedAcknowledgeTimer,
		TCPEndpoint::_DelayedAcknowledgeTimer, this);
	gStackModule->init_timer(&fTimeWaitTimer, TCPEndpoint::_TimeWaitTimer, this);
}


TCPEndpoint::~TCPEndpoint()
{
	recursive_lock_lock(&fLock);

	gStackModule->cancel_timer(&fRetransmitTimer);
	gStackModule->cancel_timer(&fPersistTimer);
	gStackModule->cancel_timer(&fDelayedAcknowledgeTimer);
	gStackModule->cancel_timer(&fTimeWaitTimer);

	gEndpointManager->Unbind(this);

	recursive_lock_destroy(&fLock);
	//benaphore_destroy(&fReceiveLock);
	//benaphore_destroy(&fSendLock);
	delete_sem(fReceiveLock);
	delete_sem(fSendLock);
}


status_t
TCPEndpoint::InitCheck() const
{
	if (fLock.sem < B_OK)
		return fLock.sem;
	if (fReceiveLock < B_OK)
		return fReceiveLock;
	if (fSendLock < B_OK)
		return fSendLock;

	return B_OK;
}


//	#pragma mark - protocol API


status_t
TCPEndpoint::Open()
{
	TRACE("Open()");
	// nothing to do here...
	return B_OK;
}


status_t
TCPEndpoint::Close()
{
	TRACE("Close()");
	RecursiveLocker lock(fLock);

	if (fState == LISTEN)
		delete_sem(fAcceptSemaphore);

	if (fState == SYNCHRONIZE_SENT || fState == LISTEN) {
		fState = CLOSED;
		return B_OK;
	}

	status_t status = _ShutdownEgress(true);
	if (status != B_OK)
		return status;

	TRACE("Close(): Entering state %d", fState);

	if (socket->options & SO_LINGER) {
		TRACE("Close(): Lingering for %i secs", socket->linger);

		bigtime_t maximum = system_time() + socket->linger * 1000000LL;

		while (fSendQueue.Used() > 0) {
			lock.Unlock();
			status = acquire_sem_etc(fSendLock, 1, B_CAN_INTERRUPT
										| B_ABSOLUTE_TIMEOUT, maximum);
			if (status == B_TIMED_OUT) {
				TRACE("Close(): Lingering made me wait, but not all data was sent.");
				return B_OK;
			} else if (status < B_OK)
				return status;

			lock.Lock();
		}

		TRACE("Close(): Waited for all data to be sent with success");
	}

	// TODO: do i need to wait until fState returns to CLOSED?
	return B_OK;
}


status_t
TCPEndpoint::Free()
{
	TRACE("Free()");

	if (fState <= SYNCHRONIZE_SENT || fState == TIME_WAIT)
		return B_OK;

	_EnterTimeWait();
	return B_BUSY;
		// we'll be freed later when the 2MSL timer expires
}


/*!
	Creates and sends a synchronize packet to /a address, and then waits
	until the connection has been established or refused.
*/
status_t
TCPEndpoint::Connect(const struct sockaddr *address)
{
	TRACE("Connect() on address %s",
		  AddressString(gDomain, address, true).Data());

	if (address->sa_family != AF_INET)
		return EAFNOSUPPORT;

	RecursiveLocker locker(&fLock);

	TRACE("  Connect(): in state %d", fState);

	// Can only call connect() from CLOSED or LISTEN states
	// otherwise endpoint is considered already connected
	if (fState == LISTEN) {
		// this socket is about to connect; remove pending connections in the backlog
		gSocketModule->set_max_backlog(socket, 0);
	} else if (fState != CLOSED)
		return EISCONN;

	// get a net_route if there isn't one
	// TODO: get a net_route_info instead!
	if (fRoute == NULL) {
		fRoute = gDatalinkModule->get_route(gDomain, (sockaddr *)address);
		TRACE("  Connect(): Using Route %p", fRoute);
		if (fRoute == NULL)
			return ENETUNREACH;
	}

	// make sure connection does not already exist
	status_t status = gEndpointManager->SetConnection(this,
		(sockaddr *)&socket->address, address, fRoute->interface->address);
	if (status < B_OK) {
		TRACE("  Connect(): could not add connection: %s!", strerror(status));
		return status;
	}

	fReceiveMaxSegmentSize = _GetMSS(address);

	// Compute the window shift we advertise to our peer - if it doesn't support
	// this option, this will be reset to 0 (when its SYN is received)
	fReceiveWindowShift = 0;
	while (fReceiveWindowShift < TCP_MAX_WINDOW_SHIFT
		&& (0xffffUL << fReceiveWindowShift) < socket->receive.buffer_size) {
		fReceiveWindowShift++;
	}

	TRACE("  Connect(): starting 3-way handshake...");

	fState = SYNCHRONIZE_SENT;
	fInitialSendSequence = system_time() >> 4;
	fSendNext = fInitialSendSequence;
	fSendUnacknowledged = fInitialSendSequence;
	fSendMax = fInitialSendSequence;
	fSendQueue.SetInitialSequence(fSendNext + 1);

	// send SYN
	status = _SendQueued();
	if (status != B_OK) {
		fState = CLOSED;
		return status;
	}

	// wait until 3-way handshake is complete (if needed)

	bigtime_t timeout = min_c(socket->send.timeout, TCP_CONNECTION_TIMEOUT);
	if (timeout == 0) {
		// we're a non-blocking socket
		return EINPROGRESS;
	}

	locker.Unlock();

	status = acquire_sem_etc(fSendLock, 1, B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, timeout);

	TRACE("  Connect(): Connection complete: %s", strerror(status));
	return status;
}


status_t
TCPEndpoint::Accept(struct net_socket **_acceptedSocket)
{
	TRACE("Accept()");

	// TODO: test for pending sockets
	// TODO: test for non-blocking I/O
	status_t status;
	do {
		status = acquire_sem_etc(fAcceptSemaphore, 1, B_RELATIVE_TIMEOUT,
			socket->receive.timeout);
		if (status < B_OK)
			return status;

		status = gSocketModule->dequeue_connected(socket, _acceptedSocket);
	} while (status < B_OK);

	return status;
}


status_t
TCPEndpoint::Bind(sockaddr *address)
{
	TRACE("Bind() on address %s",
		  AddressString(gDomain, address, true).Data());

	RecursiveLocker lock(fLock);

	if (fState != CLOSED)
		return EISCONN;

	// let IP check whether there is an interface that supports the given address:
	status_t status = next->module->bind(next, address);
	if (status < B_OK)
		return status;

	if (gAddressModule->get_port(address) == 0)
		status = gEndpointManager->BindToEphemeral(this, address);
	else
		status = gEndpointManager->Bind(this, address);

	return status;
}


status_t
TCPEndpoint::Unbind(struct sockaddr *address)
{
	TRACE("Unbind()");

	RecursiveLocker lock(fLock);
	return gEndpointManager->Unbind(this);
}


status_t
TCPEndpoint::Listen(int count)
{
	TRACE("Listen()");

	RecursiveLocker lock(fLock);

	if (fState != CLOSED)
		return B_BAD_VALUE;
	if (!IsBound())
		return EDESTADDRREQ;

	fAcceptSemaphore = create_sem(0, "tcp accept");
	fState = LISTEN;
	return B_OK;
}


status_t
TCPEndpoint::Shutdown(int direction)
{
	TRACE("Shutdown(%i)", direction);

	RecursiveLocker lock(fLock);

	if (direction == SHUT_RD || direction == SHUT_RDWR) {
		fFlags |= FLAG_NO_RECEIVE;
	}

	if (direction == SHUT_WR || direction == SHUT_RDWR)
		_ShutdownEgress(false);

	return B_OK;
}


/*!
	Puts data contained in \a buffer into send buffer
*/
status_t
TCPEndpoint::SendData(net_buffer *buffer)
{
	TRACE("SendData(buffer %p, size %lu, flags %lx)",
		  buffer, buffer->size, buffer->flags);

	RecursiveLocker lock(fLock);

	if (fFlags & FLAG_NO_SEND) {
		// TODO: send SIGPIPE signal to app?
		return EPIPE;
	}

	size_t bytesLeft = buffer->size;

	do {
		net_buffer *chunk;
		if (bytesLeft > socket->send.buffer_size) {
			// divide the buffer in multiple of the maximum segment size
			size_t chunkSize = ((socket->send.buffer_size >> 1) / fSendMaxSegmentSize)
				* fSendMaxSegmentSize;

			chunk = gBufferModule->split(buffer, chunkSize);
			TRACE("  Send() split buffer at %lu (buffer size %lu, mss %lu) -> %p",
				  chunkSize, socket->send.buffer_size, fSendMaxSegmentSize, chunk);
			if (chunk == NULL)
				return B_NO_MEMORY;
		} else
			chunk = buffer;

		while (fSendQueue.Free() < chunk->size) {
			lock.Unlock();

			status_t status = acquire_sem_etc(fSendLock, 1,
				B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, socket->send.timeout);
			if (status < B_OK)
				return status;

			lock.Lock();
		}

		// TODO: check state!

		if (chunk->size == 0) {
			gBufferModule->free(chunk);
			return B_OK;
		}

		size_t chunkSize = chunk->size;
		fSendQueue.Add(chunk);

		status_t status = _SendQueued();

		if (buffer != chunk) {
			// as long as we didn't eat the buffer, we can still return an error code
			// (we don't own the buffer if we return an error code)
			if (status < B_OK)
				return status;
		}

		bytesLeft -= chunkSize;
	} while (bytesLeft > 0);

	return B_OK;
}


ssize_t
TCPEndpoint::SendAvailable()
{
	TRACE("SendAvailable()");

	RecursiveLocker locker(fLock);
	return fSendQueue.Free();
}


status_t
TCPEndpoint::ReadData(size_t numBytes, uint32 flags, net_buffer** _buffer)
{
	TRACE("ReadData(%lu bytes)", numBytes);

	RecursiveLocker locker(fLock);

	if (fState == SYNCHRONIZE_SENT || fState == SYNCHRONIZE_RECEIVED) {
		// we need to wait until the connection becomes established
		if (flags & MSG_DONTWAIT)
			return B_WOULD_BLOCK;

		locker.Unlock();

		status_t status = acquire_sem_etc(fSendLock, 1,
			B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, socket->receive.timeout);
		if (status < B_OK)
			return status;

		locker.Lock();
	}

	if ((fFlags & FLAG_NO_RECEIVE) != 0 && fReceiveQueue.Available() == 0) {
		// there is no data left in the queue, and we can't receive anything, anymore
		*_buffer = NULL;
		return B_OK;
	}

	// read data out of buffer
	// TODO: add support for urgent data (MSG_OOB)

	bigtime_t timeout = system_time() + socket->receive.timeout;

	while (fReceiveQueue.PushedData() == 0
			&& fReceiveQueue.Available() < numBytes
			&& (fFlags & FLAG_NO_RECEIVE) == 0) {
		locker.Unlock();

		status_t status = acquire_sem_etc(fReceiveLock, 1,
			B_ABSOLUTE_TIMEOUT | B_CAN_INTERRUPT, timeout);

		locker.Lock();

		if (status < B_OK) {
			// TODO: If we are timing out, should we push the
			//       available data?
			if (status == B_TIMED_OUT && fReceiveQueue.Available() > 0)
				break;
			return status;
		}
	}

	TRACE("ReadData(): read %lu bytes, %lu are available",
		  numBytes, fReceiveQueue.Available());

	if (numBytes < fReceiveQueue.Available())
		release_sem_etc(fReceiveLock, 1, B_DO_NOT_RESCHEDULE);

	return fReceiveQueue.Get(numBytes, (flags & MSG_PEEK) == 0, _buffer);
}


ssize_t
TCPEndpoint::ReadAvailable()
{
	TRACE("ReadAvailable()");

	RecursiveLocker locker(fLock);
	return _AvailableBytesOrDisconnect();
}


//	#pragma mark - misc


bool
TCPEndpoint::IsBound() const
{
	return !gAddressModule->is_empty_address((sockaddr *)&socket->address, true);
}


status_t
TCPEndpoint::DelayedAcknowledge()
{
	// if the timer is already running, and there is still more than
	// half of the receive window free, just wait for the timer to expire
	if (gStackModule->is_timer_active(&fDelayedAcknowledgeTimer)
		&& (fReceiveMaxAdvertised - fReceiveNext) > (socket->receive.buffer_size >> 1))
		return B_OK;

	if (gStackModule->cancel_timer(&fDelayedAcknowledgeTimer)) {
		// timer was active, send an ACK now (with the exception above,
		// we send every other ACK)
		return _SendQueued();
	}

	gStackModule->set_timer(&fDelayedAcknowledgeTimer, TCP_DELAYED_ACKNOWLEDGE_TIMEOUT);
	return B_OK;
}


status_t
TCPEndpoint::SendAcknowledge()
{
	return _SendQueued(true);
}


void
TCPEndpoint::_StartPersistTimer()
{
	gStackModule->set_timer(&fPersistTimer, 1000000LL);
}


void
TCPEndpoint::_EnterTimeWait()
{
	gStackModule->set_timer(&fTimeWaitTimer, TCP_MAX_SEGMENT_LIFETIME << 1);
}


status_t
TCPEndpoint::UpdateTimeWait()
{
	return B_OK;
}


//	#pragma mark - receive


int32
TCPEndpoint::ListenReceive(tcp_segment_header &segment, net_buffer *buffer)
{
	// Essentially, we accept only TCP_FLAG_SYNCHRONIZE in this state,
	// but the error behaviour differs
	if (segment.flags & TCP_FLAG_RESET)
		return DROP;
	if (segment.flags & TCP_FLAG_ACKNOWLEDGE)
		return DROP | RESET;
	if ((segment.flags & TCP_FLAG_SYNCHRONIZE) == 0)
		return DROP;

	// TODO: drop broadcast/multicast

	// spawn new endpoint for accept()
	net_socket *newSocket;
	if (gSocketModule->spawn_pending_socket(socket, &newSocket) < B_OK)
		return DROP;

	gAddressModule->set_to((sockaddr *)&newSocket->address,
		(sockaddr *)&buffer->destination);
	gAddressModule->set_to((sockaddr *)&newSocket->peer,
		(sockaddr *)&buffer->source);

	TCPEndpoint *endpoint = (TCPEndpoint *)newSocket->first_protocol;

	// TODO: proper error handling!

	endpoint->fRoute = gDatalinkModule->get_route(gDomain,
		(sockaddr *)&newSocket->peer);
	if (endpoint->fRoute == NULL)
		return DROP;

	if (gEndpointManager->SetConnection(endpoint, (sockaddr *)&buffer->destination,
			(sockaddr *)&buffer->source, NULL) < B_OK)
		return DROP;

	endpoint->fInitialReceiveSequence = segment.sequence;
	endpoint->fReceiveQueue.SetInitialSequence(segment.sequence + 1);
	endpoint->fState = SYNCHRONIZE_RECEIVED;
	endpoint->fAcceptSemaphore = fAcceptSemaphore;
	endpoint->fReceiveMaxSegmentSize = _GetMSS((sockaddr *)&newSocket->peer);
		// 40 bytes for IP and TCP header without any options
		// TODO: make this depending on the RTF_LOCAL flag?
	endpoint->fReceiveNext = segment.sequence + 1;
		// account for the extra sequence number for the synchronization

	endpoint->fInitialSendSequence = system_time() >> 4;
	endpoint->fSendNext = endpoint->fInitialSendSequence;
	endpoint->fSendUnacknowledged = endpoint->fSendNext;
	endpoint->fSendMax = endpoint->fSendNext;

	// set options
	if ((fOptions & TCP_NOOPT) == 0) {
		if (segment.max_segment_size > 0)
			endpoint->fSendMaxSegmentSize = segment.max_segment_size;
		else
			endpoint->fReceiveMaxSegmentSize = TCP_DEFAULT_MAX_SEGMENT_SIZE;
		if (segment.has_window_shift) {
			endpoint->fFlags |= FLAG_OPTION_WINDOW_SHIFT;
			endpoint->fSendWindowShift = segment.window_shift;
		} else {
			endpoint->fFlags &= ~FLAG_OPTION_WINDOW_SHIFT;
			endpoint->fReceiveWindowShift = 0;
		}
	}

	// send SYN+ACK
	status_t status = endpoint->_SendQueued();

	endpoint->fInitialSendSequence = endpoint->fSendNext;
	endpoint->fSendQueue.SetInitialSequence(endpoint->fSendNext);

	if (status < B_OK)
		return DROP;

	segment.flags &= ~TCP_FLAG_SYNCHRONIZE;
		// we handled this flag now, it must not be set for further processing

	return endpoint->Receive(segment, buffer);
		// TODO: here, the ack/delayed ack call will be made on the parent socket!
}


int32
TCPEndpoint::SynchronizeSentReceive(tcp_segment_header &segment, net_buffer *buffer)
{
	if ((segment.flags & TCP_FLAG_ACKNOWLEDGE) != 0
		&& (fInitialSendSequence >= segment.acknowledge
			|| fSendMax < segment.acknowledge))
		return DROP | RESET;

	if (segment.flags & TCP_FLAG_RESET) {
		fError = ECONNREFUSED;
		fState = CLOSED;
		return DROP;
	}

	if ((segment.flags & TCP_FLAG_SYNCHRONIZE) == 0)
		return DROP;

	fInitialReceiveSequence = segment.sequence;
	segment.sequence++;

	fSendUnacknowledged = segment.acknowledge;
	fReceiveNext = segment.sequence;
	fReceiveQueue.SetInitialSequence(fReceiveNext);

	if (segment.max_segment_size > 0)
		fSendMaxSegmentSize = segment.max_segment_size;
	else
		fReceiveMaxSegmentSize = TCP_DEFAULT_MAX_SEGMENT_SIZE;
	if (segment.has_window_shift) {
		fFlags |= FLAG_OPTION_WINDOW_SHIFT;
		fSendWindowShift = segment.window_shift;
	} else {
		fFlags &= ~FLAG_OPTION_WINDOW_SHIFT;
		fReceiveWindowShift = 0;
	}

	if (segment.flags & TCP_FLAG_ACKNOWLEDGE) {
		// the connection has been established
		fState = ESTABLISHED;

		if (socket->parent != NULL) {
			gSocketModule->set_connected(socket);
			release_sem_etc(fAcceptSemaphore, 1, B_DO_NOT_RESCHEDULE);
		}

		release_sem_etc(fSendLock, 1, B_DO_NOT_RESCHEDULE);
		release_sem_etc(fReceiveLock, 1, B_DO_NOT_RESCHEDULE);
			// TODO: this is not enough - we need to use B_RELEASE_ALL
		gSocketModule->notify(socket, B_SELECT_READ, fReceiveQueue.Available());
	} else {
		// simultaneous open
		fState = SYNCHRONIZE_RECEIVED;
	}

	segment.flags &= ~TCP_FLAG_SYNCHRONIZE;
		// we handled this flag now, it must not be set for further processing

	return Receive(segment, buffer) | IMMEDIATE_ACKNOWLEDGE;
}


int32
TCPEndpoint::Receive(tcp_segment_header &segment, net_buffer *buffer)
{
	TRACE("Receive(): Connection in state %d received packet %p (%lu bytes) with flags 0x%x, seq %lu, ack %lu!",
		  fState, buffer, buffer->size, segment.flags, segment.sequence, segment.acknowledge);

	// TODO: rethink locking!

	uint32 advertisedWindow = (uint32)segment.advertised_window << fSendWindowShift;

	// First, handle the most common case for uni-directional data transfer
	// (known as header prediction - the segment must not change the window,
	// and must be the expected sequence, and contain no control flags)

	if (fState == ESTABLISHED
		&& segment.AcknowledgeOnly()
		&& fReceiveNext == segment.sequence
		&& advertisedWindow > 0 && advertisedWindow == fSendWindow
		&& fSendNext == fSendMax) {
		if (buffer->size == 0) {
			// this is a pure acknowledge segment - we're on the sending end
			if (fSendUnacknowledged < segment.acknowledge
				&& fSendMax >= segment.acknowledge) {
				TRACE("Receive(): header prediction send!");
				// and it only acknowledges outstanding data

				// TODO: update RTT estimators

				fSendQueue.RemoveUntil(segment.acknowledge);
				fSendUnacknowledged = segment.acknowledge;

				// stop retransmit timer
				gStackModule->cancel_timer(&fRetransmitTimer);

				// notify threads waiting on the socket to become writable again
				release_sem_etc(fSendLock, 1, B_DO_NOT_RESCHEDULE);
					// TODO: real conditional locking needed!
				gSocketModule->notify(socket, B_SELECT_WRITE, fSendWindow);

				// if there is data left to be send, send it now
				_SendQueued();
				return DROP;
			}
		} else if (segment.acknowledge == fSendUnacknowledged
			&& fReceiveQueue.IsContiguous()
			&& fReceiveQueue.Free() >= buffer->size) {
			TRACE("Receive(): header prediction receive!");
			// we're on the receiving end of the connection, and this segment
			// is the one we were expecting, in-sequence
			if (fFlags & FLAG_NO_RECEIVE) {
				return DROP;
			} else {
				fReceiveNext += buffer->size;
				TRACE("Receive(): receive next = %lu", (uint32)fReceiveNext);
				fReceiveQueue.Add(buffer, segment.sequence);
				if (segment.flags & TCP_FLAG_PUSH)
					fReceiveQueue.SetPushPointer(fReceiveNext);

				release_sem_etc(fReceiveLock, 1, B_DO_NOT_RESCHEDULE);
					// TODO: real conditional locking needed!
				gSocketModule->notify(socket, B_SELECT_READ,
									  fReceiveQueue.Available());

				return KEEP | ACKNOWLEDGE;
			}
		}
	}

	// The fast path was not applicable, so we continue with the standard processing
	// of the incoming segment

	// First have a look at the data being sent

	if (segment.flags & TCP_FLAG_RESET) {
		// is this a valid reset?
		if (fLastAcknowledgeSent <= segment.sequence
			&& tcp_sequence(segment.sequence) < fLastAcknowledgeSent + fReceiveWindow) {
			// TODO: close connection depending on state
			fError = ECONNREFUSED;
			fState = CLOSED;

			release_sem_etc(fReceiveLock, 1, B_DO_NOT_RESCHEDULE);
				// TODO: real conditional locking needed!
			gSocketModule->notify(socket, B_SELECT_READ, fReceiveQueue.Available());
		}

		return DROP;
	}
	if ((segment.flags & TCP_FLAG_SYNCHRONIZE) != 0
		|| (fState == SYNCHRONIZE_RECEIVED
			&& (fInitialReceiveSequence > segment.sequence
				|| (segment.flags & TCP_FLAG_ACKNOWLEDGE) != 0
					&& (fSendUnacknowledged > segment.acknowledge
						|| fSendMax < segment.acknowledge)))) {
		// reset the connection - either the initial SYN was faulty, or we received
		// a SYN within the data stream
		return DROP | RESET;
	}

	fReceiveWindow = max_c(fReceiveQueue.Free(), fReceiveWindow);
		// the window must not shrink

	// trim buffer to be within the receive window
	int32 drop = fReceiveNext - segment.sequence;
	if (drop > 0) {
		if ((uint32)drop > buffer->size
			|| ((uint32)drop == buffer->size && (segment.flags & TCP_FLAG_FINISH) == 0)) {
			// don't accidently remove a FIN we shouldn't remove
			segment.flags &= ~TCP_FLAG_FINISH;
			drop = buffer->size;
		}

		// remove duplicate data at the start
		TRACE("* remove %ld bytes from the start", drop);
		gBufferModule->remove_header(buffer, drop);
		segment.sequence += drop;
	}

	int32 action = KEEP;

	drop = segment.sequence + buffer->size - (fReceiveNext + fReceiveWindow);
	if (drop > 0) {
		// remove data exceeding our window
		if ((uint32)drop >= buffer->size) {
			// if we can accept data, or the segment is not what we'd expect,
			// drop the segment (an immediate acknowledge is always triggered)
			if (fReceiveWindow != 0 || segment.sequence != fReceiveNext)
				return DROP | IMMEDIATE_ACKNOWLEDGE;

			action |= IMMEDIATE_ACKNOWLEDGE;
		}

		if ((segment.flags & TCP_FLAG_FINISH) != 0) {
			// we need to remove the finish, too, as part of the data
			drop--;
		}

		segment.flags &= ~(TCP_FLAG_FINISH | TCP_FLAG_PUSH);
		TRACE("* remove %ld bytes from the end", drop);
		gBufferModule->remove_trailer(buffer, drop);
	}

	// Then look at the acknowledgement for any updates

	if ((segment.flags & TCP_FLAG_ACKNOWLEDGE) != 0) {
		// process acknowledged data
		if (fState == SYNCHRONIZE_RECEIVED) {
			// TODO: window scaling!
			if (socket->parent != NULL) {
				gSocketModule->set_connected(socket);
				release_sem_etc(fAcceptSemaphore, 1, B_DO_NOT_RESCHEDULE);
			}

			fState = ESTABLISHED;

			release_sem_etc(fSendLock, 1, B_DO_NOT_RESCHEDULE);
			release_sem_etc(fReceiveLock, 1, B_DO_NOT_RESCHEDULE);
				// TODO: real conditional locking needed!
			gSocketModule->notify(socket, B_SELECT_READ, fReceiveQueue.Available());
		}
		if (fSendMax < segment.acknowledge)
			return DROP | IMMEDIATE_ACKNOWLEDGE;
		if (fSendUnacknowledged >= segment.acknowledge) {
			// this is a duplicate acknowledge
			// TODO: handle this!
			if (buffer->size == 0 && advertisedWindow == fSendWindow
				&& (segment.flags & TCP_FLAG_FINISH) == 0) {
				TRACE("Receive(): duplicate ack!");
				fDuplicateAcknowledgeCount++;

				gStackModule->cancel_timer(&fRetransmitTimer);

				fSendNext = segment.acknowledge;
				_SendQueued();
				return DROP;
			}
		} else {
			// this segment acknowledges in flight data
			fDuplicateAcknowledgeCount = 0;

			if (fSendMax == segment.acknowledge) {
				// there is no outstanding data to be acknowledged
				// TODO: if the transmit timer function is already waiting
				//	to acquire this endpoint's lock, we should stop it anyway
				TRACE("Receive(): all inflight data ack'd!");
				gStackModule->cancel_timer(&fRetransmitTimer);
			} else {
				TRACE("Receive(): set retransmit timer!");
				// TODO: set retransmit timer correctly
				if (!gStackModule->is_timer_active(&fRetransmitTimer))
					gStackModule->set_timer(&fRetransmitTimer, 1000000LL);
			}

			fSendUnacknowledged = segment.acknowledge;
			fSendQueue.RemoveUntil(segment.acknowledge);

			if (fSendNext < fSendUnacknowledged)
				fSendNext = fSendUnacknowledged;

			if (segment.acknowledge > fSendQueue.LastSequence() && fState > ESTABLISHED) {
				// our TCP_FLAG_FINISH has been acknowledged
				TRACE("Receive(): FIN has been acknowledged!");

				switch (fState) {
					case FINISH_SENT:
						fState = FINISH_ACKNOWLEDGED;
						break;
					case CLOSING:
						fState = TIME_WAIT;
						_EnterTimeWait();
						break;
					case WAIT_FOR_FINISH_ACKNOWLEDGE:
						fState = CLOSED;
						break;

					default:
						break;
				}
			}
		}
	}

	// TODO: update window
	fSendWindow = advertisedWindow;
	if (advertisedWindow > fSendMaxWindow)
		fSendMaxWindow = advertisedWindow;

	// TODO: process urgent data!
	// TODO: ignore data *after* FIN

	if (segment.flags & TCP_FLAG_FINISH) {
		TRACE("Receive(): peer is finishing connection!");
		fReceiveNext++;
		fFlags |= FLAG_NO_RECEIVE;

		release_sem_etc(fReceiveLock, 1, B_DO_NOT_RESCHEDULE);
			// TODO: real conditional locking needed!
		gSocketModule->notify(socket, B_SELECT_READ, _AvailableBytesOrDisconnect());

		// other side is closing connection; change states
		switch (fState) {
			case ESTABLISHED:
			case SYNCHRONIZE_RECEIVED:
				fState = FINISH_RECEIVED;
				action |= IMMEDIATE_ACKNOWLEDGE;
				break;
			case FINISH_ACKNOWLEDGED:
				fState = TIME_WAIT;
				_EnterTimeWait();
				break;
			case FINISH_RECEIVED:
				// a second FIN?
				break;
			case FINISH_SENT:
				// simultaneous close
				fState = CLOSING;
				break;

			default:
				break;
		}
	}

	if (buffer->size > 0 || (segment.flags & (TCP_FLAG_SYNCHRONIZE | TCP_FLAG_FINISH)) != 0)
		action |= ACKNOWLEDGE;

	TRACE("Receive():Entering state %d, segment action %ld", fState, action);

	// state machine is done switching states and the data is good.
	// put it in the receive buffer

	if (buffer->size > 0) {
		fReceiveQueue.Add(buffer, segment.sequence);
		fReceiveNext = fReceiveQueue.NextSequence();
		TRACE("Receive(): adding data, receive next = %lu!", (uint32)fReceiveNext);

		release_sem_etc(fReceiveLock, 1, B_DO_NOT_RESCHEDULE);
			// TODO: real conditional locking needed!
		gSocketModule->notify(socket, B_SELECT_READ, fReceiveQueue.Available());
	} else
		gBufferModule->free(buffer);

	if (segment.flags & TCP_FLAG_PUSH)
		fReceiveQueue.SetPushPointer(fReceiveNext);

	return action;
}


//	#pragma mark - send


/*!
	The segment flags to send depend completely on the state we're in.
	_SendQueued() need to be smart enough to clear TCP_FLAG_FINISH when
	it couldn't send all the data.
*/
inline uint8
TCPEndpoint::_CurrentFlags()
{
	switch (fState) {
		case CLOSED:
			return TCP_FLAG_RESET | TCP_FLAG_ACKNOWLEDGE;
		case LISTEN:
			return 0;

		case SYNCHRONIZE_SENT:
			return TCP_FLAG_SYNCHRONIZE;
		case SYNCHRONIZE_RECEIVED:
			return TCP_FLAG_SYNCHRONIZE | TCP_FLAG_ACKNOWLEDGE;

		case ESTABLISHED:
		case FINISH_RECEIVED:
		case FINISH_ACKNOWLEDGED:
		case TIME_WAIT:
			return TCP_FLAG_ACKNOWLEDGE;

		case WAIT_FOR_FINISH_ACKNOWLEDGE:
		case FINISH_SENT:
		case CLOSING:
			return TCP_FLAG_FINISH | TCP_FLAG_ACKNOWLEDGE;
	}

	// never gets here
	return 0;
}


inline bool
TCPEndpoint::_ShouldSendSegment(tcp_segment_header &segment, uint32 length,
	bool outstandingAcknowledge)
{
	if (length > 0) {
		// Avoid the silly window syndrome - we only send a segment in case:
		// - we have a full segment to send, or
		// - we're at the end of our buffer queue, or
		// - the buffer is at least larger than half of the maximum send window, or
		// - we're retransmitting data
		if (length == fSendMaxSegmentSize
			|| ((!outstandingAcknowledge || (fOptions & TCP_NODELAY) != 0)
				&& tcp_sequence(fSendNext + length) == fSendQueue.LastSequence())
			|| (fSendMaxWindow > 0 && length >= fSendMaxWindow / 2)
			|| fSendNext < fSendMax)
			return true;
	}

	// check if we need to send a window update to the peer
	if (segment.advertised_window > 0) {
		// correct the window to take into account what already has been advertised
		uint32 window = (min_c(segment.advertised_window, TCP_MAX_WINDOW)
			<< fReceiveWindowShift) - (fReceiveMaxAdvertised - fReceiveNext);

		// if we can advertise a window larger than twice the maximum segment
		// size, or half the maximum buffer size we send a window update
		if (window >= (fReceiveMaxSegmentSize << 1)
			|| window >= (socket->receive.buffer_size >> 1))
			return true;
	}

	if ((segment.flags & (TCP_FLAG_SYNCHRONIZE | TCP_FLAG_FINISH | TCP_FLAG_RESET)) != 0)
		return true;

	// there is no reason to send a segment just now
	return false;
}


/*!
	Sends one or more TCP segments with the data waiting in the queue, or some
	specific flags that need to be sent.
*/
status_t
TCPEndpoint::_SendQueued(bool force)
{
	if (fRoute == NULL)
		return B_ERROR;

	// Determine if we need to send anything at all

	tcp_segment_header segment;
	segment.flags = _CurrentFlags();

	uint32 sendWindow = fSendWindow;
	uint32 available = fSendQueue.Available(fSendNext);
	bool outstandingAcknowledge = fSendMax != fSendUnacknowledged;

	if (force && sendWindow == 0 && fSendNext <= fSendQueue.LastSequence()) {
		// send one byte of data to ask for a window update
		// (triggered by the persist timer)
		segment.flags &= ~TCP_FLAG_FINISH;
		sendWindow = 1;
	}

	int32 length = min_c(available, sendWindow) - (fSendNext - fSendUnacknowledged);
	if (length < 0) {
		// either the window shrank, or we sent a still unacknowledged FIN 
		length = 0;
		if (sendWindow == 0) {
			// Enter persist state
			// TODO: stop retransmit timer!
			fSendNext = fSendUnacknowledged;
		}
	}

	uint32 segmentLength = min_c((uint32)length, fSendMaxSegmentSize);
	if (tcp_sequence(fSendNext + segmentLength) > fSendUnacknowledged + available) {
		// we'll still have data in the queue after the next write, so remove the FIN
		segment.flags &= ~TCP_FLAG_FINISH;
	}

	segment.advertised_window = min_c(65535, fReceiveQueue.Free());
		// TODO: support shift option!
	segment.acknowledge = fReceiveNext;
	segment.urgent_offset = 0;

	while (true) {
		// Determine if we should really send this segment
		if (!force && !_ShouldSendSegment(segment, segmentLength, outstandingAcknowledge)) {
			if (fSendQueue.Available()
				&& !gStackModule->is_timer_active(&fPersistTimer)
				&& !gStackModule->is_timer_active(&fRetransmitTimer))
				_StartPersistTimer();

			return B_OK;
		}

		net_buffer *buffer = gBufferModule->create(256);
		if (buffer == NULL)
			return B_NO_MEMORY;

		status_t status = B_OK;
		if (segmentLength > 0)
			fSendQueue.Get(buffer, fSendNext, segmentLength);
		if (status < B_OK) {
			gBufferModule->free(buffer);
			return status;
		}

		gAddressModule->set_to((sockaddr *)&buffer->source, (sockaddr *)&socket->address);
		gAddressModule->set_to((sockaddr *)&buffer->destination, (sockaddr *)&socket->peer);
		TRACE("SendQueued() flags %x, buffer %p, size %lu, from address %s to %s",
			segment.flags, buffer, buffer->size,
			AddressString(gDomain, (sockaddr *)&buffer->source, true).Data(),
			AddressString(gDomain, (sockaddr *)&buffer->destination, true).Data());

		uint32 size = buffer->size;
		if (length > 0 && fSendNext + segmentLength == fSendQueue.LastSequence()) {
			// if we've emptied our send queue, set the PUSH flag
			segment.flags |= TCP_FLAG_PUSH;
		}

		segment.sequence = fSendNext;

		if ((segment.flags & TCP_FLAG_SYNCHRONIZE) != 0
			&& (fOptions & TCP_NOOPT) == 0) {
			// add connection establishment options
			segment.max_segment_size = fReceiveMaxSegmentSize;
			if ((fFlags & FLAG_OPTION_WINDOW_SHIFT) != 0) {
				segment.has_window_shift = true;
				segment.window_shift = fReceiveWindowShift;
			}
		}

		status = add_tcp_header(segment, buffer);
		if (status != B_OK) {
			gBufferModule->free(buffer);
			return status;
		}

		// TODO: we need to trim the segment to the max segment size in case
		//		the options made it too large

		// Update send status - we need to do this before we send the data
		// for local connections as the answer is directly handled

		if ((segment.flags & TCP_FLAG_SYNCHRONIZE) != 0) {
			// count SYN into sequence length and reset options for the next segment
			segment.max_segment_size = 0;
			segment.has_window_shift = false;
			size++;
		}

		if ((segment.flags & TCP_FLAG_FINISH) != 0) {
			// count FIN into sequence length
			size++;
		}

		uint32 sendMax = fSendMax;
		fSendNext += size;
		if (fSendMax < fSendNext)
			fSendMax = fSendNext;

		fReceiveMaxAdvertised = fReceiveNext
			+ ((uint32)segment.advertised_window << fReceiveWindowShift);

		status = next->module->send_routed_data(next, fRoute, buffer);
		if (status < B_OK) {
			gBufferModule->free(buffer);

			fSendNext = segment.sequence;
			fSendMax = sendMax;
				// restore send status
			return status;
		}

		length -= segmentLength;
		if (length == 0)
			break;

		segmentLength = min_c((uint32)length, fSendMaxSegmentSize);
		segment.flags &= ~(TCP_FLAG_SYNCHRONIZE | TCP_FLAG_RESET | TCP_FLAG_FINISH);
	}

	return B_OK;
}


int
TCPEndpoint::_GetMSS(const sockaddr *address) const
{
	return next->module->get_mtu(next, (sockaddr *)address) - sizeof(tcp_header);
}


ssize_t
TCPEndpoint::_AvailableBytesOrDisconnect() const
{
	size_t availableBytes = fReceiveQueue.Available();
	if (availableBytes > 0)
		return availableBytes;

	if ((fFlags & FLAG_NO_RECEIVE) != 0)
		return ENOTCONN;

	return 0;
}


status_t
TCPEndpoint::_ShutdownEgress(bool closing)
{
	tcp_state previousState = fState;

	if (fState == SYNCHRONIZE_RECEIVED || fState == ESTABLISHED)
		fState = FINISH_SENT;
	else if (fState == FINISH_RECEIVED)
		fState = WAIT_FOR_FINISH_ACKNOWLEDGE;
	else if (closing)
		fState = CLOSED;
	else
		return B_OK;

	status_t status = _SendQueued();
	if (status != B_OK) {
		fState = previousState;
		return status;
	}

	fFlags |= FLAG_NO_SEND;

	return B_OK;
}


//	#pragma mark - timer


/*static*/ void
TCPEndpoint::_RetransmitTimer(net_timer *timer, void *data)
{
	TCPEndpoint *endpoint = (TCPEndpoint *)data;

	RecursiveLocker locker(endpoint->Lock());
	if (!locker.IsLocked())
		return;

	endpoint->fSendNext = endpoint->fSendUnacknowledged;
	endpoint->_SendQueued();
	endpoint->fSendNext = endpoint->fSendMax;
}


/*static*/ void
TCPEndpoint::_PersistTimer(net_timer *timer, void *data)
{
	TCPEndpoint *endpoint = (TCPEndpoint *)data;

	RecursiveLocker locker(endpoint->Lock());
	if (!locker.IsLocked())
		return;

	endpoint->_SendQueued(true);
}


/*static*/ void
TCPEndpoint::_DelayedAcknowledgeTimer(struct net_timer *timer, void *data)
{
	TCPEndpoint *endpoint = (TCPEndpoint *)data;

	RecursiveLocker locker(endpoint->Lock());
	if (!locker.IsLocked())
		return;

	endpoint->_SendQueued(true);
}


/*static*/ void
TCPEndpoint::_TimeWaitTimer(struct net_timer *timer, void *data)
{
	TCPEndpoint *endpoint = (TCPEndpoint *)data;

	if (recursive_lock_lock(&endpoint->Lock()) < B_OK)
		return;

	gSocketModule->delete_socket(endpoint->socket);
}

