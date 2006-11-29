/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Galante, haiku.galante@gmail.com
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "TCPConnection.h"

#include <net_buffer.h>
#include <net_datalink.h>

#include <KernelExport.h>
#include <util/list.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <new>
#include <stdlib.h>
#include <string.h>

#include <lock.h>
#include <util/AutoLock.h>
#include <util/khash.h>

#include <NetBufferUtilities.h>
#include <NetUtilities.h>


// Things this implementation currently doesn't implement:
// TCP, RFC 793
// TCP Slow Start, Congestion Avoidance, Fast Retransmit, and Fast Recovery, RFC 2001, RFC 2581, RFC 3042
// NewReno Modification to TCP's Fast Recovery, RFC 2582
// Explicit Congestion Notification (ECN), RFC 3168
// SYN-Cache
// TCP Extensions for High Performance, RFC 1323
// SACK, Selective Acknowledgment - RFC 2018, RFC 2883, RFC 3517
// Forward RTO-Recovery, RFC 4138

#define TRACE_TCP
#ifdef TRACE_TCP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif

// Initial estimate for packet round trip time (RTT)
#define	TCP_INITIAL_RTT	120000000LL

// Estimate for Maximum segment lifetime in the internet
#define TCP_MAX_SEGMENT_LIFETIME (2 * TCP_INITIAL_RTT)


TCPConnection::TCPConnection(net_socket *socket)
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
	fError(B_OK)
{
	//gStackModule->init_timer(&fTimer, _TimeWait, this);

	recursive_lock_init(&fLock, "tcp lock");
		// TODO: to be replaced with a real locking strategy!
	//benaphore_init(&fReceiveLock, "tcp receive");
	//benaphore_init(&fSendLock, "tcp send");
	fSendLock = create_sem(0, "tcp send");
	fReceiveLock = create_sem(0, "tcp receive");

	gStackModule->init_timer(&fPersistTimer, TCPConnection::_PersistTimer, this);
	gStackModule->init_timer(&fRetransmitTimer, TCPConnection::_RetransmitTimer, this);
	gStackModule->init_timer(&fDelayedAcknowledgeTimer,
		TCPConnection::_DelayedAcknowledgeTimer, this);
}


TCPConnection::~TCPConnection()
{
	//gStackModule->set_timer(&fTimer, -1);

	recursive_lock_destroy(&fLock);
	//benaphore_destroy(&fReceiveLock);
	//benaphore_destroy(&fSendLock);
	//delete_sem(fAcceptSemaphore);
	delete_sem(fReceiveLock);
	delete_sem(fSendLock);
}


status_t
TCPConnection::InitCheck() const
{
	if (fReceiveLock < B_OK)
		return fReceiveLock;
	if (fSendLock < B_OK)
		return fSendLock;
	//if (fAcceptSemaphore < B_OK)
	//	return fAcceptSemaphore;

	return B_OK;
}


//	#pragma mark - protocol API


status_t
TCPConnection::Open()
{
	TRACE(("%p.Open()\n", this));
	// nothing to do here...
	return B_OK;
}


status_t
TCPConnection::Close()
{
	TRACE(("TCP:%p.Close()\n", this));
	RecursiveLocker lock(fLock);

	if (fState == SYNCHRONIZE_SENT || fState == LISTEN) {
		fState = CLOSED;
		return B_OK;
	}

	tcp_state previousState = fState;

	if (fState == SYNCHRONIZE_RECEIVED || fState == ESTABLISHED)
		fState = FINISH_SENT;
	else if (fState == FINISH_RECEIVED)
		fState = WAIT_FOR_FINISH_ACKNOWLEDGE;
	else
		fState = CLOSED;

	status_t status = _SendQueued();
	if (status != B_OK) {
		fState = previousState;
		return status;
	}

	TRACE(("TCP: %p.Close(): Entering state %d\n", this, fState));
	// TODO: do i need to wait until fState returns to CLOSED?
	return B_OK;
}


status_t
TCPConnection::Free()
{
	TRACE(("TCP:%p.Free()\n", this));

	// TODO: if this connection is not in the hash, we don't have to call this one
	return B_OK;
}


/*!
	Creates and sends a synchronize packet to /a address, and then waits
	until the connection has been established or refused.
*/
status_t
TCPConnection::Connect(const struct sockaddr *address)
{
	TRACE(("TCP:%p.Connect() on address %s\n", this,
		AddressString(gDomain, address, true).Data()));

	if (address->sa_family != AF_INET)
		return EAFNOSUPPORT;

	//BenaphoreLocker lock(&fSendLock);

	TRACE(("  TCP: Connect(): in state %d\n", fState));

	// Can only call connect() from CLOSED or LISTEN states
	// otherwise connection is considered already connected
	if (fState == LISTEN) {
		// this socket is about to connect; remove pending connections in the backlog
		gSocketModule->set_max_backlog(socket, 0);
	} else if (fState != CLOSED)
		return EISCONN;

	// get a net_route if there isn't one
	// TODO: get a net_route_info instead!
	if (fRoute == NULL) {
		fRoute = gDatalinkModule->get_route(gDomain, (sockaddr *)address);
		TRACE(("  TCP: Connect(): Using Route %p\n", fRoute));
		if (fRoute == NULL)
			return ENETUNREACH;
	}

	remove_connection(this);
		// we need to temporarily remove us from the connection list, as we're
		// changing our addresses

	// need to associate this connection with a real address, not INADDR_ANY
	if (gAddressModule->is_empty_address((sockaddr *)&socket->address, false)) {
		TRACE(("  TCP: Connect(): Local Address is INADDR_ANY\n"));
		uint16 port = gAddressModule->get_port((sockaddr *)&socket->address);
		gAddressModule->set_to((sockaddr *)&socket->address,
			(sockaddr *)fRoute->interface->address);
		gAddressModule->set_port((sockaddr *)&socket->address, port);
			// need to reset the port after overwriting the address
	}

	gAddressModule->set_to((sockaddr *)&socket->peer, address);

	// make sure connection does not already exist
	status_t status = insert_connection(this);
	if (status < B_OK) {
		TRACE(("  TCP: Connect(): could not add connection: %s!\n", strerror(status)));
		return status;
	}

	fReceiveMaxSegmentSize = next->module->get_mtu(next, (sockaddr *)address)
		- sizeof(tcp_header);

	// Compute the window shift we advertise to our peer - if it doesn't support
	// this option, this will be reset to 0 (when its SYN is received)
	fReceiveWindowShift = 0;
	while (fReceiveWindowShift < TCP_MAX_WINDOW_SHIFT
		&& (0xffffUL << fReceiveWindowShift) < socket->receive.buffer_size) {
		fReceiveWindowShift++;
	}
dprintf("************************* size = %ld, shift = %d\n", socket->receive.buffer_size, fReceiveWindowShift);

	TRACE(("  TCP: Connect(): starting 3-way handshake...\n"));

	fState = SYNCHRONIZE_SENT;

	// send SYN
	status = _SendQueued();
	if (status != B_OK) {
		fState = CLOSED;
		return status;
	}

	fSendQueue.SetInitialSequence(fSendNext);

	// wait until 3-way handshake is complete (if needed)

	bigtime_t timeout = min_c(socket->send.timeout, TCP_CONNECTION_TIMEOUT);
	if (timeout == 0) {
		// we're a non-blocking socket
		return EINPROGRESS;
	}

	status = acquire_sem_etc(fSendLock, 1, B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, timeout);

	TRACE(("  TCP: Connect(): Connection complete: %s\n", strerror(status)));
	return status;
}


status_t
TCPConnection::Accept(struct net_socket **_acceptedSocket)
{
	TRACE(("TCP:%p.Accept()\n", this));

	// TODO: test for pending sockets
	// TODO: test for non-blocking I/O
	status_t status;
	do {
		status = acquire_sem_etc(fReceiveLock, 1, B_RELATIVE_TIMEOUT,
			socket->receive.timeout);
		if (status < B_OK)
			return status;

		status = gSocketModule->dequeue_connected(socket, _acceptedSocket);
	} while (status < B_OK);

	return status;
}


status_t
TCPConnection::Bind(sockaddr *address)
{
	TRACE(("TCP:%p.Bind() on address %s\n", this,
		AddressString(gDomain, address, true).Data()));

	if (address->sa_family != AF_INET)
		return EAFNOSUPPORT;

	//BenaphoreLocker lock(&fSendLock);
		// TODO: there is no lock yet for these things...

	if (fState != CLOSED)
		return EISCONN;

	// let IP check whether there is an interface that supports the given address:
	status_t status = next->module->bind(next, address);
	if (status < B_OK)
		return status;

	gAddressModule->set_to((sockaddr *)&socket->address, address);

	if (gAddressModule->get_port((sockaddr *)&socket->address) == 0) {
		// assign ephemeral port
		// TODO: use port 40000 and following for now
		static int e = 40000;
		gAddressModule->set_port((sockaddr *)&socket->address, htons(e++));
		status = insert_connection(this);
	} else {
		status = insert_connection(this);
	}

	return status;
}


status_t
TCPConnection::Unbind(struct sockaddr *address)
{
	TRACE(("TCP:%p.Unbind()\n", this ));

	//BenaphoreLocker lock(&fSendLock);
		// TODO: there is no lock yet for these things...

	status_t status = remove_connection(this);
	if (status != B_OK)
		return status;

	gAddressModule->set_to_empty_address((sockaddr *)&socket->address);
	gAddressModule->set_port((sockaddr *)&socket->address, 0);
	return B_OK;
}


status_t
TCPConnection::Listen(int count)
{
	TRACE(("TCP:%p.Listen()\n", this));
	//BenaphoreLocker lock(&fSendLock);
	if (fState != CLOSED)
		return B_BAD_VALUE;

	fState = LISTEN;
	return B_OK;
}


status_t
TCPConnection::Shutdown(int direction)
{
	TRACE(("TCP:%p.Shutdown()\n", this));
	// TODO: implement shutdown!
	return B_ERROR;
}


/*!
	Puts data contained in \a buffer into send buffer
*/
status_t
TCPConnection::SendData(net_buffer *buffer)
{
	TRACE(("TCP:%p.SendData()\n", this));

	RecursiveLocker locker(fLock);
	fSendQueue.Add(buffer);
	return _SendQueued();
}


size_t
TCPConnection::SendAvailable()
{
	TRACE(("TCP:%p.SendAvailable()\n", this));

	RecursiveLocker locker(fLock);
	return fSendQueue.Free();
}


status_t
TCPConnection::ReadData(size_t numBytes, uint32 flags, net_buffer** _buffer)
{
	TRACE(("TCP:%p.ReadData()\n", this));

	//BenaphoreLocker lock(&fReceiveLock);

	if (fState == SYNCHRONIZE_SENT || fState == SYNCHRONIZE_RECEIVED) {
		// we need to wait until the connection becomes established
		if (flags & MSG_DONTWAIT)
			return B_WOULD_BLOCK;

		status_t status = acquire_sem_etc(fSendLock, 1,
			B_RELATIVE_TIMEOUT | B_CAN_INTERRUPT, socket->receive.timeout);
		if (status < B_OK)
			return status;
	}

	// read data out of buffer
	// TODO: add support for urgent data (MSG_OOB)
	// TODO: wait until enough bytes are available

	RecursiveLocker locker(fLock);
	return fReceiveQueue.Get(numBytes, (flags & MSG_PEEK) == 0, _buffer);
}


size_t
TCPConnection::ReadAvailable()
{
	TRACE(("TCP:%p.ReadAvailable()\n", this));

	RecursiveLocker locker(fLock);
	return fReceiveQueue.Available();
}


//	#pragma mark - misc


status_t
TCPConnection::DelayedAcknowledge()
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
TCPConnection::SendAcknowledge()
{
	return _SendQueued(true);
}


void
TCPConnection::_StartPersistTimer()
{
	gStackModule->set_timer(&fPersistTimer, 1000000LL);
}


status_t
TCPConnection::UpdateTimeWait()
{
	return B_OK;
}


//	#pragma mark - receive


int32
TCPConnection::ListenReceive(tcp_segment_header &segment, net_buffer *buffer)
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

	// spawn new connection for accept()
	net_socket *newSocket;
	if (gSocketModule->spawn_pending_socket(socket, &newSocket) < B_OK)
		return DROP;

	gAddressModule->set_to((sockaddr *)&newSocket->address,
		(sockaddr *)&buffer->destination);
	gAddressModule->set_to((sockaddr *)&newSocket->peer,
		(sockaddr *)&buffer->source);

	TCPConnection *connection = (TCPConnection *)newSocket->first_protocol;

	// TODO: proper error handling!

	connection->fRoute = gDatalinkModule->get_route(gDomain,
		(sockaddr *)&newSocket->peer);
	if (connection->fRoute == NULL)
		return DROP;

	if (insert_connection(connection) < B_OK)
		return DROP;

	connection->fInitialReceiveSequence = segment.sequence;
	connection->fState = SYNCHRONIZE_RECEIVED;
	connection->fReceiveMaxSegmentSize = connection->fRoute->mtu - 40;
		// 40 bytes for IP and TCP header without any options
		// TODO: make this depending on the RTF_LOCAL flag?
	connection->fReceiveNext = segment.sequence + 1;
		// account for the extra sequence number for the synchronization

	connection->fSendNext = connection->fInitialSendSequence;
	connection->fSendUnacknowledged = connection->fSendNext;
	connection->fSendMax = connection->fSendNext;

	if (segment.max_segment_size > 0)
		connection->fSendMaxSegmentSize = segment.max_segment_size;

	//benaphore_lock(&connection->fSendLock);
	status_t status = connection->_SendQueued();
	//benaphore_unlock(&connection->fSendLock);

	connection->fInitialSendSequence = fSendNext;
	connection->fSendQueue.SetInitialSequence(fSendNext);

	if (status < B_OK)
		return DROP;

	segment.flags &= ~TCP_FLAG_SYNCHRONIZE;
		// we handled this flag now, it must not be set for further processing

	return connection->Receive(segment, buffer);
}


int32
TCPConnection::SynchronizeSentReceive(tcp_segment_header &segment, net_buffer *buffer)
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

	segment.sequence++;

	fSendUnacknowledged = segment.acknowledge;
	fReceiveNext = segment.sequence;
	fInitialReceiveSequence = segment.sequence;
	fReceiveQueue.SetInitialSequence(fReceiveNext);

	if (segment.flags & TCP_FLAG_ACKNOWLEDGE) {
		// the connection has been established
		fState = ESTABLISHED;

		release_sem_etc(fSendLock, 1, B_DO_NOT_RESCHEDULE);
			// TODO: this is not enough - we need to use B_RELEASE_ALL
	} else {
		// simultaneous open
		fState = SYNCHRONIZE_RECEIVED;
	}

	segment.flags &= ~TCP_FLAG_SYNCHRONIZE;
		// we handled this flag now, it must not be set for further processing

	return Receive(segment, buffer) | IMMEDIATE_ACKNOWLEDGE;
}


int32
TCPConnection::Receive(tcp_segment_header &segment, net_buffer *buffer)
{
	TRACE(("TCP:%p.ReceiveData(): Connection in state %d received packet %p with flags 0x%x, seq %lu, ack %lu!\n",
		this, fState, buffer, segment.flags, segment.sequence, segment.acknowledge));

	// TODO: rethink locking!

	uint32 advertisedWindow = (uint32)segment.advertised_window << fSendWindowShift;

	// First, handle the most common case for uni-directional data transfer
	// (known as header prediction - the segment must not change the window,
	// and must be the expected sequence, and contain no control flags)

	if (fState == ESTABLISHED
		&& segment.AcknowledgeOnly()
		&& advertisedWindow > 0 && advertisedWindow == fSendWindow
		&& fSendNext == fSendMax) {
		if (buffer->size == 0) {
			// this is a pure acknowledge segment - we're on the sending end
			if (fSendUnacknowledged < segment.acknowledge
				&& fSendMax >= segment.acknowledge) {
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
			// we're on the receiving end of the connection, and this segment
			// is the one we were expecting, in-sequence
			fReceiveQueue.Add(buffer, segment.sequence);
			fReceiveNext += buffer->size;

			release_sem_etc(fReceiveLock, 1, B_DO_NOT_RESCHEDULE);
				// TODO: real conditional locking needed!
			gSocketModule->notify(socket, B_SELECT_READ, fReceiveQueue.Available());

			return KEEP | ACKNOWLEDGE;
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
		}

		return DROP;
	}
	if ((segment.flags & TCP_FLAG_SYNCHRONIZE) != 0
		|| (fState == SYNCHRONIZE_RECEIVED
			&& (fSendUnacknowledged > segment.acknowledge
				|| fSendMax < segment.acknowledge
				|| fInitialReceiveSequence > segment.sequence))) {
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
		gBufferModule->remove_trailer(buffer, drop);
	}

	// Then look at the acknowledgement for any updates

	if ((segment.flags & TCP_FLAG_ACKNOWLEDGE) != 0) {
		// process acknowledged data
		if (fState == SYNCHRONIZE_RECEIVED) {
			// TODO: window scaling!
			gSocketModule->set_connected(socket);
			//fReceiveQueue.SetInitialSequence(fReceiveNext);
			fState = ESTABLISHED;
		}
		if (fSendMax < segment.acknowledge)
			return DROP | IMMEDIATE_ACKNOWLEDGE;
		if (fSendUnacknowledged >= segment.acknowledge) {
			// this is a duplicate acknowledge
			// TODO: handle this!
			fDuplicateAcknowledgeCount++;
		} else {
			// this segment acknowleges in flight data
			fDuplicateAcknowledgeCount = 0;

			if (fSendMax == segment.acknowledge) {
				// there is no outstanding data to be acknowledged
				// TODO: if the transmit timer function is already waiting
				//	to acquire this connection's lock, we should stop it anyway
				gStackModule->cancel_timer(&fRetransmitTimer);
			} else {
				// TODO: set retransmit timer correctly
				if (!gStackModule->is_timer_active(&fRetransmitTimer))
					gStackModule->set_timer(&fRetransmitTimer, 1000000LL);
			}

			fSendUnacknowledged = segment.acknowledge;
			fSendQueue.RemoveUntil(segment.acknowledge);

			if (fSendNext < fSendUnacknowledged)
				fSendNext = fSendUnacknowledged;

			if (segment.acknowledge > fSendQueue.LastSequence()) {
				// our TCP_FLAG_FINISH has been acknowledged

				switch (fState) {
					case FINISH_SENT:
						fState = FINISH_ACKNOWLEDGED;
						break;
					case CLOSING:
						fState = TIME_WAIT;
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
		dprintf("peer is finishing connection!");
		fReceiveNext++;

		// other side is closing connection; change states
		switch (fState) {
			case ESTABLISHED:
			case SYNCHRONIZE_RECEIVED:
				fState = FINISH_RECEIVED;
				action |= IMMEDIATE_ACKNOWLEDGE;
				break;
			case FINISH_ACKNOWLEDGED:
				fState = TIME_WAIT;
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

	TRACE(("TCP %p.Receive():Entering state %d, segment action %ld\n", this, fState, action));

	// state machine is done switching states and the data is good.
	// put it in the receive buffer

	if (buffer->size > 0) {
		if (fReceiveNext == segment.sequence)
			fReceiveNext += buffer->size;
		fReceiveQueue.Add(buffer, segment.sequence);
	} else
		gBufferModule->free(buffer);

	return action;
}


//	#pragma mark - send


/*!
	The segment flags to send depend completely on the state we're in.
	_SendQueued() need to be smart enough to clear TCP_FLAG_FINISH when
	it couldn't send all the data.
*/
inline uint8
TCPConnection::_CurrentFlags()
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
TCPConnection::_ShouldSendSegment(tcp_segment_header &segment, uint32 length,
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
TCPConnection::_SendQueued(bool force)
{
	if (fRoute == NULL)
		return B_ERROR;

	// Determine if we need to send anything at all

	tcp_segment_header segment;
	segment.flags = _CurrentFlags();

	uint32 sendWindow = fSendWindow;
	uint32 available = fSendQueue.Available(fSendNext);
	bool outstandingAcknowledge = fSendMax != fSendUnacknowledged;
dprintf("fSendWindow = %lu, available = %lu, fSendNext = %lu, fSendUnacknowledged = %lu\n", fSendWindow, available, (uint32)fSendNext, (uint32)fSendUnacknowledged);

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
dprintf("length = %ld, segmentLength = %lu\n", length, segmentLength);
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
		TRACE(("TCP:%p.SendQueued() flags %u, buffer %p, size %lu, from address %s to %s\n", this,
			segment.flags, buffer, buffer->size,
			AddressString(gDomain, (sockaddr *)&buffer->source, true).Data(),
			AddressString(gDomain, (sockaddr *)&buffer->destination, true).Data()));

		uint32 size = buffer->size;
		if (length > 0 && fSendNext + segmentLength == fSendQueue.LastSequence()) {
			// if we've emptied our send queue, set the PUSH flag
			segment.flags |= TCP_FLAG_PUSH;
		}

		segment.sequence = fSendNext;

		if ((segment.flags & TCP_FLAG_SYNCHRONIZE) != 0) {
			// add connection establishment options
			segment.max_segment_size = fReceiveMaxSegmentSize;
			segment.window_shift = fReceiveWindowShift;
		}

		status = add_tcp_header(segment, buffer);
		if (status != B_OK) {
			gBufferModule->free(buffer);
			return status;
		}

		status = next->module->send_routed_data(next, fRoute, buffer);
		if (status < B_OK) {
			gBufferModule->free(buffer);
			return status;
		}

		// Only count 1 SYN, the 1 sent when transitioning from CLOSED or LISTEN
		if ((segment.flags & TCP_FLAG_SYNCHRONIZE) != 0)
			size++;

		// Only count 1 FIN, the 1 sent when transitioning from
		// ESTABLISHED, SYNCHRONIZE_RECEIVED or FINISH_RECEIVED
		if ((segment.flags & TCP_FLAG_FINISH) != 0)
			size++;

		if (fSendMax == fSendNext)
			fSendMax += size;
		fSendNext += size;

		fReceiveMaxAdvertised = fReceiveNext
			+ ((uint32)segment.advertised_window << fReceiveWindowShift);

		length -= segmentLength;
		if (length == 0)
			break;

		segmentLength = min_c((uint32)length, fSendMaxSegmentSize);
		segment.flags &= ~(TCP_FLAG_SYNCHRONIZE | TCP_FLAG_RESET | TCP_FLAG_FINISH);
	}

	return B_OK;
}


//	#pragma mark - timer


/*static*/ void
TCPConnection::_RetransmitTimer(net_timer *timer, void *data)
{
	TCPConnection *connection = (TCPConnection *)data;

	RecursiveLocker locker(connection->Lock());

	connection->fSendNext = connection->fSendUnacknowledged;
	connection->_SendQueued();
	connection->fSendNext = connection->fSendMax;
}


/*static*/ void
TCPConnection::_PersistTimer(net_timer *timer, void *data)
{
	TCPConnection *connection = (TCPConnection *)data;

	RecursiveLocker locker(connection->Lock());
	connection->_SendQueued(true);
}


/*static*/ void
TCPConnection::_DelayedAcknowledgeTimer(struct net_timer *timer, void *data)
{
	TCPConnection *connection = (TCPConnection *)data;

	RecursiveLocker locker(connection->Lock());
	connection->_SendQueued(true);
}


/*static*/ void
TCPConnection::_TimeWait(struct net_timer *timer, void *data)
{
}


//	#pragma mark - hash functions


int
TCPConnection::Compare(void *_connection, const void *_key)
{
	const tcp_connection_key *key = (tcp_connection_key *)_key;
	TCPConnection *connection = ((TCPConnection *)_connection);

	if (gAddressModule->equal_addresses_and_ports(key->local,
			(sockaddr *)&connection->socket->address)
		&& gAddressModule->equal_addresses_and_ports(key->peer,
			(sockaddr *)&connection->socket->peer))
		return 0;

	return 1;
}


uint32
TCPConnection::Hash(void *_connection, const void *_key, uint32 range)
{
	if (_connection != NULL) {
		TCPConnection *connection = (TCPConnection *)_connection;
		return gAddressModule->hash_address_pair((sockaddr *)&connection->socket->address,
			(sockaddr *)&connection->socket->peer) % range;
	}

	const tcp_connection_key *key = (tcp_connection_key *)_key;
	return gAddressModule->hash_address_pair(key->local, key->peer) % range;
}
