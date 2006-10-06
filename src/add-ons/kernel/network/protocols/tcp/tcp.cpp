/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <net_buffer.h>
#include <net_datalink.h>
#include <net_protocol.h>
#include <net_stack.h>

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
#	define TRACE_BLOCK(x) dump_block x
#else
#	define TRACE(x)
#	define TRACE_BLOCK(x)
#endif


#define MAX_HASH_TCP	64

static net_domain *sDomain;
static net_address_module_info *sAddressModule;
net_buffer_module_info *sBufferModule;
static net_datalink_module_info *sDatalinkModule;
static net_stack_module_info *sStackModule;
static hash_table *sTCPHash;
static benaphore sTCPLock;

status_t
tcp_segment(net_buffer *buffer, uint16 flags, uint32 seq, uint32 ack, uint16 adv_win);

size_t
tcp_get_mtu(net_protocol *protocol, const struct sockaddr *address);

#include "tcp.h"
#include "TCPConnection.h"


#ifdef TRACE_TCP
#	define DUMP_TCP_HASH tcp_dump_hash()
// Dumps the TCP Connection hash.  sTCPLock must NOT be held when calling
void
tcp_dump_hash(){
	BenaphoreLocker lock(&sTCPLock);
	if (sDomain == NULL) {
		TRACE(("Unable to dump TCP Connections!\n"));
		return;
	}
	struct hash_iterator iterator;
	hash_open(sTCPHash, &iterator);
	TCPConnection *connection;
	hash_rewind(sTCPHash, &iterator);
	TRACE(("Active TCP Connections:\n"));
	while ((connection = (TCPConnection *)hash_next(sTCPHash, &iterator)) != NULL) {
		TRACE(("  TCPConnection %p: %s, %s\n", connection,
		AddressString(sDomain, (sockaddr *)&connection->socket->address, true).Data(),
		AddressString(sDomain, (sockaddr *)&connection->socket->peer, true).Data()));
	}
	hash_close(sTCPHash, &iterator, false);
}
#else
#	define DUMP_TCP_HASH 0
#endif


net_protocol *
tcp_init_protocol(net_socket *socket)
{
	DUMP_TCP_HASH;
	socket->protocol = IPPROTO_TCP;
	TCPConnection *protocol = new (std::nothrow) TCPConnection(socket);
	TRACE(("Creating new TCPConnection: %p\n", protocol));
	return protocol;
}


status_t
tcp_uninit_protocol(net_protocol *protocol)
{
	DUMP_TCP_HASH;
	TRACE(("Deleting TCPConnection: %p\n", protocol));
	delete (TCPConnection *)protocol;
	return B_OK;
}


status_t
tcp_open(net_protocol *protocol)
{
	if (!sDomain)
		sDomain = sStackModule->get_domain(AF_INET);
	if (!sAddressModule)
		sAddressModule = sDomain->address_module;
	DUMP_TCP_HASH;

	return ((TCPConnection *)protocol)->Open();
}


status_t
tcp_close(net_protocol *protocol)
{
	DUMP_TCP_HASH;
	return ((TCPConnection *)protocol)->Close();
}


status_t
tcp_free(net_protocol *protocol)
{
	DUMP_TCP_HASH;
	return ((TCPConnection *)protocol)->Free();
}


status_t
tcp_connect(net_protocol *protocol, const struct sockaddr *address)
{
	DUMP_TCP_HASH;
	return ((TCPConnection *)protocol)->Connect(address);
}


status_t
tcp_accept(net_protocol *protocol, struct net_socket **_acceptedSocket)
{
	return ((TCPConnection *)protocol)->Accept(_acceptedSocket);
}


status_t
tcp_control(net_protocol *protocol, int level, int option, void *value,
	size_t *_length)
{
	return protocol->next->module->control(protocol->next, level, option,
		value, _length);
}


status_t
tcp_bind(net_protocol *protocol, struct sockaddr *address)
{
	DUMP_TCP_HASH;
	return ((TCPConnection *)protocol)->Bind(address);
}


status_t
tcp_unbind(net_protocol *protocol, struct sockaddr *address)
{
	DUMP_TCP_HASH;
	return ((TCPConnection *)protocol)->Unbind(address);
}


status_t
tcp_listen(net_protocol *protocol, int count)
{
	return ((TCPConnection *)protocol)->Listen(count);
}


status_t
tcp_shutdown(net_protocol *protocol, int direction)
{
	return ((TCPConnection *)protocol)->Shutdown(direction);
}


status_t
tcp_send_data(net_protocol *protocol, net_buffer *buffer)
{
	return ((TCPConnection *)protocol)->SendData(buffer);
}


status_t
tcp_send_routed_data(net_protocol *protocol, struct net_route *route,
	net_buffer *buffer)
{
	return ((TCPConnection *)protocol)->SendRoutedData(route, buffer);
}


ssize_t
tcp_send_avail(net_protocol *protocol)
{
	return ((TCPConnection *)protocol)->SendAvailable();
}


status_t
tcp_read_data(net_protocol *protocol, size_t numBytes, uint32 flags,
	net_buffer **_buffer)
{
	return ((TCPConnection *)protocol)->ReadData(numBytes, flags, _buffer);
}


ssize_t
tcp_read_avail(net_protocol *protocol)
{
	return ((TCPConnection *)protocol)->ReadAvailable();
}


struct net_domain *
tcp_get_domain(net_protocol *protocol)
{
	return protocol->next->module->get_domain(protocol->next);
}


size_t
tcp_get_mtu(net_protocol *protocol, const struct sockaddr *address)
{
	return protocol->next->module->get_mtu(protocol->next, address);
}


/*!
	Constructs a TCP header on \a buffer with the specified values
	for \a flags, \a seq \a ack and \a adv_win.
*/
status_t
tcp_segment(net_buffer *buffer, uint16 flags, uint32 seq, uint32 ack, uint16 adv_win)
{
	buffer->protocol = IPPROTO_TCP;

	NetBufferPrepend<tcp_header> bufferHeader(buffer);
	if (bufferHeader.Status() != B_OK)
		return bufferHeader.Status();

	tcp_header &header = bufferHeader.Data();

	header.source_port = sAddressModule->get_port((sockaddr *)&buffer->source);
	header.destination_port = sAddressModule->get_port((sockaddr *)&buffer->destination);
	header.sequence_num = htonl(seq);
	header.acknowledge_num = htonl(ack);
	header.reserved = 0;
	header.header_length = 5;// currently no options supported
	header.flags = (uint8)flags;
	header.advertised_window = htons(adv_win);
	header.checksum = 0;
	header.urgent_ptr = 0;// urgent pointer not supported
	
	// compute and store checksum
	Checksum checksum;
	sAddressModule->checksum_address(&checksum, (sockaddr *)&buffer->source);
	sAddressModule->checksum_address(&checksum, (sockaddr *)&buffer->destination);
	checksum
		<< (uint16)htons(IPPROTO_TCP)
		<< (uint16)htons(buffer->size)
		<< Checksum::BufferHelper(buffer, sBufferModule);
	header.checksum = checksum;
	TRACE(("TCP: Checksum for segment %p is %X\n", buffer, header.checksum));
	return B_OK;
}


status_t
tcp_receive_data(net_buffer *buffer)
{
	TRACE(("TCP: Received buffer %p\n", buffer));
	if (!sDomain) {
		// domain and address module are not known yet, we copy them from
		// the buffer's interface (if any):
		if (buffer->interface == NULL || buffer->interface->domain == NULL)
			sDomain = sStackModule->get_domain(AF_INET);
		else
			sDomain = buffer->interface->domain;
		if (sDomain == NULL) {
			// this shouldn't occur, of course, but who knows...
			return B_BAD_VALUE;
		}
		sAddressModule = sDomain->address_module;
	}

	NetBufferHeader<tcp_header> bufferHeader(buffer);
	if (bufferHeader.Status() < B_OK)
		return bufferHeader.Status();

	tcp_header &header = bufferHeader.Data();

	tcp_connection_key key;
	key.peer = (struct sockaddr *)&buffer->source;
	key.local = (struct sockaddr *)&buffer->destination;

	//TODO: check TCP Checksum

	sAddressModule->set_port((struct sockaddr *)&buffer->source, header.source_port);
	sAddressModule->set_port((struct sockaddr *)&buffer->destination, header.destination_port);

	DUMP_TCP_HASH;

	BenaphoreLocker hashLock(&sTCPLock);
	TCPConnection *connection = (TCPConnection *)hash_lookup(sTCPHash, &key);
	TRACE(("TCP: Received packet corresponds to connection %p\n", connection));
	if (connection != NULL){
		return connection->ReceiveData(buffer);
	} else {
		/* TODO:
		   No explicit connection exists.  Check for wildcard connections:
		   First check if any connections exist where local = IPADDR_ANY
		   then check when local = peer = IPADDR_ANY.
		   port numbers always remain the same */

		// If no connection exists (and RST is not set) send RST
		if (!(header.flags & TCP_FLG_RST)) {
			TRACE(("TCP:  Connection does not exist!\n"));
			net_buffer *reply_buf = sBufferModule->create(512);
			sAddressModule->set_to((sockaddr *)&reply_buf->source, (sockaddr *)&buffer->destination);
			sAddressModule->set_to((sockaddr *)&reply_buf->destination, (sockaddr *)&buffer->source);

			uint32 sequenceNum, acknowledgeNum;
			uint16 flags;
			if (header.flags & TCP_FLG_ACK) {
				sequenceNum = ntohl(header.acknowledge_num);
				acknowledgeNum = 0;
				flags = TCP_FLG_RST;
			} else {
				sequenceNum = 0;
				acknowledgeNum = ntohl(header.sequence_num) + 1 + buffer->size - ((uint32)header.header_length<<2);
				flags = TCP_FLG_RST | TCP_FLG_ACK;
			}
			
			status_t status = tcp_segment(reply_buf, flags, sequenceNum, acknowledgeNum, 0);
			if (status != B_OK) {
				sBufferModule->free(reply_buf);
				return status;
			}
			TRACE(("TCP:  Sending RST...\n"));
			status = sDomain->module->send_data(NULL, reply_buf);
			if (status !=B_OK) {
				sBufferModule->free(reply_buf);
				return status;
			}
		}
	}
	return B_OK;
}


status_t
tcp_error(uint32 code, net_buffer *data)
{
	return B_ERROR;
}


status_t
tcp_error_reply(net_protocol *protocol, net_buffer *causedError, uint32 code,
	void *errorData)
{
	return B_ERROR;
}


//	#pragma mark -


static status_t
tcp_init()
{
	status_t status;

	sDomain = NULL;
	sAddressModule = NULL;

	status = get_module(NET_STACK_MODULE_NAME, (module_info **)&sStackModule);
	if (status < B_OK)
		return status;
	status = get_module(NET_BUFFER_MODULE_NAME, (module_info **)&sBufferModule);
	if (status < B_OK)
		goto err1;
	status = get_module(NET_DATALINK_MODULE_NAME, (module_info **)&sDatalinkModule);
	if (status < B_OK)
		goto err2;

	sTCPHash = hash_init(MAX_HASH_TCP, TCPConnection::HashOffset(),
		&TCPConnection::Compare, &TCPConnection::Hash);
	if (sTCPHash == NULL)
		goto err3;

	status = benaphore_init(&sTCPLock, "TCP Hash Lock");
	if (status < B_OK)
		goto err4;

	status = sStackModule->register_domain_protocols(AF_INET, SOCK_STREAM, IPPROTO_IP,
		"network/protocols/tcp/v1",
		"network/protocols/ipv4/v1",
		NULL);
	if (status < B_OK)
		goto err5;

	status = sStackModule->register_domain_protocols(AF_INET, SOCK_STREAM, IPPROTO_TCP,
		"network/protocols/tcp/v1",
		"network/protocols/ipv4/v1",
		NULL);
	if (status < B_OK)
		goto err5;

	status = sStackModule->register_domain_receiving_protocol(AF_INET, IPPROTO_TCP,
		"network/protocols/tcp/v1");
	if (status < B_OK)
		goto err5;

	return B_OK;

err5:
	benaphore_destroy(&sTCPLock);
err4:
	hash_uninit(sTCPHash);
err3:
	put_module(NET_DATALINK_MODULE_NAME);
err2:
	put_module(NET_BUFFER_MODULE_NAME);
err1:
	put_module(NET_STACK_MODULE_NAME);

	TRACE(("init_tcp() fails with %lx (%s)\n", status, strerror(status)));
	return status;
}


static status_t
tcp_uninit()
{
	benaphore_destroy(&sTCPLock);
	hash_uninit(sTCPHash);
	put_module(NET_DATALINK_MODULE_NAME);
	put_module(NET_BUFFER_MODULE_NAME);
	put_module(NET_STACK_MODULE_NAME);

	return B_OK;
}


static status_t
tcp_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return tcp_init();

		case B_MODULE_UNINIT:
			return tcp_uninit();

		default:
			return B_ERROR;
	}
}


net_protocol_module_info sTCPModule = {
	{
		"network/protocols/tcp/v1",
		0,
		tcp_std_ops
	},
	tcp_init_protocol,
	tcp_uninit_protocol,
	tcp_open,
	tcp_close,
	tcp_free,
	tcp_connect,
	tcp_accept,
	tcp_control,
	tcp_bind,
	tcp_unbind,
	tcp_listen,
	tcp_shutdown,
	tcp_send_data,
	tcp_send_routed_data,
	tcp_send_avail,
	tcp_read_data,
	tcp_read_avail,
	tcp_get_domain,
	tcp_get_mtu,
	tcp_receive_data,
	tcp_error,
	tcp_error_reply,
};

module_info *modules[] = {
	(module_info *)&sTCPModule,
	NULL
};
