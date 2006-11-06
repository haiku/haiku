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
	fLastByteAckd(0), //system_time()),
	fNextByteToSend(fLastByteAckd),
	fNextByteToWrite(fLastByteAckd + 1),
	fNextByteToRead(0),
	fNextByteExpected(0),
	fLastByteReceived(0),
	fAvgRTT(TCP_INITIAL_RTT),
	fSendBuffer(NULL),
	fReceiveBuffer(NULL),
	fState(CLOSED),
	fError(B_OK),
	fRoute(NULL)
{
	benaphore_init(&fLock, "TCPConnection");
	gStackModule->init_timer(&fTimer, _TimeWait, this);
	list_init(&fReorderQueue);
	list_init(&fWaitQueue);
}


TCPConnection::~TCPConnection()
{
	benaphore_destroy(&fLock);
}


inline bool
TCPConnection::_IsAcknowledgeValid(uint32 acknowledge)
{
	return fLastByteAckd > fNextByteToSend
		? acknowledge >= fLastByteAckd || acknowledge <= fNextByteToSend
		: acknowledge >= fLastByteAckd && acknowledge <= fNextByteToSend;
}


inline bool
TCPConnection::_IsSequenceValid(uint32 sequence, uint32 length)
{
	uint32 end = sequence + length;

	if (fNextByteToRead < fNextByteToRead + TCP_MAX_RECV_BUF) {
		return sequence >= fNextByteToRead
			&& end <= fNextByteToRead + TCP_MAX_RECV_BUF;
	}

	// integer overflow
	return (sequence >= fNextByteToRead
			|| sequence <= fNextByteToRead + TCP_MAX_RECV_BUF)
		&& (end >= fNextByteToRead
			|| end <= fNextByteToRead + TCP_MAX_RECV_BUF);
}


status_t
TCPConnection::Open()
{
	TRACE(("%p.Open()\n", this));
	if (gAddressModule == NULL)
		return B_ERROR;
	TRACE(("TCP: Open(): Using Address Module %p\n", gAddressModule));

	BenaphoreLocker lock(&fLock);
	gAddressModule->set_to_empty_address((sockaddr *)&socket->address);
	gAddressModule->set_port((sockaddr *)&socket->address, 0);
	gAddressModule->set_to_empty_address((sockaddr *)&socket->peer);
	gAddressModule->set_port((sockaddr *)&socket->peer, 0);
	return B_OK;
}


status_t
TCPConnection::Close()
{
	BenaphoreLocker lock(&fLock);
	TRACE(("TCP:%p.Close()\n", this));
	if (fState == SYNCHRONIZE_SENT || fState == LISTEN) {
		fState = CLOSED;
		return B_OK;
	}
	tcp_state nextState = CLOSED;
	if (fState == SYNCHRONIZE_RECEIVED || fState == ESTABLISHED)
		nextState = FINISH_SENT;
	if (fState == FINISH_RECEIVED)
		nextState = WAIT_FOR_FINISH_ACKNOWLEDGE;

	status_t status = _SendQueuedData(TCP_FLAG_FINISH | TCP_FLAG_ACKNOWLEDGE, false);
	if (status != B_OK)
		return status;

	fState = nextState;
	TRACE(("TCP: %p.Close(): Entering state %d\n", this, fState));
	// TODO: do i need to wait until fState returns to CLOSED?
	return B_OK;
}


status_t
TCPConnection::Free()
{
	TRACE(("TCP:%p.Free()\n", this));

	BenaphoreLocker hashLock(&gConnectionLock);
	BenaphoreLocker lock(&fLock);
	tcp_connection_key key;
	key.local = (sockaddr *)&socket->address;
	key.peer = (sockaddr *)&socket->peer;
	if (hash_lookup(gConnectionHash, &key) != NULL) {
		return hash_remove(gConnectionHash, (void *)this);
	}
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

	benaphore_lock(&gConnectionLock);
		// want to release lock later, so no autolock
	BenaphoreLocker lock(&fLock);

	// Can only call Connect from CLOSED or LISTEN states
	// otherwise connection is considered already connected
	if (fState != CLOSED && fState != LISTEN) {
		benaphore_unlock(&gConnectionLock);
		return EISCONN;
	}
	TRACE(("TCP: Connect(): in state %d\n", fState));

	// get a net_route if there isn't one
	// TODO: get a net_route_info instead!
	if (fRoute == NULL) {
		fRoute = gDatalinkModule->get_route(gDomain, (sockaddr *)address);
		TRACE(("TCP: Connect(): Using Route %p\n", fRoute));
		if (fRoute == NULL) {
			benaphore_unlock(&gConnectionLock);
			return ENETUNREACH;
		}
	}

	// need to associate this connection with a real address, not INADDR_ANY
	if (gAddressModule->is_empty_address((sockaddr *)&socket->address, false)) {
		TRACE(("TCP: Connect(): Local Address is INADDR_ANY\n"));
		gAddressModule->set_to((sockaddr *)&socket->address, (sockaddr *)fRoute->interface->address);
		// since most stacks terminate connections from port 0
		// use port 40000 for now.  This should be moved to Bind(), and Bind() called before Connect().
		gAddressModule->set_port((sockaddr *)&socket->address, htons(40000));
	}

	// make sure connection does not already exist
	tcp_connection_key key;
	key.local = (sockaddr *)&socket->address;
	key.peer = address;
	if (hash_lookup(gConnectionHash, &key) != NULL) {
		benaphore_unlock(&gConnectionLock);
		return EADDRINUSE;
	}
	TRACE(("TCP: Connect(): connecting...\n"));

	status_t status;
	gAddressModule->set_to((sockaddr *)&socket->peer, address);
	status = hash_insert(gConnectionHash, (void *)this);
	if (status != B_OK) {
		TRACE(("TCP: Connect(): Error inserting connection into hash!\n"));
		benaphore_unlock(&gConnectionLock);
		return status;
	}

	// done manipulating the hash, release the lock
	benaphore_unlock(&gConnectionLock);

	TRACE(("TCP: Connect(): starting 3-way handshake...\n"));
	// send SYN
	status = _SendQueuedData(TCP_FLAG_SYNCHRONIZE, false);
	if (status != B_OK)
		return status;

	fState = SYNCHRONIZE_SENT;

	// TODO: wait until 3-way handshake is complete
	TRACE(("TCP: Connect(): Connection complete\n"));
	return B_OK;
}


status_t
TCPConnection::Accept(struct net_socket **_acceptedSocket)
{
	TRACE(("TCP:%p.Accept()\n", this));
	return B_ERROR;
}


status_t
TCPConnection::Bind(sockaddr *address)
{
	TRACE(("TCP:%p.Bind() on address %s\n", this,
		AddressString(gDomain, address, true).Data()));

	if (address->sa_family != AF_INET)
		return EAFNOSUPPORT;

	BenaphoreLocker hashLock(&gConnectionLock);
	BenaphoreLocker lock(&fLock);

	// let IP check whether there is an interface that supports the given address:
	status_t status = next->module->bind(next, address);
	if (status < B_OK)
		return status;

	gAddressModule->set_to((sockaddr *)&socket->address, address);
	// for now, leave port=0.  TCP should still work 1 connection at a time
	if (0) { //gAddressModule->get_port((sockaddr *)&socket->address) == 0) {
		// assign ephemeral port
	} else {
		// TODO: Check for Socket flags
		tcp_connection_key key;
		key.peer = (sockaddr *)&socket->peer;
		key.local = (sockaddr *)&socket->address;
		if (hash_lookup(gConnectionHash, &key) == NULL) {
			hash_insert(gConnectionHash, (void *)this);
		} else
			return EADDRINUSE;
	}

	return B_OK;
}


status_t
TCPConnection::Unbind(struct sockaddr *address)
{
	TRACE(("TCP:%p.Unbind()\n", this ));

	BenaphoreLocker hashLock(&gConnectionLock);
	BenaphoreLocker lock(&fLock);

	status_t status = hash_remove(gConnectionHash, (void *)this);
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
	BenaphoreLocker lock(&fLock);
	if (fState != CLOSED)
		return B_ERROR;

	fState = LISTEN;
	return B_OK;
}


status_t
TCPConnection::Shutdown(int direction)
{
	TRACE(("TCP:%p.Shutdown()\n", this));
	return B_ERROR;
}


/*!
	Puts data contained in \a buffer into send buffer
*/
status_t
TCPConnection::SendData(net_buffer *buffer)
{
	TRACE(("TCP:%p.SendData()\n", this));
	size_t bufferSize = buffer->size;
	BenaphoreLocker lock(&fLock);
	if (fSendBuffer != NULL) {
		status_t status = gBufferModule->merge(fSendBuffer, buffer, true);
		if (status != B_OK)
			return status;
	} else
		fSendBuffer = buffer;

	fNextByteToWrite += bufferSize;
	return _SendQueuedData(TCP_FLAG_ACKNOWLEDGE, false);
}


status_t
TCPConnection::SendRoutedData(net_route *route, net_buffer *buffer)
{
	TRACE(("TCP:%p.SendRoutedData()\n", this));
	{
		BenaphoreLocker lock(&fLock);
		fRoute = route;
	}
	return SendData(buffer);
}


size_t
TCPConnection::SendAvailable()
{
	TRACE(("TCP:%p.SendAvailable()\n", this));
	BenaphoreLocker lock(&fLock);
	if (fSendBuffer != NULL)
		return TCP_MAX_SEND_BUF - fSendBuffer->size;

	return TCP_MAX_SEND_BUF;
}


status_t
TCPConnection::ReadData(size_t numBytes, uint32 flags, net_buffer** _buffer)
{
	TRACE(("TCP:%p.ReadData()\n", this));

	BenaphoreLocker lock(&fLock);

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
	BenaphoreLocker lock(&fLock);
	if (fReceiveBuffer != NULL)
		return fReceiveBuffer->size;

	return 0;
}


/*!
	You must hold the connection's lock when calling this method
*/
status_t
TCPConnection::_EnqueueReceivedData(net_buffer *buffer, uint32 sequence)
{
	TRACE(("TCP:%p.EnqueueReceivedData(%p, %lu)\n", this, buffer, sequence));
	status_t status;

	if (sequence == fNextByteExpected) {
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

		fNextByteExpected += buffer->size;
		if (fReceiveBuffer == NULL)
			fReceiveBuffer = buffer;
		else {
			status = gBufferModule->merge(fReceiveBuffer, buffer, true);
			if (status < B_OK) {
				fNextByteExpected -= buffer->size;
				return status;
			}
		}
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
TCPConnection::ReceiveData(net_buffer *buffer)
{
	BenaphoreLocker lock(&fLock);
	TRACE(("TCP:%p.ReceiveData()\n", this));

	NetBufferHeader<tcp_header> bufferHeader(buffer);
	if (bufferHeader.Status() < B_OK)
		return bufferHeader.Status();

	tcp_header &header = bufferHeader.Data();

	uint16 flags = 0x0;
	tcp_state nextState = fState;
	status_t status = B_OK;
	uint32 byteAckd = ntohl(header.acknowledge_num);
	uint32 byteRcvd = ntohl(header.sequence_num);
	uint32 headerLength = (uint32)header.header_length << 2;
	uint32 dataLength = buffer->size - headerLength;

	TRACE(("TCP: ReceiveData(): Connection in state %d received packet %p with flags %X!\n", fState, buffer, header.flags));
	switch (fState) {
		case CLOSED:
		case TIME_WAIT:
			gBufferModule->free(buffer);
			if (header.flags & TCP_FLAG_ACKNOWLEDGE)
				return _Reset(byteAckd, 0);

			return _Reset(0, byteRcvd + dataLength);

		case LISTEN:
			// if packet is SYN, spawn new TCPConnection in SYN_RCVD state
			// and add it to the Connection Queue.  The new TCPConnection
			// must continue the handshake by replying with SYN+ACK.  Any
			// data in the packet must go into the new TCPConnection's receive
			// buffer.
			// Otherwise, RST+ACK is sent.
			// The current TCPConnection always remains in LISTEN state.
			return B_ERROR;

		case SYNCHRONIZE_SENT:
			if (header.flags & TCP_FLAG_RESET) {
				fError = ECONNREFUSED;
				fState = CLOSED;
				return B_ERROR;
			}

			if (header.flags & TCP_FLAG_ACKNOWLEDGE && !_IsAcknowledgeValid(byteAckd))
				return _Reset(byteAckd, 0);

			if (header.flags & TCP_FLAG_SYNCHRONIZE) {
				fNextByteToRead = fNextByteExpected = ntohl(header.sequence_num) + 1;
				flags |= TCP_FLAG_ACKNOWLEDGE;
				fLastByteAckd = byteAckd;

				// cancel resend of this segment
				if (header.flags & TCP_FLAG_ACKNOWLEDGE)
					nextState = ESTABLISHED;
				else {
					nextState = SYNCHRONIZE_RECEIVED;
					flags |= TCP_FLAG_SYNCHRONIZE;
				}
			}
			break;

		case SYNCHRONIZE_RECEIVED:
			if (header.flags & TCP_FLAG_ACKNOWLEDGE && _IsAcknowledgeValid(byteAckd))
				fState = ESTABLISHED;
			else
				_Reset(byteAckd, 0);
			break;

		default:
			// In a synchronized state.
			// first check that the received sequence number is good
			if (_IsSequenceValid(byteRcvd, dataLength)) {
				// If a valid RST was received, terminate the connection.
				if (header.flags & TCP_FLAG_RESET) {
					fError = ECONNREFUSED;
					fState = CLOSED;
					return B_ERROR;
				}

				if (header.flags & TCP_FLAG_ACKNOWLEDGE && _IsAcknowledgeValid(byteAckd)) {
					fLastByteAckd = byteAckd;
					if (fLastByteAckd == fNextByteToWrite) {
						if (fState == WAIT_FOR_FINISH_ACKNOWLEDGE) {
							nextState = CLOSED;
							status = hash_remove(gConnectionHash, this);
							if (status != B_OK)
								return status;
						}
						if (fState == CLOSING) {
							nextState = TIME_WAIT;
							status = hash_remove(gConnectionHash, this);
							if (status != B_OK)
								return status;
						}
						if (fState == FINISH_SENT) {
							nextState = FINISH_ACKNOWLEDGED;
						}
					}
				}
				if (header.flags & TCP_FLAG_FINISH) {
					// other side is closing connection; change states
					switch (fState) {
						case ESTABLISHED:
							nextState = FINISH_RECEIVED;
							fNextByteExpected++;
							break;
						case FINISH_ACKNOWLEDGED:
							nextState = TIME_WAIT;
							fNextByteExpected++;
							break;
						case FINISH_RECEIVED:
							if (fLastByteAckd == fNextByteToWrite) {
								// our FIN has been ACKd: go to TIME_WAIT
								nextState = TIME_WAIT;
								status = hash_remove(gConnectionHash, this);
								if (status != B_OK)
									return status;
								gStackModule->set_timer(&fTimer, TCP_MAX_SEGMENT_LIFETIME);
							} else
								nextState = CLOSING;
							fNextByteExpected++;
							break;
						default:
							break;
					}
				}
				flags |= TCP_FLAG_ACKNOWLEDGE;
			} else {
				// out-of-order packet received.  remind the other side of where we are
				return _SendQueuedData(TCP_FLAG_ACKNOWLEDGE, true);
			}
			break;
	}
	TRACE(("TCP %p.ReceiveData():Entering state %d\n", this, fState));

	// state machine is done switching states and the data is good.
	// put it in the receive buffer
	// TODO: This isn't the most efficient way to do it, and will need to be changed
	//  to deal with Silly Window Syndrome
	bufferHeader.Remove(headerLength);
	if (buffer->size > 0) {
		status = _EnqueueReceivedData(buffer, byteRcvd);
		if (status != B_OK)
			return status;
	} else
		gBufferModule->free(buffer);

	if (fState != CLOSING && fState != WAIT_FOR_FINISH_ACKNOWLEDGE) {
		status = _SendQueuedData(flags, false);
		if (status != B_OK)
			return status;
	}

	fState = nextState;
	return B_OK;
}


status_t
TCPConnection::_Reset(uint32 sequence, uint32 acknowledge)
{
	TRACE(("TCP:%p.Reset()\n", this));
	net_buffer *reply = gBufferModule->create(512);
	if (reply == NULL)
		return B_NO_MEMORY;

	gAddressModule->set_to((sockaddr *)&reply->source, (sockaddr *)&socket->address);
	gAddressModule->set_to((sockaddr *)&reply->destination, (sockaddr *)&socket->peer);

	status_t status = add_tcp_header(reply,
		TCP_FLAG_RESET | (acknowledge == 0 ? 0 : TCP_FLAG_ACKNOWLEDGE),
		sequence, acknowledge, 0);
	if (status != B_OK) {
		gBufferModule->free(reply);
		return status;
	}

	TRACE(("TCP: Reset():Sending RST...\n"));

	status = next->module->send_routed_data(next, fRoute, reply);
	if (status != B_OK) {
		// if sending failed, we stay responsible for the buffer
		gBufferModule->free(reply);
	}

	return status;
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
	the send buffer and \a empty is false, fEffectiveWindow bytes or less of it are sent as well.
	Sequence and Acknowledgement numbers are filled in accordingly.
	The fLock benaphore must be held before calling.
*/
status_t
TCPConnection::_SendQueuedData(uint16 flags, bool empty)
{
	TRACE(("TCP:%p.SendQueuedData(%X,%s)\n", this, flags, empty ? "1" : "0"));

	if (fRoute == NULL)
		return B_ERROR;

	net_buffer *buffer;
	uint32 effectiveWindow = min_c(next->module->get_mtu(next,
		(sockaddr *)&socket->address), fNextByteToWrite - fNextByteToSend);

	if (empty || effectiveWindow == 0 || fSendBuffer == NULL || fSendBuffer->size == 0) {
		buffer = gBufferModule->create(256);
		TRACE(("TCP: Sending Buffer %p\n", buffer));
		if (buffer == NULL)
			return ENOBUFS;
	} else {
		buffer = fSendBuffer;
		if (effectiveWindow == fSendBuffer->size)
			fSendBuffer = NULL;
		else
			fSendBuffer = gBufferModule->split(fSendBuffer, effectiveWindow);
	}

	gAddressModule->set_to((sockaddr *)&buffer->source, (sockaddr *)&socket->address);
	gAddressModule->set_to((sockaddr *)&buffer->destination, (sockaddr *)&socket->peer);
	TRACE(("TCP:%p.SendQueuedData() to address %s\n", this,
		AddressString(gDomain, (sockaddr *)&buffer->destination, true).Data()));
	TRACE(("TCP:%p.SendQueuedData() from address %s\n", this,
		AddressString(gDomain, (sockaddr *)&buffer->source, true).Data()));

	uint16 advertisedWindow = TCP_MAX_RECV_BUF - (fNextByteExpected - fNextByteToRead);
	uint32 size = buffer->size;

	status_t status = add_tcp_header(buffer, flags, fNextByteToSend,
		fNextByteExpected, advertisedWindow);
	if (status != B_OK) {
		gBufferModule->free(buffer);
		return status;
	}

	// Only count 1 SYN, the 1 sent when transitioning from CLOSED or LISTEN
	if ((flags & TCP_FLAG_SYNCHRONIZE) != 0 && (fState == CLOSED || fState == LISTEN))
		fNextByteToSend++;

	// Only count 1 FIN, the 1 sent when transitioning from
	// ESTABLISHED, SYNCHRONIZE_RECEIVED or FINISH_RECEIVED
	if ((flags & TCP_FLAG_FINISH) != 0
		&& (fState == SYNCHRONIZE_RECEIVED || fState == ESTABLISHED || fState == FINISH_RECEIVED))
		fNextByteToSend++;

	fNextByteToSend += size;

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
	TCPConnection *connection= ((TCPConnection *)_connection);

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
		return gAddressModule->hash_address_pair(
			(sockaddr *)&connection->socket->address, (sockaddr *)&connection->socket->peer) % range;
	}

	const tcp_connection_key *key = (tcp_connection_key *)_key;
	return gAddressModule->hash_address_pair( 
			key->local, key->peer) % range;
}
