/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Galante, haiku.galante@gmail.com
 */

// Initial estimate for packet round trip time (RTT)
#define	TCP_INITIAL_RTT	120000000LL

// Estimate for Maximum segment lifetime in the internet
#define TCP_MAX_SEGMENT_LIFETIME (2 * TCP_INITIAL_RTT)

// keep maximum buffer sizes < max net_buffer size for now
#define	TCP_MAX_SEND_BUF 1024
#define TCP_MAX_RECV_BUF TCP_MAX_SEND_BUF

#define TCP_IS_GOOD_ACK(x) (fLastByteAckd > fNextByteToSend ? \
	((x) >= fLastByteAckd || (x) <= fNextByteToSend) : \
	((x) >= fLastByteAckd && (x) <= fNextByteToSend))

#define TCP_IS_GOOD_SEQ(x,y) (fNextByteToRead < fNextByteToRead + TCP_MAX_RECV_BUF ? \
	(x) >= fNextByteToRead && (x) + (y) <= fNextByteToRead + TCP_MAX_RECV_BUF : \
	((x) >= fNextByteToRead || (x) <= fNextByteToRead + TCP_MAX_RECV_BUF) && \
	((x) + (y) >= fNextByteToRead || (x) + (y) <= fNextByteToRead + TCP_MAX_RECV_BUF))

typedef struct {
	const sockaddr	*local;
	const sockaddr	*peer;
} tcp_connection_key;


class TCPConnection : public net_protocol {
public:
	TCPConnection(net_socket *socket);
	~TCPConnection();

	status_t Open();
	status_t Close();
	status_t Free();
	status_t Connect(const struct sockaddr *address);
	status_t Accept(struct net_socket **_acceptedSocket);
	status_t Bind(struct sockaddr *address);
	status_t Unbind(struct sockaddr *address);
	status_t Listen(int count);
	status_t Shutdown(int direction);
	status_t SendData(net_buffer *buffer);
	status_t SendRoutedData(net_route *route, net_buffer *buffer);
	size_t SendAvailable();
	status_t ReadData(size_t numBytes, uint32 flags, net_buffer **_buffer);
	size_t ReadAvailable();

	status_t ReceiveData(net_buffer *buffer);

	static void ResendSegment(struct net_timer *timer, void *data);
	static int Compare(void *_packet, const void *_key);
	static uint32 Hash(void *_packet, const void *_key, uint32 range);
	static int32 HashOffset() { return offsetof(TCPConnection, fHashLink); }
private:

	class TCPSegment {
	public:

		struct list_link link;

		TCPSegment(net_buffer *buffer, uint32 sequenceNumber, bigtime_t timeout);
		~TCPSegment();

		net_buffer	*fBuffer;
		bigtime_t	fTime;
		uint32		fSequenceNumber;
		net_timer	fTimer;
		bool		fTimedOut;
	};

	status_t SendQueuedData(uint16 flags, bool empty);
	status_t EnqueueReceivedData(net_buffer *buffer, uint32 sequenceNumber);
	status_t Reset(uint32 sequenceNum, uint32 acknowledgeNum);
	static void TimeWait(struct net_timer *timer, void *data);

	uint32	fLastByteAckd;
	uint32	fNextByteToSend;
	uint32	fNextByteToWrite;

	uint32	fNextByteToRead;
	uint32	fNextByteExpected;
	uint32	fLastByteReceived;

	bigtime_t	fAvgRTT;

	net_buffer	*fSendBuffer;
	net_buffer	*fReceiveBuffer;

	struct list fReorderQueue;
	struct list fWaitQueue;

	TCPConnection	*fHashLink;
	tcp_state		fState;
	status_t		fError;
	benaphore		fLock;
	net_timer		fTimer;

	net_route 	*fRoute;
};


TCPConnection::TCPSegment::TCPSegment(net_buffer *buffer, uint32 sequenceNumber, bigtime_t timeout)
	:
	fBuffer(buffer),
	fTime(system_time()),
	fSequenceNumber(sequenceNumber),
	fTimedOut(false)
{
	if (timeout > 0) {
		sStackModule->init_timer(&fTimer, &TCPConnection::ResendSegment, this);
		sStackModule->set_timer(&fTimer, timeout);
	}
}


TCPConnection::TCPSegment::~TCPSegment()
{
	sStackModule->set_timer(&fTimer, -1);
}


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
	sStackModule->init_timer(&fTimer, TimeWait, this);
	list_init(&fReorderQueue);
	list_init(&fWaitQueue);
}


TCPConnection::~TCPConnection()
{
	benaphore_destroy(&fLock);
}


status_t
TCPConnection::Open()
{
	TRACE(("%p.Open()\n", this));
	if (sAddressModule == NULL)
		return B_ERROR;
	TRACE(("TCP: Open(): Using Address Module %p\n", sAddressModule));

	BenaphoreLocker lock(&fLock);
	sAddressModule->set_to_empty_address((sockaddr *)&socket->address);
	sAddressModule->set_port((sockaddr *)&socket->address, 0);
	sAddressModule->set_to_empty_address((sockaddr *)&socket->peer);
	sAddressModule->set_port((sockaddr *)&socket->peer, 0);
	return B_OK;
}


status_t
TCPConnection::Close()
{
	BenaphoreLocker lock(&fLock);
	TRACE(("TCP:%p.Close()\n", this));
	if (fState == SYN_SENT || fState == LISTEN) {
		fState = CLOSED;
		return B_OK;
	}
	tcp_state nextState = CLOSED;
	if (fState == SYN_RCVD || fState == ESTABLISHED)
		nextState = FIN_WAIT1;
	if (fState == CLOSE_WAIT)
		nextState = LAST_ACK;
	status_t status = SendQueuedData(TCP_FLG_FIN | TCP_FLG_ACK, false);
	if (status != B_OK)
		return status;
	fState = nextState;
	TRACE(("TCP: %p.Close(): Entering state %d\n", this, fState));
	//do i need to wait until fState returns to CLOSED?
	return B_OK;
}


status_t
TCPConnection::Free()
{
	TRACE(("TCP:%p.Free()\n", this));

	BenaphoreLocker hashLock(&sTCPLock);
	BenaphoreLocker lock(&fLock);
	tcp_connection_key key;
	key.local = (sockaddr *)&socket->address;
	key.peer = (sockaddr *)&socket->peer;
	if (hash_lookup(sTCPHash, &key) != NULL) {
		return hash_remove(sTCPHash, (void *)this);
	}
	return B_OK;
}


/*!
	Creates and sends a SYN packet to /a address
*/
status_t
TCPConnection::Connect(const struct sockaddr *address)
{
	TRACE(("TCP:%p.Connect() on address %s\n", this,
		AddressString(sDomain, address, true).Data()));

	if (address->sa_family != AF_INET)
		return EAFNOSUPPORT;

	benaphore_lock(&sTCPLock); // want to release lock later, so no autolock
	BenaphoreLocker lock(&fLock);

	// Can only call Connect from CLOSED or LISTEN states
	// otherwise connection is considered already connected
	if (fState != CLOSED && fState != LISTEN) {
		benaphore_unlock(&sTCPLock);
		return EISCONN;
	}
	TRACE(("TCP: Connect(): in state %d\n", fState));

	// get a net_route if there isn't one
	if (fRoute == NULL) {
		fRoute = sDatalinkModule->get_route(sDomain, (sockaddr *)address);
		TRACE(("TCP: Connect(): Using Route %p\n", fRoute));
		if (fRoute == NULL) {
			benaphore_unlock(&sTCPLock);
			return ENETUNREACH;
		}
	}

	// need to associate this connection with a real address, not INADDR_ANY
	if (sAddressModule->is_empty_address((sockaddr *)&socket->address, false)) {
		TRACE(("TCP: Connect(): Local Address is INADDR_ANY\n"));
		sAddressModule->set_to((sockaddr *)&socket->address, (sockaddr *)fRoute->interface->address);
		// since most stacks terminate connections from port 0
		// use port 40000 for now.  This should be moved to Bind(), and Bind() called before Connect().
		sAddressModule->set_port((sockaddr *)&socket->address, htons(40000));
	}

	// make sure connection does not already exist
	tcp_connection_key key;
	key.local = (sockaddr *)&socket->address;
	key.peer = address;
	if (hash_lookup(sTCPHash, &key) != NULL) {
		benaphore_unlock(&sTCPLock);
		return EADDRINUSE;
	}
	TRACE(("TCP: Connect(): connecting...\n"));

	status_t status;
	sAddressModule->set_to((sockaddr *)&socket->peer, address);
	status = hash_insert(sTCPHash, (void *)this);
	if (status != B_OK) {
		TRACE(("TCP: Connect(): Error inserting connection into hash!\n"));
		benaphore_unlock(&sTCPLock);
		return status;
	}

	// done manipulating the hash, release the lock
	benaphore_unlock(&sTCPLock);

	TRACE(("TCP: Connect(): starting 3-way handshake...\n"));
	// send SYN
	status = SendQueuedData(TCP_FLG_SYN, false);
	if (status != B_OK)
		return status;
	fState = SYN_SENT;

	// TODO: Should Connect() not return until 3-way handshake is complete?
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
		AddressString(sDomain, address, true).Data()));

	if (address->sa_family != AF_INET)
		return EAFNOSUPPORT;

	BenaphoreLocker hashLock(&sTCPLock);
	BenaphoreLocker lock(&fLock);

	// let IP check whether there is an interface that supports the given address:
	status_t status = next->module->bind(next, address);
	if (status < B_OK)
		return status;

	sAddressModule->set_to((sockaddr *)&socket->address, address);
	// for now, leave port=0.  TCP should still work 1 connection at a time
	if (0) { //sAddressModule->get_port((sockaddr *)&socket->address) == 0) {
		//assign ephemeral port
	} else {
		//TODO:Check for Socket flags
		tcp_connection_key key;
		key.peer = (sockaddr *)&socket->peer;
		key.local = (sockaddr *)&socket->address;
		if (hash_lookup(sTCPHash, &key) == NULL) {
			hash_insert(sTCPHash, (void *)this);
		} else
			return EADDRINUSE;
	}

	return B_OK;
}


status_t
TCPConnection::Unbind(struct sockaddr *address)
{
	TRACE(("TCP:%p.Unbind()\n", this ));

	BenaphoreLocker hashLock(&sTCPLock);
	BenaphoreLocker lock(&fLock);

	status_t status = hash_remove(sTCPHash, (void *)this);
	if (status != B_OK)
		return status;

	sAddressModule->set_to_empty_address((sockaddr *)&socket->address);
	sAddressModule->set_port((sockaddr *)&socket->address, 0);
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
		status_t status = sBufferModule->merge(fSendBuffer, buffer, true);
		if (status != B_OK)
			return status;
	} else
		fSendBuffer = buffer;

	fNextByteToWrite += bufferSize;
	return SendQueuedData(TCP_FLG_ACK, false);
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
	if (fState != ESTABLISHED || fState != FIN_WAIT1 || fState != FIN_WAIT2) {
		// is this correct semantics?
dprintf("  TCP state = %d\n", fState);
		return B_ERROR;
	}

dprintf("  TCP error = %ld\n", fError);
	if (fError != B_OK)
		return fError;

	if (fReceiveBuffer->size < numBytes)
		numBytes = fReceiveBuffer->size;

	*_buffer = sBufferModule->split(fReceiveBuffer, numBytes);
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


status_t
TCPConnection::EnqueueReceivedData(net_buffer *buffer, uint32 sequenceNumber)
{
	TRACE(("TCP:%p.EnqueueReceivedData(%p, %lu)\n", this, buffer, sequenceNumber));
	status_t status;

	if (sequenceNumber == fNextByteExpected) {
		// first check if the received buffer meets up with the first
		// segment in the ReorderQueue
		TCPSegment *next;
		while ((next = (TCPSegment *)list_get_first_item(&fReorderQueue)) != NULL) {
			if (sequenceNumber + buffer->size >= next->fSequenceNumber) {
				if (sequenceNumber + buffer->size > next->fSequenceNumber) {
					status = sBufferModule->trim(buffer, sequenceNumber - next->fSequenceNumber);
					if (status != B_OK)
						return status;
				}
				status = sBufferModule->merge(buffer, next->fBuffer, true);
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
			status = sBufferModule->merge(fReceiveBuffer, buffer, true);
			if (status < B_OK) {
				fNextByteExpected -= buffer->size;
				return status;
			}
		}
	} else {
		// add this buffer into the ReorderQueue in the correct place
		// creating a new TCPSegment if necessary
		TCPSegment *next = NULL;
		do {
			next = (TCPSegment *)list_get_next_item(&fReorderQueue, next);
			if (next != NULL && next->fSequenceNumber < sequenceNumber)
				continue;
			if (next != NULL && sequenceNumber + buffer->size >= next->fSequenceNumber) {
				// merge the new buffer with the next buffer
				if (sequenceNumber + buffer->size > next->fSequenceNumber) {
					status = sBufferModule->trim(buffer, sequenceNumber - next->fSequenceNumber);
					if (status != B_OK)
						return status;
				}
				status = sBufferModule->merge(buffer, next->fBuffer, true);
				if (status != B_OK)
					return status;
				next->fBuffer = buffer;
				next->fSequenceNumber = sequenceNumber;
				break;
			}
			TCPSegment *segment = new(std::nothrow) TCPSegment(buffer, sequenceNumber, -1);
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
	uint32 payloadLength = buffer->size - headerLength;

	TRACE(("TCP: ReceiveData(): Connection in state %d received packet %p with flags %X!\n", fState, buffer, header.flags));
	switch (fState) {
		case CLOSED:
		case TIME_WAIT:
			sBufferModule->free(buffer);
			if (header.flags & TCP_FLG_ACK)
				return Reset(byteAckd, 0);

			return Reset(0, byteRcvd + payloadLength);

		case LISTEN:
			// if packet is SYN, spawn new TCPConnection in SYN_RCVD state
			// and add it to the Connection Queue.  The new TCPConnection
			// must continue the handshake by replying with SYN+ACK.  Any
			// data in the packet must go into the new TCPConnection's receive
			// buffer.
			// Otherwise, RST+ACK is sent.
			// The current TCPConnection always remains in LISTEN state.
			return B_ERROR;

		case SYN_SENT:
			if (header.flags & TCP_FLG_RST) {
				fError = ECONNREFUSED;
				fState = CLOSED;
				return B_ERROR;
			}

			if (header.flags & TCP_FLG_ACK && !TCP_IS_GOOD_ACK(byteAckd))
				return Reset(byteAckd, 0);

			if (header.flags & TCP_FLG_SYN) {
				fNextByteToRead = fNextByteExpected = ntohl(header.sequence_num) + 1;
				flags |= TCP_FLG_ACK;
				fLastByteAckd = byteAckd;

				// cancel resend of this segment
				if (header.flags & TCP_FLG_ACK)
					nextState = ESTABLISHED;
				else {
					nextState = SYN_RCVD;
					flags |= TCP_FLG_SYN;
				}
			}
			break;

		case SYN_RCVD:
			if (header.flags & TCP_FLG_ACK && TCP_IS_GOOD_ACK(byteAckd))
				fState = ESTABLISHED;
			else
				Reset(byteAckd, 0);
			break;

		default:
			// In a synchronized state.
			// first check that the received sequence number is good
			if (TCP_IS_GOOD_SEQ(byteRcvd, payloadLength)) {
				// If a valid RST was received, terminate the connection.
				if (header.flags & TCP_FLG_RST) {
					fError = ECONNREFUSED;
					fState = CLOSED;
					return B_ERROR;
				}

				if (header.flags & TCP_FLG_ACK && TCP_IS_GOOD_ACK(byteAckd) ) {
					fLastByteAckd = byteAckd;
					if (fLastByteAckd == fNextByteToWrite) {
						if (fState == LAST_ACK ) {
							nextState = CLOSED;
							status = hash_remove(sTCPHash, this);
							if (status != B_OK)
								return status;
						}
						if (fState == CLOSING) {
							nextState = TIME_WAIT;
							status = hash_remove(sTCPHash, this);
							if (status != B_OK)
								return status;
						}
						if (fState == FIN_WAIT1) {
							nextState = FIN_WAIT2;
						}
					}
				}
				if (header.flags & TCP_FLG_FIN) {
					// other side is closing connection.  change states
					switch (fState) {
						case ESTABLISHED:
							nextState = CLOSE_WAIT;
							fNextByteExpected++;
							break;
						case FIN_WAIT2:
							nextState = TIME_WAIT;
							fNextByteExpected++;
							break;
						case FIN_WAIT1:
							if (fLastByteAckd == fNextByteToWrite) {
								// our FIN has been ACKd: go to TIME_WAIT
								nextState = TIME_WAIT;
								status = hash_remove(sTCPHash, this);
								if (status != B_OK)
									return status;
								sStackModule->set_timer(&fTimer, TCP_MAX_SEGMENT_LIFETIME);
							} else
								nextState = CLOSING;
							fNextByteExpected++;
							break;
						default:
							break;
					}
				}
				flags |= TCP_FLG_ACK;
			} else {
				// out-of-order packet received.  remind the other side of where we are
				return SendQueuedData(TCP_FLG_ACK, true);
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
		status = EnqueueReceivedData(buffer, byteRcvd);
		if (status != B_OK)
			return status;
	} else
		sBufferModule->free(buffer);

	if (fState != CLOSING && fState != LAST_ACK) {
		status = SendQueuedData(flags, false);
		if (status != B_OK)
			return status;
	}

	fState = nextState;
	return B_OK;
}


status_t
TCPConnection::Reset(uint32 sequenceNum, uint32 acknowledgeNum)
{
	TRACE(("TCP:%p.Reset()\n", this));
	net_buffer *reply_buf = sBufferModule->create(512);
	sAddressModule->set_to((sockaddr *)&reply_buf->source, (sockaddr *)&socket->address);
	sAddressModule->set_to((sockaddr *)&reply_buf->destination, (sockaddr *)&socket->peer);

	uint16 flags = TCP_FLG_RST | acknowledgeNum == 0 ? 0 : TCP_FLG_ACK;
	status_t status = tcp_segment(reply_buf, flags , sequenceNum, acknowledgeNum, 0);
	if (status != B_OK) {
		sBufferModule->free(reply_buf);
		return status;
	}
	TRACE(("TCP: Reset():Sending RST...\n"));
	status = next->module->send_routed_data(next, fRoute, reply_buf);
	if (status !=B_OK)
		sBufferModule->free(reply_buf);
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
}


/*!
	Sends a TCP packet with the specified \a flags.  If there is any data in
	the send buffer and \a empty is false, fEffectiveWindow bytes or less of it are sent as well.
	Sequence and Acknowledgement numbers are filled in accordingly.
	The fLock benaphore must be held before calling.
*/
status_t
TCPConnection::SendQueuedData(uint16 flags, bool empty)
{
	TRACE(("TCP:%p.SendQueuedData(%X,%s)\n", this, flags, empty ? "1" : "0"));

	if (fRoute == NULL)
		return B_ERROR;

	net_buffer *buffer;
	uint32 effectiveWindow = min_c(tcp_get_mtu(this, (sockaddr *)&socket->address), fNextByteToWrite - fNextByteToSend);
	if (empty || effectiveWindow == 0 || fSendBuffer == NULL || fSendBuffer->size == 0) {
		buffer = sBufferModule->create(256);
		TRACE(("TCP: Sending Buffer %p\n", buffer));
		if (buffer == NULL)
			return ENOBUFS;
	} else {
		buffer = fSendBuffer;
		if (effectiveWindow == fSendBuffer->size)
			fSendBuffer = NULL;
		else
			fSendBuffer = sBufferModule->split(fSendBuffer, effectiveWindow);
	}

	sAddressModule->set_to((sockaddr *)&buffer->source, (sockaddr *)&socket->address);
	sAddressModule->set_to((sockaddr *)&buffer->destination, (sockaddr *)&socket->peer);
	TRACE(("TCP:%p.SendQueuedData() to address %s\n", this,
		AddressString(sDomain, (sockaddr *)&buffer->destination, true).Data()));
	TRACE(("TCP:%p.SendQueuedData() from address %s\n", this,
		AddressString(sDomain, (sockaddr *)&buffer->source, true).Data()));

	uint16 advWin = TCP_MAX_RECV_BUF - (fNextByteExpected - fNextByteToRead);
	uint32 size = buffer->size;

	status_t status = tcp_segment(buffer, flags, fNextByteToSend, fNextByteExpected, advWin);
	if (status != B_OK) {
		sBufferModule->free(buffer);
		return status;
	}
	// Only count 1 SYN, the 1 sent when transitioning from CLOSED or LISTEN
	if (TCP_FLG_SYN & flags && (fState == CLOSED || fState == LISTEN))
		fNextByteToSend++;
	// Only count 1 FIN, the 1 sent when transitioning from ESTABLISHED, SYN_RCVD or CLOSE_WAIT
	if (TCP_FLG_FIN & flags && (fState == SYN_RCVD || fState == ESTABLISHED || fState == CLOSE_WAIT))
		fNextByteToSend++;
	fNextByteToSend += size;

#if 0
	TCPSegment *segment = new(std::nothrow)
		TCPSegment(sequenceNum, 0, 2*fAvgRTT);
#endif

	return next->module->send_routed_data(next, fRoute, buffer);
}


void
TCPConnection::TimeWait(struct net_timer *timer, void *data)
{
}


int
TCPConnection::Compare(void *_connection, const void *_key)
{
	const tcp_connection_key *key = (tcp_connection_key *)_key;
	TCPConnection *connection= ((TCPConnection *)_connection);

	if (sAddressModule->equal_addresses_and_ports(key->local,
			(sockaddr *)&connection->socket->address)
		&& sAddressModule->equal_addresses_and_ports(key->peer,
			(sockaddr *)&connection->socket->peer))
		return 0;

	return 1;
}


uint32
TCPConnection::Hash(void *_connection, const void *_key, uint32 range)
{
	if (_connection != NULL) {
		TCPConnection *connection = (TCPConnection *)_connection;
		return sAddressModule->hash_address_pair(
			(sockaddr *)&connection->socket->address, (sockaddr *)&connection->socket->peer) % range;
	}

	const tcp_connection_key *key = (tcp_connection_key *)_key;
	return sAddressModule->hash_address_pair( 
			key->local, key->peer) % range;
}
