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


// References:
//  - RFC 793 - Transmission Control Protocol
//  - RFC 813 - Window and Acknowledgement Strategy in TCP
//
// Things this implementation currently doesn't implement:
//
// TCP Slow Start, Congestion Avoidance, Fast Retransmit, and Fast Recovery, RFC 2001, RFC 2581, RFC 3042
// NewReno Modification to TCP's Fast Recovery, RFC 2582
// Explicit Congestion Notification (ECN), RFC 3168
// SYN-Cache
// TCP Extensions for High Performance, RFC 1323
// SACK, Selective Acknowledgment - RFC 2018, RFC 2883, RFC 3517
// Forward RTO-Recovery, RFC 4138

//#define TRACE_TCP

#ifdef TRACE_TCP
// the space before ', ##args' is important in order for this to work with cpp 2.95
#	define TRACE(format, args...)	dprintf("TCP [%llu] %p (%12s) " format "\n", \
		system_time(), this, name_for_state(fState) , ##args)
#else
#	define TRACE(args...)			do { } while (0)
#endif

// Initial estimate for packet round trip time (RTT)
#define	TCP_INITIAL_RTT	120000000LL

// constants for the fFlags field
enum {
	FLAG_OPTION_WINDOW_SCALE	= 0x01,
	FLAG_OPTION_TIMESTAMP		= 0x02,
	// TODO: Should FLAG_NO_RECEIVE apply as well to received connections?
	//       That is, what is expected from accept() after a shutdown()
	//       is performed on a listen()ing socket.
	FLAG_NO_RECEIVE				= 0x04,
};


static inline bigtime_t
absolute_timeout(bigtime_t timeout)
{
	if (timeout == 0 || timeout == B_INFINITE_TIMEOUT)
		return timeout;

	return timeout + system_time();
}


static inline status_t
posix_error(status_t error)
{
	if (error == B_TIMED_OUT)
		return B_WOULD_BLOCK;

	return error;
}


static inline bool
in_window(const tcp_sequence &sequence, const tcp_sequence &rcvNext,
	uint32 rcvWindow)
{
	return sequence >= rcvNext && sequence < (rcvNext + rcvWindow);
}


static inline bool
segment_in_sequence(const tcp_segment_header &segment, int size,
	const tcp_sequence &rcvNext, uint32 rcvWindow)
{
	tcp_sequence sequence(segment.sequence);
	if (size == 0) {
		if (rcvWindow == 0)
			return sequence == rcvNext;
		return in_window(sequence, rcvNext, rcvWindow);
	} else {
		if (rcvWindow == 0)
			return false;
		return in_window(sequence, rcvNext, rcvWindow)
			|| in_window(sequence + size - 1, rcvNext, rcvWindow);
	}
}


WaitList::WaitList(const char *name)
{
	fSem = create_sem(0, name);
}


WaitList::~WaitList()
{
	delete_sem(fSem);
}


status_t
WaitList::InitCheck() const
{
	return fSem;
}


status_t
WaitList::Wait(RecursiveLocker &locker, bigtime_t timeout, bool wakeNext)
{
	locker.Unlock();
	status_t status = acquire_sem_etc(fSem, 1, B_ABSOLUTE_TIMEOUT
		| B_CAN_INTERRUPT, timeout);
	locker.Lock();
	if (wakeNext && status == B_OK)
		Signal();
	return status;
}


void
WaitList::Signal()
{
	release_sem_etc(fSem, 1, B_DO_NOT_RESCHEDULE
		| B_RELEASE_IF_WAITING_ONLY);
}


TCPEndpoint::TCPEndpoint(net_socket *socket)
	:
	fManager(NULL),
	fReceiveList("tcp receive"),
	fSendList("tcp send"),
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
	fReceivedTSval(0),
	fState(CLOSED),
	fFlags(FLAG_OPTION_WINDOW_SCALE | FLAG_OPTION_TIMESTAMP),
	fError(B_OK),
	fSpawned(false)
{
	//gStackModule->init_timer(&fTimer, _TimeWait, this);

	recursive_lock_init(&fLock, "tcp lock");
		// TODO: to be replaced with a real locking strategy!

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

	if (fManager) {
		fManager->Unbind(this);
		return_endpoint_manager(fManager);
	}

	recursive_lock_destroy(&fLock);
}


status_t
TCPEndpoint::InitCheck() const
{
	if (fLock.sem < B_OK)
		return fLock.sem;

	if (fReceiveList.InitCheck() < B_OK)
		return fReceiveList.InitCheck();

	if (fSendList.InitCheck() < B_OK)
		return fSendList.InitCheck();

	return B_OK;
}


//	#pragma mark - protocol API


status_t
TCPEndpoint::Open()
{
	TRACE("Open()");

	if (Domain() == NULL || AddressModule() == NULL)
		return EAFNOSUPPORT;

	fManager = create_endpoint_manager(Domain());
	if (fManager == NULL)
		return EAFNOSUPPORT;

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

	if (socket->options & SO_LINGER) {
		TRACE("Close(): Lingering for %i secs", socket->linger);

		bigtime_t maximum = absolute_timeout(socket->linger * 1000000LL);

		while (fSendQueue.Used() > 0) {
			status = fSendList.Wait(lock, maximum);
			if (status == B_TIMED_OUT || status == B_WOULD_BLOCK)
				break;
			else if (status < B_OK)
				return status;
		}

		TRACE("Close(): after waiting, the SendQ was left with %lu bytes.",
			fSendQueue.Used());
	}

	// TODO: do i need to wait until fState returns to CLOSED?
	return B_OK;
}


status_t
TCPEndpoint::Free()
{
	TRACE("Free()");

	RecursiveLocker _(fLock);

	if (fState <= SYNCHRONIZE_SENT || fState == TIME_WAIT)
		return B_OK;

	// we are only interested in the timer, not in changing state
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
		AddressString(Domain(), address, true).Data());

	RecursiveLocker locker(fLock);

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
		fRoute = gDatalinkModule->get_route(Domain(), (sockaddr *)address);
		TRACE("  Connect(): Using Route %p", fRoute);
		if (fRoute == NULL)
			return ENETUNREACH;
	}

	// make sure connection does not already exist
	status_t status = fManager->SetConnection(this,
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

	// If we are running over Loopback, after _SendQueued() returns we
	// may be in ESTABLISHED already.
	if (fState == ESTABLISHED) {
		TRACE("  Connect() completed after _SendQueued()");
		return B_OK;
	}

	// wait until 3-way handshake is complete (if needed)
	bigtime_t timeout = min_c(socket->send.timeout, TCP_CONNECTION_TIMEOUT);
	if (timeout == 0) {
		// we're a non-blocking socket
		TRACE("  Connect() delayed, return EINPROGRESS");
		return EINPROGRESS;
	}

	status = _WaitForEstablished(locker, absolute_timeout(timeout));
	TRACE("  Connect(): Connection complete: %s", strerror(status));
	return posix_error(status);
}


status_t
TCPEndpoint::Accept(struct net_socket **_acceptedSocket)
{
	TRACE("Accept()");

	RecursiveLocker locker(fLock);

	status_t status;
	bigtime_t timeout = absolute_timeout(socket->receive.timeout);

	do {
		locker.Unlock();

		status = acquire_sem_etc(fAcceptSemaphore, 1, B_ABSOLUTE_TIMEOUT
			| B_CAN_INTERRUPT, timeout);
		if (status < B_OK)
			return status;

		locker.Lock();
		status = gSocketModule->dequeue_connected(socket, _acceptedSocket);
		if (status == B_OK)
			TRACE("  Accept() returning %p", (*_acceptedSocket)->first_protocol);
	} while (status < B_OK);

	return status;
}


status_t
TCPEndpoint::Bind(sockaddr *address)
{
	if (address == NULL)
		return B_BAD_VALUE;

	RecursiveLocker lock(fLock);

	TRACE("Bind() on address %s",
		  AddressString(Domain(), address, true).Data());

	if (fState != CLOSED)
		return EISCONN;

	// let IP check whether there is an interface that supports the given address:
	status_t status = next->module->bind(next, address);
	if (status < B_OK)
		return status;

	if (AddressModule()->get_port(address) == 0)
		status = fManager->BindToEphemeral(this);
	else
		status = fManager->Bind(this);

	TRACE("  Bind() bound to %s (status %i)",
		  AddressString(Domain(), (sockaddr *)&socket->address, true).Data(),
		  (int)status);

	return status;
}


status_t
TCPEndpoint::Unbind(struct sockaddr *address)
{
	TRACE("Unbind()");

	RecursiveLocker lock(fLock);
	return fManager->Unbind(this);
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
	if (fAcceptSemaphore < B_OK)
		return ENOBUFS;

	fState = LISTEN;
	return B_OK;
}


status_t
TCPEndpoint::Shutdown(int direction)
{
	TRACE("Shutdown(%i)", direction);

	RecursiveLocker lock(fLock);

	if (direction == SHUT_RD || direction == SHUT_RDWR)
		fFlags |= FLAG_NO_RECEIVE;

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

	if (fState == CLOSED)
		return ENOTCONN;
	else if (fState == LISTEN) {
		// TODO change socket from passive to active.
		return EOPNOTSUPP;
	} else if (fState == FINISH_SENT || fState == FINISH_ACKNOWLEDGED
				|| fState == CLOSING || fState == WAIT_FOR_FINISH_ACKNOWLEDGE
				|| fState == TIME_WAIT) {
		// TODO: send SIGPIPE signal to app?
		return EPIPE;
	}

	if (buffer->size > 0) {
		if (buffer->size > fSendQueue.Size())
			return EMSGSIZE;

		bigtime_t timeout = absolute_timeout(socket->send.timeout);

		while (fSendQueue.Free() < buffer->size) {
			status_t status = fSendList.Wait(lock, timeout);
			if (status < B_OK)
				return posix_error(status);
		}

		fSendQueue.Add(buffer);
	}

	if (fState == ESTABLISHED || fState == FINISH_RECEIVED)
		_SendQueued();

	return B_OK;
}


ssize_t
TCPEndpoint::SendAvailable()
{
	RecursiveLocker locker(fLock);

	ssize_t available;

	if (fState == FINISH_SENT || fState == FINISH_ACKNOWLEDGED
		|| fState == CLOSING || fState == WAIT_FOR_FINISH_ACKNOWLEDGE
		|| fState == TIME_WAIT || fState == LISTEN || fState == CLOSED)
		available = EPIPE;
	else
		available = fSendQueue.Free();

	TRACE("SendAvailable(): %li", available);
	return available;
}


status_t
TCPEndpoint::ReadData(size_t numBytes, uint32 flags, net_buffer** _buffer)
{
	TRACE("ReadData(%lu bytes, flags 0x%x)", numBytes, (unsigned int)flags);

	RecursiveLocker locker(fLock);

	*_buffer = NULL;

	if (fState == CLOSED)
		return ENOTCONN;

	bigtime_t timeout = absolute_timeout(socket->receive.timeout);

	if (fState == SYNCHRONIZE_SENT || fState == SYNCHRONIZE_RECEIVED) {
		if (flags & MSG_DONTWAIT)
			return B_WOULD_BLOCK;

		status_t status = _WaitForEstablished(locker, timeout);
		if (status < B_OK)
			return posix_error(status);
	}

	size_t dataNeeded = socket->receive.low_water_mark;

	// When MSG_WAITALL is set then the function should block
	// until the full amount of data can be returned.
	if (flags & MSG_WAITALL)
		dataNeeded = numBytes;

	// TODO: add support for urgent data (MSG_OOB)

	while (true) {
		if (fState == CLOSING || fState == WAIT_FOR_FINISH_ACKNOWLEDGE
				|| fState == TIME_WAIT) {
			// ``Connection closing''.
			return B_OK;
		}

		if (fReceiveQueue.Available() >= dataNeeded ||
				((fReceiveQueue.PushedData() > 0)
					&& (fReceiveQueue.PushedData() >= fReceiveQueue.Available())))
			break;

		if (fState == FINISH_RECEIVED) {
			// ``If no text is awaiting delivery, the RECEIVE will
			//   get a Connection closing''.
			return B_OK;
		}

		if (flags & MSG_DONTWAIT)
			return B_WOULD_BLOCK;

		status_t status = fReceiveList.Wait(locker, timeout, false);
		if (status < B_OK) {
			// The Open Group base specification mentions that EINTR should be
			// returned if the recv() is interrupted before _any data_ is
			// available. So we actually check if there is data, and if so,
			// push it to the user.
			if ((status == B_TIMED_OUT || status == B_INTERRUPTED)
					&& fReceiveQueue.Available() > 0)
				break;

			return posix_error(status);
		}
	}

	TRACE("  ReadData(): read %lu bytes, %lu are available.",
		  numBytes, fReceiveQueue.Available());

	if (numBytes < fReceiveQueue.Available())
		fReceiveList.Signal();

	ssize_t receivedBytes = fReceiveQueue.Get(numBytes,
		(flags & MSG_PEEK) == 0, _buffer);

	TRACE("  ReadData(): %lu bytes kept.", fReceiveQueue.Available());

	return receivedBytes;
}


ssize_t
TCPEndpoint::ReadAvailable()
{
	RecursiveLocker locker(fLock);

	TRACE("ReadAvailable(): %li", _AvailableData());

	return _AvailableData();
}


//	#pragma mark - misc


bool
TCPEndpoint::IsBound() const
{
	return !AddressModule()->is_empty_address((sockaddr *)&socket->address, true);
}


void
TCPEndpoint::DeleteSocket()
{
	// the next call will delete `this'.
	gSocketModule->delete_socket(socket);
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
		return SendAcknowledge();
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
TCPEndpoint::_ListenReceive(tcp_segment_header &segment, net_buffer *buffer)
{
	TRACE("ListenReceive()");

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

	AddressModule()->set_to((sockaddr *)&newSocket->address,
		(sockaddr *)&buffer->destination);
	AddressModule()->set_to((sockaddr *)&newSocket->peer,
		(sockaddr *)&buffer->source);

	return ((TCPEndpoint *)newSocket->first_protocol)->Spawn(this, segment, buffer);
}


int32
TCPEndpoint::Spawn(TCPEndpoint *parent, tcp_segment_header &segment,
	net_buffer *buffer)
{
	RecursiveLocker _(fLock);

	fState = SYNCHRONIZE_RECEIVED;
	fManager = parent->fManager;

	TRACE("Spawn()");

	fSpawned = true;

	sockaddr *local = (sockaddr *)&socket->address;
	sockaddr *peer = (sockaddr *)&socket->peer;

	// TODO: proper error handling!

	fRoute = gDatalinkModule->get_route(Domain(), peer);
	if (fRoute == NULL)
		return DROP;

	if (fManager->SetConnection(this, local, peer, NULL) < B_OK)
		return DROP;

	fInitialReceiveSequence = segment.sequence;
	fReceiveQueue.SetInitialSequence(segment.sequence + 1);
	fAcceptSemaphore = parent->fAcceptSemaphore;
	fReceiveMaxSegmentSize = _GetMSS(peer);
		// 40 bytes for IP and TCP header without any options
		// TODO: make this depending on the RTF_LOCAL flag?
	fReceiveNext = segment.sequence + 1;
		// account for the extra sequence number for the synchronization

	fInitialSendSequence = system_time() >> 4;
	fSendNext = fInitialSendSequence;
	fSendUnacknowledged = fSendNext;
	fSendMax = fSendNext;

	// set options
	if ((parent->fOptions & TCP_NOOPT) == 0) {
		if (segment.max_segment_size > 0)
			fSendMaxSegmentSize = segment.max_segment_size;
		else
			fReceiveMaxSegmentSize = TCP_DEFAULT_MAX_SEGMENT_SIZE;
		if (segment.has_window_shift) {
			fFlags |= FLAG_OPTION_WINDOW_SCALE;
			fSendWindowShift = segment.window_shift;
		} else {
			fFlags &= ~FLAG_OPTION_WINDOW_SCALE;
			fReceiveWindowShift = 0;
		}
	}

	_UpdateTimestamps(segment, 0, false);

	// send SYN+ACK
	status_t status = _SendQueued();

	fInitialSendSequence = fSendNext;
	fSendQueue.SetInitialSequence(fSendNext);

	if (status < B_OK)
		return DROP;

	segment.flags &= ~TCP_FLAG_SYNCHRONIZE;
		// we handled this flag now, it must not be set for further processing

	return _Receive(segment, buffer);
}


int32
TCPEndpoint::_SynchronizeSentReceive(tcp_segment_header &segment, net_buffer *buffer)
{
	TRACE("SynchronizeSentReceive()");

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
		fFlags |= FLAG_OPTION_WINDOW_SCALE;
		fSendWindowShift = segment.window_shift;
	} else {
		fFlags &= ~FLAG_OPTION_WINDOW_SCALE;
		fReceiveWindowShift = 0;
	}

	if (segment.flags & TCP_FLAG_ACKNOWLEDGE) {
		_MarkEstablished();
	} else {
		// simultaneous open
		fState = SYNCHRONIZE_RECEIVED;
	}

	_UpdateTimestamps(segment, 0, false);

	segment.flags &= ~TCP_FLAG_SYNCHRONIZE;
		// we handled this flag now, it must not be set for further processing

	return _Receive(segment, buffer) | IMMEDIATE_ACKNOWLEDGE;
}


int32
TCPEndpoint::SegmentReceived(tcp_segment_header &segment, net_buffer *buffer)
{
	RecursiveLocker locker(fLock);

	TRACE("SegmentReceived(): packet %p (%lu bytes) with flags 0x%x, seq %lu, "
		"ack %lu", buffer, buffer->size, segment.flags, segment.sequence,
		segment.acknowledge);

	int32 segmentAction = DROP;

	switch (fState) {
		case LISTEN:
			segmentAction = _ListenReceive(segment, buffer);
			break;

		case SYNCHRONIZE_SENT:
			segmentAction = _SynchronizeSentReceive(segment, buffer);
			break;

		case SYNCHRONIZE_RECEIVED:
		case ESTABLISHED:
		case FINISH_RECEIVED:
		case WAIT_FOR_FINISH_ACKNOWLEDGE:
		case FINISH_SENT:
		case FINISH_ACKNOWLEDGED:
		case CLOSING:
		case TIME_WAIT:
		case CLOSED:
			segmentAction = _SegmentReceived(segment, buffer);
			break;
	}

	// process acknowledge action as asked for by the *Receive() method
	if (segmentAction & IMMEDIATE_ACKNOWLEDGE)
		SendAcknowledge();
	else if (segmentAction & ACKNOWLEDGE)
		DelayedAcknowledge();

	return segmentAction;
}

int32
TCPEndpoint::_SegmentReceived(tcp_segment_header &segment, net_buffer *buffer)
{
	uint32 advertisedWindow = (uint32)segment.advertised_window << fSendWindowShift;

	// First, handle the most common case for uni-directional data transfer
	// (known as header prediction - the segment must not change the window,
	// and must be the expected sequence, and contain no control flags)

	if (fState == ESTABLISHED
		&& segment.AcknowledgeOnly()
		&& fReceiveNext == segment.sequence
		&& advertisedWindow > 0 && advertisedWindow == fSendWindow
		&& fSendNext == fSendMax) {

		_UpdateTimestamps(segment, buffer->size, true);

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
				fSendList.Signal();
					// TODO: real conditional locking needed!
				gSocketModule->notify(socket, B_SELECT_WRITE, fSendWindow);

				// if there is data left to be send, send it now
				_SendQueued();
				return DROP;
			}
		} else if (segment.acknowledge == fSendUnacknowledged
			&& fReceiveQueue.IsContiguous()
			&& fReceiveQueue.Free() >= buffer->size
			&& !(fFlags & FLAG_NO_RECEIVE)) {
			_AddData(segment, buffer);
			_NotifyReader();
			return KEEP | ((segment.flags & TCP_FLAG_PUSH) ?
				IMMEDIATE_ACKNOWLEDGE : ACKNOWLEDGE);
		}
	}

	// The fast path was not applicable, so we continue with the standard
	// processing of the incoming segment

	if (fState != SYNCHRONIZE_SENT && fState != LISTEN && fState != CLOSED) {
		// 1. check sequence number
		if (!segment_in_sequence(segment, buffer->size, fReceiveNext,
				fReceiveWindow)) {
			TRACE("  Receive(): segment out of window, next: %lu wnd: %lu",
				(uint32)fReceiveNext, fReceiveWindow);
			if (segment.flags & TCP_FLAG_RESET)
				return DROP;
			return DROP | IMMEDIATE_ACKNOWLEDGE;
		}
	}

	return _Receive(segment, buffer);
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
	segment.urgent_offset = 0;

	if (fOptions & FLAG_OPTION_TIMESTAMP) {
		segment.has_timestamps = true;
		segment.TSecr = fReceivedTSval;
		segment.TSval = 0;
	}

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

	size_t availableBytes = fReceiveQueue.Free();

	if (fOptions & FLAG_OPTION_WINDOW_SCALE)
		segment.advertised_window = availableBytes >> fReceiveWindowShift;
	else
		segment.advertised_window = min_c(TCP_MAX_WINDOW, availableBytes);

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

		AddressModule()->set_to((sockaddr *)&buffer->source, (sockaddr *)&socket->address);
		AddressModule()->set_to((sockaddr *)&buffer->destination, (sockaddr *)&socket->peer);

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
			if (fFlags & FLAG_OPTION_WINDOW_SCALE) {
				segment.has_window_shift = true;
				segment.window_shift = fReceiveWindowShift;
			}
		}

		TRACE("SendQueued() flags %x, buffer %p, size %lu, from address %s to %s",
			segment.flags, buffer, buffer->size,
			AddressString(Domain(), (sockaddr *)&buffer->source, true).Data(),
			AddressString(Domain(), (sockaddr *)&buffer->destination, true).Data());

		status = add_tcp_header(AddressModule(), segment, buffer);
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

		if (segment.flags & TCP_FLAG_ACKNOWLEDGE)
			fLastAcknowledgeSent = segment.acknowledge;

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


status_t
TCPEndpoint::_ShutdownEgress(bool closing)
{
	tcp_state previousState = fState;

	if (fState == SYNCHRONIZE_RECEIVED || fState == ESTABLISHED)
		fState = FINISH_SENT;
	else if (fState == FINISH_RECEIVED)
		fState = WAIT_FOR_FINISH_ACKNOWLEDGE;
	else
		return B_OK;

	status_t status = _SendQueued();
	if (status != B_OK) {
		fState = previousState;
		return status;
	}

	return B_OK;
}


ssize_t
TCPEndpoint::_AvailableData() const
{
	// TODO: Refer to the FLAG_NO_RECEIVE comment above regarding
	//       the application of FLAG_NO_RECEIVE in listen()ing
	//       sockets.
	if (fState == LISTEN)
		return gSocketModule->count_connected(socket);
	else if (fState == SYNCHRONIZE_SENT)
		return 0;

	ssize_t availableData = fReceiveQueue.Available();

	if (availableData == 0 && !_ShouldReceive())
		return ENOTCONN;

	return availableData;
}


void
TCPEndpoint::_NotifyReader()
{
	fReceiveList.Signal();
	gSocketModule->notify(socket, B_SELECT_READ, _AvailableData());
}


bool
TCPEndpoint::_ShouldReceive() const
{
	if (fFlags & FLAG_NO_RECEIVE)
		return false;

	return fState == ESTABLISHED || fState == FINISH_SENT
			|| fState == FINISH_ACKNOWLEDGED;
}


int32
TCPEndpoint::_Receive(tcp_segment_header &segment, net_buffer *buffer)
{
	uint32 advertisedWindow = (uint32)segment.advertised_window << fSendWindowShift;

	size_t segmentLength = buffer->size;

	if (segment.flags & TCP_FLAG_RESET) {
		// is this a valid reset?
		if (fLastAcknowledgeSent <= segment.sequence
			&& tcp_sequence(segment.sequence)
				< (fLastAcknowledgeSent + fReceiveWindow)) {
			if (fState == SYNCHRONIZE_RECEIVED)
				fError = ECONNREFUSED;
			else if (fState == CLOSING || fState == TIME_WAIT
					|| fState == WAIT_FOR_FINISH_ACKNOWLEDGE)
				fError = ENOTCONN;
			else
				fError = ECONNRESET;

			_NotifyReader();
			fState = CLOSED;
		}

		return DROP;
	}

	if ((segment.flags & TCP_FLAG_SYNCHRONIZE) != 0
		|| (fState == SYNCHRONIZE_RECEIVED
			&& (fInitialReceiveSequence > segment.sequence
				|| (segment.flags & TCP_FLAG_ACKNOWLEDGE) != 0
					&& (fSendUnacknowledged > segment.acknowledge
						|| fSendMax < segment.acknowledge)))) {
		// reset the connection - either the initial SYN was faulty, or we
		// received a SYN within the data stream
		return DROP | RESET;
	}

	fReceiveWindow = max_c(fReceiveQueue.Free(), fReceiveWindow);
		// the window must not shrink

	// trim buffer to be within the receive window
	int32 drop = fReceiveNext - segment.sequence;
	if (drop > 0) {
		if ((uint32)drop > buffer->size
			|| ((uint32)drop == buffer->size
				&& (segment.flags & TCP_FLAG_FINISH) == 0)) {
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
			_MarkEstablished();
		}

		if (fSendMax < segment.acknowledge || fState == TIME_WAIT)
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

			if (segment.acknowledge > fSendQueue.LastSequence()
					&& fState > ESTABLISHED) {
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

	if (segment.flags & TCP_FLAG_URGENT) {
		if (fState == ESTABLISHED || fState == FINISH_SENT
			|| fState == FINISH_ACKNOWLEDGED) {
			// TODO: Handle urgent data:
			//  - RCV.UP <- max(RCV.UP, SEG.UP)
			//  - signal the user that urgent data is available (SIGURG)
		}
	}

	bool notify = false;

	if (buffer->size > 0 &&	_ShouldReceive()) {
		_AddData(segment, buffer);
		notify = true;
	} else
		action = (action & ~KEEP) | DROP;

	if (segment.flags & TCP_FLAG_FINISH) {
		segmentLength++;
		if (fState != CLOSED && fState != LISTEN && fState != SYNCHRONIZE_SENT) {
			TRACE("Receive(): peer is finishing connection!");
			fReceiveNext++;
			notify = true;

			// FIN implies PSH
			fReceiveQueue.SetPushPointer();

			// RFC 793 states that we must send an ACK to FIN
			action |= IMMEDIATE_ACKNOWLEDGE;

			// other side is closing connection; change states
			switch (fState) {
				case ESTABLISHED:
				case SYNCHRONIZE_RECEIVED:
					fState = FINISH_RECEIVED;
					break;
				case FINISH_SENT:
					// simultaneous close
					fState = CLOSING;
					break;
				case FINISH_ACKNOWLEDGED:
					fState = TIME_WAIT;
					_EnterTimeWait();
					break;
				default:
					break;
			}
		}
	}

	if (notify)
		_NotifyReader();

	if (buffer->size > 0 || (segment.flags & TCP_FLAG_SYNCHRONIZE) != 0)
		action |= ACKNOWLEDGE;

	_UpdateTimestamps(segment, segmentLength, true);

	TRACE("Receive() Action %ld", action);

	return action;
}


void
TCPEndpoint::_UpdateTimestamps(tcp_segment_header &segment, size_t segmentLength,
	bool checkSequence)
{
	if (segment.has_timestamps)
		fOptions |= FLAG_OPTION_TIMESTAMP;
	else
		fOptions &= ~FLAG_OPTION_TIMESTAMP;

	if (fOptions & FLAG_OPTION_TIMESTAMP) {
		tcp_sequence sequence(segment.sequence);

		if (!checkSequence || (fLastAcknowledgeSent >= sequence
				&& fLastAcknowledgeSent < (sequence + segmentLength)))
			fReceivedTSval = segment.TSval;
	}
}


void
TCPEndpoint::_MarkEstablished()
{
	fState = ESTABLISHED;

	if (socket->parent != NULL) {
		gSocketModule->set_connected(socket);
		release_sem_etc(fAcceptSemaphore, 1, B_DO_NOT_RESCHEDULE);
	}

	fSendList.Signal();
}


status_t
TCPEndpoint::_WaitForEstablished(RecursiveLocker &locker, bigtime_t timeout)
{
	while (fState != ESTABLISHED) {
		status_t status = fSendList.Wait(locker, timeout);
		if (status < B_OK)
			return status;
	}

	return B_OK;
}


void
TCPEndpoint::_AddData(tcp_segment_header &segment, net_buffer *buffer)
{
	fReceiveNext += buffer->size;
	fReceiveQueue.Add(buffer, segment.sequence);

	TRACE("  _AddData(): adding data, receive next = %lu. Now have %lu bytes.",
		(uint32)fReceiveNext, fReceiveQueue.Available());

	if (segment.flags & TCP_FLAG_PUSH)
		fReceiveQueue.SetPushPointer();
}


//	#pragma mark - timer


/*static*/ void
TCPEndpoint::_RetransmitTimer(net_timer *timer, void *data)
{
	TCPEndpoint *endpoint = (TCPEndpoint *)data;

	RecursiveLocker locker(endpoint->fLock);
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

	RecursiveLocker locker(endpoint->fLock);
	if (!locker.IsLocked())
		return;

	endpoint->_SendQueued(true);
}


/*static*/ void
TCPEndpoint::_DelayedAcknowledgeTimer(struct net_timer *timer, void *data)
{
	TCPEndpoint *endpoint = (TCPEndpoint *)data;

	RecursiveLocker locker(endpoint->fLock);
	if (!locker.IsLocked())
		return;

	endpoint->SendAcknowledge();
}


/*static*/ void
TCPEndpoint::_TimeWaitTimer(struct net_timer *timer, void *data)
{
	TCPEndpoint *endpoint = (TCPEndpoint *)data;

	if (recursive_lock_lock(&endpoint->fLock) < B_OK)
		return;

	endpoint->DeleteSocket();
}

