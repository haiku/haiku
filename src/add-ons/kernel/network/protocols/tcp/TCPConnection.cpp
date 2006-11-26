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
// SYN-Cache
// TCP Extensions for High Performance, RFC 1323
// SACK, Selective Acknowledgment - RFC 2018, RFC 2883

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

// keep maximum buffer sizes < max net_buffer size for now
#define	TCP_MAX_SEND_BUF 1024
#define TCP_MAX_RECV_BUF TCP_MAX_SEND_BUF

struct tcp_segment {
	struct list_link link;

	net_buffer	*buffer;
	bigtime_t	time;
	uint32		sequence;
	net_timer	timer;
	bool		timed_out;

	tcp_segment(net_buffer *buffer, uint32 sequenceNumber, bigtime_t timeout);
	~tcp_segment();
};


tcp_segment::tcp_segment(net_buffer *_buffer, uint32 sequenceNumber, bigtime_t timeout)
	:
	buffer(_buffer),
	time(system_time()),
	sequence(sequenceNumber),
	timed_out(false)
{
	if (timeout > 0) {
		gStackModule->init_timer(&timer, &TCPConnection::ResendSegment, this);
		gStackModule->set_timer(&timer, timeout);
	}
}


tcp_segment::~tcp_segment()
{
	gStackModule->set_timer(&timer, -1);
}


//	#pragma mark -


TCPConnection::TCPConnection(net_socket *socket)
	:
	fSendWindowShift(0),
	fReceiveWindowShift(0),
	fSendUnacknowledged(0), //system_time()),
	fSendNext(fSendUnacknowledged),
	fSendWindow(0),
	fSendQueue(socket->send.buffer_size),
	fRoute(NULL),
	fReceiveNext(0),
	fReceiveWindow(socket->receive.buffer_size),
	fReceiveQueue(socket->receive.buffer_size),
	fRoundTripTime(TCP_INITIAL_RTT),
	fState(CLOSED),
	fError(B_OK)
{
	gStackModule->init_timer(&fTimer, _TimeWait, this);

	//benaphore_init(&fReceiveLock, "tcp receive");
	//benaphore_init(&fSendLock, "tcp send");
	fSendLock = create_sem(0, "tcp send");
	fReceiveLock = create_sem(0, "tcp receive");
}


TCPConnection::~TCPConnection()
{
	gStackModule->set_timer(&fTimer, -1);

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


inline bool
TCPConnection::_IsAcknowledgeValid(tcp_sequence acknowledge) const
{
	return acknowledge > fSendUnacknowledged
		&& acknowledge <= fSendMax;
}


/*!
	If this method returns \c true, the sequence contains valid data with
	regard to the receive window.
*/
inline bool
TCPConnection::_IsSequenceValid(tcp_sequence sequence, uint32 length) const
{
	tcp_sequence end = tcp_sequence(sequence + length - 1);
	return (sequence >= fReceiveNext && sequence < fReceiveWindow + fReceiveWindow)
		|| (end >= fReceiveNext && end <= fReceiveNext + fReceiveWindow);
}


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
	//BenaphoreLocker lock(&fSendLock);
	TRACE(("TCP:%p.Close()\n", this));
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

	status_t status = _SendQueuedData(TCP_FLAG_FINISH | TCP_FLAG_ACKNOWLEDGE, false);
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
	remove_connection(this);
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

	fMaxReceiveSize = next->module->get_mtu(next, (sockaddr *)address)
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
	status = _SendQueuedData(TCP_FLAG_SYNCHRONIZE, false);
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

	fSendQueue.Add(buffer);

	return _SendQueuedData(TCP_FLAG_ACKNOWLEDGE, false);
}


size_t
TCPConnection::SendAvailable()
{
	TRACE(("TCP:%p.SendAvailable()\n", this));

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

	return fReceiveQueue.Get(numBytes, (flags & MSG_PEEK) == 0, _buffer);
}


size_t
TCPConnection::ReadAvailable()
{
	TRACE(("TCP:%p.ReadAvailable()\n", this));
	//BenaphoreLocker lock(&fReceiveLock);
	return fReceiveQueue.Available();
}


status_t
TCPConnection::DelayedAcknowledge()
{
	// TODO: use timer instead and/or piggyback on send
	return _SendQueuedData(TCP_FLAG_ACKNOWLEDGE, false);
}


status_t
TCPConnection::SendAcknowledge()
{
	return _SendQueuedData(TCP_FLAG_ACKNOWLEDGE, false);
}


status_t
TCPConnection::UpdateTimeWait()
{
	return B_OK;
}


int32
TCPConnection::ListenReceive(tcp_segment_header& segment, net_buffer *buffer)
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

	connection->fState = SYNCHRONIZE_RECEIVED;
	connection->fMaxReceiveSize = connection->fRoute->mtu - 40;
		// 40 bytes for IP and TCP header without any options
		// TODO: make this depending on the RTF_LOCAL flag?
	connection->fReceiveNext = segment.sequence + 1;
		// account for the extra sequence number for the synchronization

	if (segment.max_segment_size > 0)
		connection->fMaxSegmentSize = segment.max_segment_size;

	//benaphore_lock(&connection->fSendLock);
	status_t status = connection->_SendQueuedData(
		TCP_FLAG_SYNCHRONIZE | TCP_FLAG_ACKNOWLEDGE, false);
	//benaphore_unlock(&connection->fSendLock);

	fSendQueue.SetInitialSequence(fSendNext);

	if (status < B_OK)
		return DROP;

	return connection->Receive(segment, buffer);
}


int32
TCPConnection::SynchronizeSentReceive(tcp_segment_header& segment, net_buffer *buffer)
{
	if ((segment.flags & TCP_FLAG_ACKNOWLEDGE) != 0
		&& segment.acknowledge != fSendNext)
		return DROP | RESET;

	if (segment.flags & TCP_FLAG_RESET) {
		fError = ECONNREFUSED;
		fState = CLOSED;
		return DROP;
	}

	if ((segment.flags & TCP_FLAG_SYNCHRONIZE) == 0)
		return DROP;

	segment.sequence++;

	fSendUnacknowledged = segment.acknowledge + 1;
	fReceiveNext = segment.sequence;

	if (segment.flags & TCP_FLAG_ACKNOWLEDGE) {
		// the connection has been established
		fState = ESTABLISHED;
		fReceiveQueue.SetInitialSequence(fReceiveNext);

		release_sem_etc(fSendLock, 1, B_DO_NOT_RESCHEDULE);
			// TODO: this is not enough - we need to use B_RELEASE_ALL
	} else {
		// simultaneous open
		fState = SYNCHRONIZE_RECEIVED;
	}

	return Receive(segment, buffer) | IMMEDIATE_ACKNOWLEDGE;
}


int32
TCPConnection::Receive(tcp_segment_header& segment, net_buffer *buffer)
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

				// notify threads waiting on the socket to become writable again
				release_sem_etc(fSendLock, 1, B_DO_NOT_RESCHEDULE);
					// TODO: real conditional locking needed!
				gSocketModule->notify(socket, B_SELECT_WRITE, fSendWindow);

				// if there is data left to be send, send it now
				return _SendQueuedData(TCP_FLAG_ACKNOWLEDGE, false);
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
			remove_connection(this);
		}

		return DROP;
	}
	if ((segment.flags & TCP_FLAG_SYNCHRONIZE) != 0
		|| (fState == SYNCHRONIZE_RECEIVED && !_IsAcknowledgeValid(segment.acknowledge))) {
		// this is not allowed here, so we can reset the connection
		return DROP | RESET;
	}

	fReceiveWindow = max_c(fReceiveQueue.Free(), fReceiveWindow);
		// the window must not shrink

	// trim buffer to be within the receive window
	int32 drop = fReceiveNext - segment.sequence;;
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
		if (!_IsAcknowledgeValid(segment.acknowledge)) {
			if (fSendUnacknowledged >= segment.acknowledge) {
				
			}
		}
	}

	if ((segment.flags & TCP_FLAG_RESET) != 0)
		return DROP;

	status_t status;

	if ((segment.flags & TCP_FLAG_ACKNOWLEDGE) != 0
		&& _IsAcknowledgeValid(segment.acknowledge)) {
		fSendUnacknowledged = segment.acknowledge;
		if (fSendUnacknowledged == fSendNext) {
			// there is no data waiting to be processed
			if (fState == WAIT_FOR_FINISH_ACKNOWLEDGE) {
				fState = CLOSED;
				status = remove_connection(this);
				if (status != B_OK)
					return DROP;
			} else if (fState == CLOSING) {
				fState = TIME_WAIT;
				status = remove_connection(this);
				if (status != B_OK)
					return DROP;
			} else if (fState == FINISH_SENT)
				fState = FINISH_ACKNOWLEDGED;
			else if (fState == SYNCHRONIZE_RECEIVED) {
				fState = ESTABLISHED;
				fReceiveQueue.SetInitialSequence(fReceiveNext);
			}
		}

		fSendWindow = advertisedWindow;

		if (segment.flags & TCP_FLAG_FINISH) {
			dprintf("peer is finishing connection!");
			fReceiveNext++;

			// other side is closing connection; change states
			switch (fState) {
				case ESTABLISHED:
					fState = FINISH_RECEIVED;
					action |= IMMEDIATE_ACKNOWLEDGE;
					break;
				case FINISH_ACKNOWLEDGED:
					fState = TIME_WAIT;
					break;
				case FINISH_RECEIVED:
					if (fSendUnacknowledged == fSendNext) {
						fState = TIME_WAIT;
						status = remove_connection(this);
						if (status != B_OK)
							return DROP;

						//gStackModule->set_timer(&fTimer, TCP_MAX_SEGMENT_LIFETIME);
					} else
						fState = CLOSING;
					break;

				default:
					break;
			}
		}
		if (buffer->size > 0 || (segment.flags & (TCP_FLAG_SYNCHRONIZE | TCP_FLAG_FINISH)) != 0)
			action |= ACKNOWLEDGE;
	} else {
		// out-of-order packet received, remind the other side of where we are
		return DROP | ACKNOWLEDGE;
	}

	TRACE(("TCP %p.Receive():Entering state %d\n", this, fState));

	// state machine is done switching states and the data is good.
	// put it in the receive buffer

	if (buffer->size > 0)
		fReceiveQueue.Add(buffer, segment.sequence);
	else
		gBufferModule->free(buffer);

	fReceiveNext = fReceiveQueue.NextSequence();
	fReceiveWindow = fReceiveQueue.Free();

	return action;
}


/*!
 	Resends a sent segment (\a data) if the segment's ACK wasn't received
	before the timeout (eg \a timer expired)
*/
void
TCPConnection::ResendSegment(struct net_timer *timer, void *data)
{
	TRACE(("TCP:ResendSegment(%p)\n", data));
	if (data == NULL)
		return;

	// TODO: implement me!
}


/*!
	Sends a TCP packet with the specified \a flags.  If there is any data in
	the send buffer and \a empty is false, fEffectiveWindow bytes or less of
	it are sent as well.
	Sequence and Acknowledgement numbers are filled in accordingly.
	The send lock must be held before calling.
*/
status_t
TCPConnection::_SendQueuedData(uint16 flags, bool empty)
{
	TRACE(("TCP:%p.SendQueuedData(%X,%s)\n", this, flags, empty ? "1" : "0"));

	if (fRoute == NULL)
		return B_ERROR;

	uint32 effectiveWindow = min_c(next->module->get_mtu(next,
		(sockaddr *)&socket->address), fSendWindow);
	uint32 available = fSendQueue.Available(fSendNext);
dprintf("fSendWindow = %lu, available = %lu\n", fSendWindow, available);

	if (effectiveWindow > available)
		effectiveWindow = available;

	if (effectiveWindow == 0 && flags == 0)
		return B_OK;

	// TODO: determine if we should send anything at all!

	net_buffer *buffer = gBufferModule->create(256);
	if (buffer == NULL)
		return B_NO_MEMORY;

	status_t status = B_OK;
	if (effectiveWindow > 0)
		fSendQueue.Get(buffer, fSendNext, effectiveWindow);
	if (status < B_OK) {
		gBufferModule->free(buffer);
		return status;
	}

	gAddressModule->set_to((sockaddr *)&buffer->source, (sockaddr *)&socket->address);
	gAddressModule->set_to((sockaddr *)&buffer->destination, (sockaddr *)&socket->peer);
	TRACE(("TCP:%p.SendQueuedData() buffer %p, from address %s to %s\n", this,
		buffer,
		AddressString(gDomain, (sockaddr *)&buffer->destination, true).Data(),
		AddressString(gDomain, (sockaddr *)&buffer->source, true).Data()));

	uint32 size = buffer->size;

	tcp_segment_header segment;
	segment.flags = (uint8)flags;
	segment.sequence = fSendNext;
	segment.acknowledge = fReceiveNext;
	segment.advertised_window = min_c(65535, fReceiveWindow);
		// TODO: support shift option!
	segment.urgent_offset = 0;

	if ((flags & TCP_FLAG_SYNCHRONIZE) != 0) {
		// add connection establishment options
		segment.max_segment_size = fMaxReceiveSize;
		//segment.window_shift = fReceiveWindowShift;
	}

	status = add_tcp_header(segment, buffer);
	if (status != B_OK) {
		gBufferModule->free(buffer);
		return status;
	}

	// Only count 1 SYN, the 1 sent when transitioning from CLOSED or LISTEN
	if ((flags & TCP_FLAG_SYNCHRONIZE) != 0)
		fSendNext++;

	// Only count 1 FIN, the 1 sent when transitioning from
	// ESTABLISHED, SYNCHRONIZE_RECEIVED or FINISH_RECEIVED
	if ((flags & TCP_FLAG_FINISH) != 0)
		fSendNext++;

	fSendNext += size;

#if 0
	tcp_segment *segment = new(std::nothrow)
		tcp_segment(sequenceNum, 0, 2*fAvgRTT);
#endif

	return next->module->send_routed_data(next, fRoute, buffer);
}


void
TCPConnection::_TimeWait(struct net_timer *timer, void *data)
{
}


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
