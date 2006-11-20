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
	fLastAcknowledged(0), //system_time()),
	fSendNext(fLastAcknowledged),
	fSendWindow(0),
	fSendBuffer(NULL),
	fRoute(NULL),
	fReceiveNext(0),
	fReceiveWindow(32768),
	fAvgRTT(TCP_INITIAL_RTT),
	fReceiveBuffer(NULL),
	fState(CLOSED),
	fError(B_OK)
{
	gStackModule->init_timer(&fTimer, _TimeWait, this);
	list_init(&fReorderQueue);
	list_init(&fWaitQueue);

	benaphore_init(&fReceiveLock, "tcp receive");
	benaphore_init(&fSendLock, "tcp send");
	fAcceptSemaphore = create_sem(0, "tcp accept");
}


TCPConnection::~TCPConnection()
{
	gStackModule->set_timer(&fTimer, -1);

	benaphore_destroy(&fReceiveLock);
	benaphore_destroy(&fSendLock);
	delete_sem(fAcceptSemaphore);
}


status_t
TCPConnection::InitCheck() const
{
	if (fReceiveLock.sem < B_OK)
		return fReceiveLock.sem;
	if (fSendLock.sem < B_OK)
		return fSendLock.sem;
	if (fAcceptSemaphore < B_OK)
		return fAcceptSemaphore;

	return B_OK;
}


inline bool
TCPConnection::_IsAcknowledgeValid(uint32 acknowledge)
{
	dprintf("last byte ackd %lu, next byte to send %lu, ack %lu\n", fLastAcknowledged, fSendNext, acknowledge);
	return fLastAcknowledged > fSendNext
		? acknowledge >= fLastAcknowledged || acknowledge <= fSendNext
		: acknowledge >= fLastAcknowledged && acknowledge <= fSendNext;
}


inline bool
TCPConnection::_IsSequenceValid(uint32 sequence, uint32 length)
{
	uint32 end = sequence + length;

	if (fReceiveNext < fReceiveNext + TCP_MAX_RECV_BUF) {
		return sequence >= fReceiveNext
			&& end <= fReceiveNext + TCP_MAX_RECV_BUF;
	}

	// integer overflow
	return (sequence >= fReceiveNext
			|| sequence <= fReceiveNext + TCP_MAX_RECV_BUF)
		&& (end >= fReceiveNext
			|| end <= fReceiveNext + TCP_MAX_RECV_BUF);
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
	BenaphoreLocker lock(&fSendLock);
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

	BenaphoreLocker lock(&fSendLock);

	// Can only call connect() from CLOSED or LISTEN states
	// otherwise connection is considered already connected
	if (fState == LISTEN) {
		// this socket is about to connect; remove pending connections in the backlog
		gSocketModule->set_max_backlog(socket, 0);
	} else if (fState != CLOSED)
		return EISCONN;

	TRACE(("  TCP: Connect(): in state %d\n", fState));

	// get a net_route if there isn't one
	// TODO: get a net_route_info instead!
	if (fRoute == NULL) {
		fRoute = gDatalinkModule->get_route(gDomain, (sockaddr *)address);
		TRACE(("  TCP: Connect(): Using Route %p\n", fRoute));
		if (fRoute == NULL)
			return ENETUNREACH;
	}

	// need to associate this connection with a real address, not INADDR_ANY
	if (gAddressModule->is_empty_address((sockaddr *)&socket->address, false)) {
		TRACE(("  TCP: Connect(): Local Address is INADDR_ANY\n"));
		gAddressModule->set_to((sockaddr *)&socket->address, (sockaddr *)fRoute->interface->address);
	}

	gAddressModule->set_to((sockaddr *)&socket->peer, address);

	// make sure connection does not already exist
	status_t status = insert_connection(this);
	if (status < B_OK) {
		TRACE(("  TCP: Connect(): could not add connection: %s!\n", strerror(status)));
		return status;
	}

	TRACE(("  TCP: Connect(): starting 3-way handshake...\n"));

	fState = SYNCHRONIZE_SENT;
	fMaxReceiveSize = fRoute->mtu - 40;

	// send SYN
	status = _SendQueuedData(TCP_FLAG_SYNCHRONIZE, false);
	if (status != B_OK) {
		fState = CLOSED;
		return status;
	}

	// TODO: wait until 3-way handshake is complete
	TRACE(("  TCP: Connect(): Connection complete\n"));
	return B_OK;
}


status_t
TCPConnection::Accept(struct net_socket **_acceptedSocket)
{
	TRACE(("TCP:%p.Accept()\n", this));

	// TODO: test for pending sockets
	// TODO: test for non-blocking I/O
	status_t status;
	do {
		status = acquire_sem(fAcceptSemaphore);
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

	BenaphoreLocker lock(&fSendLock);
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

	BenaphoreLocker lock(&fSendLock);
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
	BenaphoreLocker lock(&fSendLock);
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

	BenaphoreLocker lock(&fSendLock);
	if (fSendBuffer != NULL) {
		status_t status = gBufferModule->merge(fSendBuffer, buffer, true);
		if (status != B_OK)
			return status;
	} else
		fSendBuffer = buffer;

	return _SendQueuedData(TCP_FLAG_ACKNOWLEDGE, false);
}


size_t
TCPConnection::SendAvailable()
{
	TRACE(("TCP:%p.SendAvailable()\n", this));
	BenaphoreLocker lock(&fSendLock);
	if (fSendBuffer != NULL)
		return TCP_MAX_SEND_BUF - fSendBuffer->size;

	return TCP_MAX_SEND_BUF;
}


status_t
TCPConnection::ReadData(size_t numBytes, uint32 flags, net_buffer** _buffer)
{
	TRACE(("TCP:%p.ReadData()\n", this));

	BenaphoreLocker lock(&fReceiveLock);

	// must be in a synchronous state
	if (fState != ESTABLISHED || fState != FINISH_SENT || fState != FINISH_ACKNOWLEDGED) {
		// TODO: is this correct semantics?
dprintf("  TCP state = %d\n", fState);
		return B_ERROR;
	}

dprintf("  TCP error = %ld\n", fError);
	if (fError != B_OK)
		return fError;

	if (fReceiveBuffer->size < numBytes)
		numBytes = fReceiveBuffer->size;

	*_buffer = gBufferModule->split(fReceiveBuffer, numBytes);
	if (*_buffer == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


size_t
TCPConnection::ReadAvailable()
{
	TRACE(("TCP:%p.ReadAvailable()\n", this));
	BenaphoreLocker lock(&fReceiveLock);
	if (fReceiveBuffer != NULL)
		return fReceiveBuffer->size;

	return 0;
}


/*!
	You must hold the connection's receive lock when calling this method
*/
status_t
TCPConnection::_EnqueueReceivedData(net_buffer *buffer, uint32 sequence)
{
	TRACE(("TCP:%p.EnqueueReceivedData(%p, %lu)\n", this, buffer, sequence));
	status_t status;

	if (sequence == fReceiveNext) {
		// first check if the received buffer meets up with the first
		// segment in the ReorderQueue
		tcp_segment *next;
		while ((next = (tcp_segment *)list_get_first_item(&fReorderQueue)) != NULL) {
			if (sequence + buffer->size >= next->sequence) {
				if (sequence + buffer->size > next->sequence) {
					status = gBufferModule->trim(buffer, sequence - next->sequence);
					if (status != B_OK)
						return status;
				}
				status = gBufferModule->merge(buffer, next->buffer, true);
				if (status != B_OK)
					return status;
				list_remove_item(&fReorderQueue, next);
				delete next;
			} else
				break;
		}

		fReceiveNext += buffer->size;

		if (fReceiveBuffer != NULL) {
			status = gBufferModule->merge(fReceiveBuffer, buffer, true);
			if (status < B_OK) {
				fReceiveNext -= buffer->size;
				return status;
			}
		} else
			fReceiveBuffer = buffer;
	} else {
		// add this buffer into the ReorderQueue in the correct place
		// creating a new tcp_segment if necessary
		tcp_segment *next = NULL;
		do {
			next = (tcp_segment *)list_get_next_item(&fReorderQueue, next);
			if (next != NULL && next->sequence < sequence)
				continue;
			if (next != NULL && sequence + buffer->size >= next->sequence) {
				// merge the new buffer with the next buffer
				if (sequence + buffer->size > next->sequence) {
					status = gBufferModule->trim(buffer, sequence - next->sequence);
					if (status != B_OK)
						return status;
				}
				status = gBufferModule->merge(buffer, next->buffer, true);
				if (status != B_OK)
					return status;

				next->buffer = buffer;
				next->sequence = sequence;
				break;
			}
			tcp_segment *segment = new(std::nothrow) tcp_segment(buffer, sequence, -1);
			if (segment == NULL)
				return B_NO_MEMORY;

			if (next == NULL)
				list_add_item(&fReorderQueue, segment);
			else
				list_insert_item_before(&fReorderQueue, next, segment);
		} while (next != NULL);
	}

	return B_OK;
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
		(sockaddr *)&newSocket->address);
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

	benaphore_lock(&connection->fSendLock);
	status_t status = connection->_SendQueuedData(
		TCP_FLAG_SYNCHRONIZE | TCP_FLAG_ACKNOWLEDGE, false);
	benaphore_unlock(&connection->fSendLock);

	if (status < B_OK)
		return DROP;

	return connection->Receive(segment, buffer);
}


int32
TCPConnection::SynchronizeSentReceive(tcp_segment_header& segment, net_buffer *buffer)
{
	if (segment.flags & TCP_FLAG_RESET) {
		fError = ECONNREFUSED;
		fState = CLOSED;
		return DROP;
	}

	if ((segment.flags & TCP_FLAG_ACKNOWLEDGE) != 0
		&& !_IsAcknowledgeValid(segment.acknowledge))
		return DROP | RESET;

	if ((segment.flags & TCP_FLAG_SYNCHRONIZE) == 0)
		return DROP;

	fReceiveNext = segment.sequence + 1;
	fLastAcknowledged = segment.acknowledge;

	if (segment.flags & TCP_FLAG_ACKNOWLEDGE) {
		// the connection has been established
		fState = ESTABLISHED;
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

	// first check that the received sequence number is good
	if (_IsSequenceValid(segment.sequence, buffer->size)) {
		// If a valid RST was received, terminate the connection.
		if (segment.flags & TCP_FLAG_RESET) {
			fError = ECONNREFUSED;
			fState = CLOSED;
			return DROP;
		}
	}

	int32 action = KEEP;
	status_t status;

	if ((segment.flags & TCP_FLAG_ACKNOWLEDGE) != 0
		&& _IsAcknowledgeValid(segment.acknowledge)) {
		fLastAcknowledged = segment.acknowledge;
		if (fLastAcknowledged == fSendNext) {
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
			else if (fState == SYNCHRONIZE_RECEIVED)
				fState = ESTABLISHED;
		}

		fSendWindow = segment.advertised_window;

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
					if (fLastAcknowledged == fSendNext) {
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
	// TODO: This isn't the most efficient way to do it, and will need to be changed
	//  to deal with Silly Window Syndrome

	if (buffer->size > 0) {
		status = _EnqueueReceivedData(buffer, segment.sequence);
		if (status != B_OK)
			return DROP;
	} else
		gBufferModule->free(buffer);

	if (fState != CLOSING && fState != WAIT_FOR_FINISH_ACKNOWLEDGE)
		action &= ACKNOWLEDGE;

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

	net_buffer *buffer;
	uint32 effectiveWindow = min_c(next->module->get_mtu(next,
		(sockaddr *)&socket->address), fSendWindow);

	if (empty || effectiveWindow == 0 || fSendBuffer == NULL || fSendBuffer->size == 0) {
		if (flags == 0) {
			// there is just nothing left to do
			return B_OK;
		}

		buffer = gBufferModule->create(256);
		if (buffer == NULL)
			return ENOBUFS;
	} else {
		if (effectiveWindow == fSendBuffer->size) {
			buffer = fSendBuffer;
			fSendBuffer = NULL;
		} else
			buffer = gBufferModule->split(fSendBuffer, effectiveWindow);
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
	segment.advertised_window = fReceiveWindow;
	segment.urgent_offset = 0;

	if ((flags & TCP_FLAG_SYNCHRONIZE) != 0) {
		// add connection establishment options
		segment.max_segment_size = fMaxReceiveSize;
	}

	status_t status = add_tcp_header(segment, buffer);
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
