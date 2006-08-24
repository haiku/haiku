/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Galante, haiku.galante@gmail.com
 */

// Initial estimate for packet round trip time (RTT)
#define	TCP_INITIAL_RTT	120000000LL

// keep maximum buffer sizes < max net_buffer size for now
#define	TCP_MAX_SEND_BUF 1024
#define TCP_MAX_RECV_BUF TCP_MAX_SEND_BUF

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
	status_t SendAvailable();
	status_t ReadData(size_t numBytes, uint32 flags, net_buffer **_buffer);
	status_t ReadAvailable();

	status_t ReceiveData(net_buffer *buffer);

	static void ResendSegment(struct net_timer *timer, void *data);
	static int Compare(void *_packet, const void *_key);
	static uint32 Hash(void *_packet, const void *_key, uint32 range);
	static int32 HashOffset() { return offsetof(TCPConnection, fHashLink); }
private:

	class TCPSegment {
	public:
		TCPSegment(uint32 sequenceNumber, uint32 size, bigtime_t timeout);
		~TCPSegment();

		bigtime_t	fTime;
		uint32		fSequenceNumber;
		uint32		fAcknowledgementNumber;
		net_timer	fTimer;
		bool		fTimedOut;
	};

	status_t Send(uint8 flags);

	uint32	fLastByteSent;
	uint32	fLastByteAckd;
	uint32	fLastByteWritten;

	uint32	fLastByteRead;
	uint32	fNextByteExpected;
	uint32	fLastByteReceived;

	uint32	fEffectiveWindow;
	
	bigtime_t	fAvgRTT;

	TCPSegment	*fSent;
	TCPSegment	*fReceived;

	TCPConnection	*fHashLink;
	tcp_state		fState;
	benaphore		fLock;

	net_route 	*fRoute;
};


TCPConnection::TCPSegment::TCPSegment(uint32 sequenceNumber, uint32 size, bigtime_t timeout)
	:
	fTime(system_time()),
	fSequenceNumber(sequenceNumber),
	fAcknowledgementNumber(sequenceNumber+size),
	fTimedOut(false)
{
	sStackModule->init_timer(&fTimer, &TCPConnection::ResendSegment, this);
	sStackModule->set_timer(&fTimer, timeout);
}


TCPConnection::TCPSegment::~TCPSegment()
{
	sStackModule->set_timer(&fTimer, -1);
}


TCPConnection::TCPConnection(net_socket *socket)
	:
	fLastByteSent(0), //system_time()),
	fLastByteAckd(1), //fLastByteSent + 1),
	fLastByteWritten(1), //fLastByteSent + 1),
	fAvgRTT(TCP_INITIAL_RTT),
	fState(CLOSED),
	fRoute(NULL)
{
	benaphore_init(&fLock, "TCPConnection");
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
	TRACE(("Using Address Module %p\n", sAddressModule));

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
	TRACE(("%p.Close()\n", this));

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


status_t
TCPConnection::Free()
{
	TRACE(("%p.Free()\n", this));
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
	// update the hash with the new key values
	key.peer = (sockaddr *)&socket->peer;
	if (hash_lookup(sTCPHash, &key) != NULL) {
		status = hash_remove(sTCPHash, (void *)this);
		if (status != B_OK) {
			TRACE(("TCP: Hash Error: %s\n", strerror(status)));
			benaphore_unlock(&sTCPLock);
			return status;
		}
	} else
		TRACE(("TCP: Connect(): Connection not in hash yet!\n"));
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
	status = Send(TCP_FLG_SYN);
	if (status != B_OK)
		return status;
	fState=SYN_SENT;

	// TODO: Should Connect() not return until 3-way handshake is complete?
	TRACE(("TCP: Connect(): Connection complete\n"));
	return B_OK;
}


status_t
TCPConnection::Accept(struct net_socket **_acceptedSocket)
{
	TRACE(("%p.Accept()\n", this));
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
	TRACE(("%p.Listen()\n", this));
	if (fState != CLOSED)
		return B_ERROR;
	fState = LISTEN;
	return B_OK;
}


status_t
TCPConnection::Shutdown(int direction)
{
	TRACE(("%p.Shutdown()\n", this));
	return B_ERROR;
}


/*!
 Puts data contained in \a buffer into send buffer
*/
status_t
TCPConnection::SendData(net_buffer *buffer)
{
	TRACE(("%p.SendData()\n", this));
	return B_ERROR;
}


status_t
TCPConnection::SendRoutedData(net_route *route, net_buffer *buffer)
{
	TRACE(("%p.SendRoutedData()\n", this));
	fRoute = route;
	return SendData(buffer);
}


status_t
TCPConnection::SendAvailable()
{
	TRACE(("%p.SendAvailable()\n", this));
	return B_ERROR;
}


status_t
TCPConnection::ReadData(size_t numBytes, uint32 flags, net_buffer **_buffer)
{
	TRACE(("%p.ReadData()\n", this));
	return B_ERROR;
}


status_t
TCPConnection::ReadAvailable()
{
	TRACE(("%p.ReadAvailable()\n", this));
	return B_ERROR;
}


status_t
TCPConnection::ReceiveData(net_buffer *buffer)
{
	TRACE(("%p.ReceiveData()\n", this));

	switch (fState) {
	case CLOSED:
		// shouldn't happen.  send RST
		TRACE(("TCP:  Connection in CLOSED state received packet %p!\n", buffer));
		break;
	case LISTEN:
		// if packet is SYN, spawn new TCPConnection in SYN_RCVD state
		// and add it to the Connection Queue.  The new TCPConnection
		// must continue the handshake by replying with SYN+ACK.  Any
		// data in the packet must go into the new TCPConnection's receive
		// buffer.
		// Otherwise, RST+ACK is sent.
		// The current TCPConnection always remains in LISTEN state.
		break;
	case SYN_SENT:
		// if packet is SYN+ACK, send ACK & enter ESTABLISHED state.
		// if packet is SYN, send ACK & enter SYN_RCVD state.
		break;
	case SYN_RCVD:
		// if packet is ACK, enter ESTABLISHED
		break;
	case ESTABLISHED:
		// if packet has ACK, update send buffer
		// if packet is FIN, send ACK & enter CLOSE_WAIT
		// update receive buffer if necessary
		break;
	// more cases needed covering connection tear-down 
	default:
		return B_ERROR;
	}
	return B_ERROR;
}


/*!
 	Resends a sent segment (\a data) if the segment's ACK wasn't received
	before the timeout (eg \a timer expired)
*/
void
TCPConnection::ResendSegment(struct net_timer *timer, void *data)
{
	TRACE(("ResendSegment(%p)\n", data));
	if (data == NULL)
		return;
}


/*!
	Sends a TCP packet with the specified \a flags.  If there is any data in
	the send buffer, fEffectiveWindow bytes or less of it are sent as well.
	Sequence and Acknowledgement numbers are filled in accordingly.
	The fLock benaphore must be held before calling.
*/
status_t
TCPConnection::Send(uint8 flags)
{
	TRACE(("%p.Send()\n", this));
	return B_OK;

	uint32 sequenceNum = 0;
	uint32 ackNum = 0;
	net_buffer *buffer;
	if (1/*no data in send buffer*/) {
		buffer = sBufferModule->create(512);
		if (buffer == NULL)
			return ENOBUFS;
	} else {
		//TODO: create net_buffer from data in send buffer
	}

	// get a net_route if there isn't one
	if (fRoute == NULL) {
		fRoute = sDatalinkModule->get_route(sDomain, (sockaddr *)&socket->peer);
		if (fRoute == NULL) {
			sBufferModule->free(buffer);
			return ENETUNREACH;
		}
	}

	uint16 advWin = TCP_MAX_RECV_BUF - (fNextByteExpected - fLastByteRead);

	tcp_segment(buffer, flags, sequenceNum, ackNum, advWin);

#if 0
	TCPSegment *segment = new(std::nothrow)
		TCPSegment(sequenceNum, 0, 2*fAvgRTT);
#endif

	return next->module->send_routed_data(next, fRoute, buffer);
}


int
TCPConnection::Compare(void *_connection, const void *_key)
{
	const tcp_connection_key *key = (tcp_connection_key *)_key;
	TCPConnection *connection= ((TCPConnection *)_connection);

	if (sAddressModule->equal_addresses_and_ports(
			key->local, (sockaddr *)&connection->socket->address)
		&& sAddressModule->equal_addresses_and_ports(
			key->peer, (sockaddr *)&connection->socket->peer))
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
