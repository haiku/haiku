/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Andrew Galante, haiku.galante@gmail.com
 */


typedef struct {
	in_addr_t	source;
	in_addr_t	destination;
	uint16		source_port;
	uint16		destination_port;
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

	static int Compare(void *_packet, const void *_key);
	static uint32 Hash(void *_packet, const void *_key, uint32 range);
	static int32 HashOffset() { return offsetof(TCPConnection, fHashLink); }
private:
	TCPConnection	*fHashLink;
	tcp_state		fState;
	benaphore		fLock;
public:
	tcp_connection_key	fKey;
};


TCPConnection::TCPConnection(net_socket *socket)
	:
	fState(CLOSED)
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
	return B_OK;
}


status_t
TCPConnection::Close()
{
	return B_OK;
}


status_t
TCPConnection::Free()
{
	return B_OK;
}


status_t
TCPConnection::Connect(const struct sockaddr *address)
{
	return B_OK;
}


status_t
TCPConnection::Accept(struct net_socket **_acceptedSocket)
{
	return B_OK;
}


status_t
TCPConnection::Bind(sockaddr *address)
{
	if (address->sa_family != AF_INET)
		return EAFNOSUPPORT;

	BenaphoreLocker lock(&fLock);

	// let IP check whether there is an interface that supports the given address:
	status_t status = next->module->bind(next, address);
	if (status < B_OK)
		return status;

	//TODO: clean this up
/*	fKey.source = ((struct sockaddr_in *)address)->sin_addr;
	fKey.source_port = sAddressModule->get_port(address);
	fKey.destination = INADDR_ANY;
	fKey.destination_port = 0;*/

	return B_OK;
}


status_t
TCPConnection::Unbind(struct sockaddr *address)
{
	return B_OK;
}


status_t
TCPConnection::Listen(int count)
{
	return B_OK;
}


status_t
TCPConnection::Shutdown(int direction)
{
	return B_OK;
}


status_t
TCPConnection::SendData(net_buffer *buffer)
{
	return B_ERROR;
}


status_t
TCPConnection::SendRoutedData(net_route *route, net_buffer *buffer)
{
	return B_ERROR;
}


status_t
TCPConnection::SendAvailable()
{
	return B_ERROR;
}


status_t
TCPConnection::ReadData(size_t numBytes, uint32 flags, net_buffer **_buffer)
{
	return B_ERROR;
}


status_t
TCPConnection::ReadAvailable()
{
	return B_ERROR;
}


int
TCPConnection::Compare(void *_packet, const void *_key)
{
	const tcp_connection_key *key = (tcp_connection_key *)_key;
	tcp_connection_key *packetKey = &((TCPConnection *)_packet)->fKey;

	if (key->source == packetKey->source
		&& key->destination == packetKey->destination
		&& key->source_port == packetKey->source_port
		&& key->destination_port == packetKey->destination_port)
		return 0;

	return 1;
}


uint32
TCPConnection::Hash(void *_packet, const void *_key, uint32 range)
{
	const tcp_connection_key *key = (tcp_connection_key *)_key;
	if (_packet != NULL)
		key = &((TCPConnection *)_packet)->fKey;

	return ((uint32)key->source_port << 16 ^ (uint32)key->destination_port
			^ key->source ^ key->destination) % range;
}
