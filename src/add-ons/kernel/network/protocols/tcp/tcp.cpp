/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Andrew Galante, haiku.galante@gmail.com
 */


#include "EndpointManager.h"
#include "TCPEndpoint.h"

#include <net_protocol.h>

#include <KernelExport.h>
#include <util/list.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <new>
#include <stdlib.h>
#include <string.h>

#include <lock.h>
#include <util/AutoLock.h>

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


net_domain *gDomain;
net_address_module_info *gAddressModule;
net_buffer_module_info *gBufferModule;
net_datalink_module_info *gDatalinkModule;
net_socket_module_info *gSocketModule;
net_stack_module_info *gStackModule;
EndpointManager *gEndpointManager;


status_t
set_domain(net_interface *interface = NULL)
{
	if (gDomain == NULL) {
		// domain and address module are not known yet, we copy them from
		// the buffer's interface (if any):
		if (interface == NULL || interface->domain == NULL)
			gDomain = gStackModule->get_domain(AF_INET);
		else
			gDomain = interface->domain;

		if (gDomain == NULL) {
			// this shouldn't occur, of course, but who knows...
			return B_BAD_VALUE;
		}
		gAddressModule = gDomain->address_module;
	}

	return B_OK;
}


static inline void
bump_option(tcp_option *&option, size_t &length)
{
	if (option->kind <= TCP_OPTION_NOP) {
		length++;
		option = (tcp_option *)((uint8 *)option + 1);
	} else {
		length += option->length;
		option = (tcp_option *)((uint8 *)option + option->length);
	}
}


static inline size_t
add_options(tcp_segment_header &segment, uint8 *buffer, size_t bufferSize)
{
	tcp_option *option = (tcp_option *)buffer;
	size_t length = 0;

	if (segment.max_segment_size > 0 && length + 8 < bufferSize) {
		option->kind = TCP_OPTION_MAX_SEGMENT_SIZE;
		option->length = 4;
		option->max_segment_size = htons(segment.max_segment_size);
		bump_option(option, length);
	}
	if (segment.has_window_shift && length + 4 < bufferSize) {
		// insert one NOP so that the subsequent data is aligned on a 4 byte boundary
		option->kind = TCP_OPTION_NOP;
		bump_option(option, length);
		
		option->kind = TCP_OPTION_WINDOW_SHIFT;
		option->length = 3;
		option->window_shift = segment.window_shift;
		bump_option(option, length);
	}

	if ((length & 3) == 0) {
		// options completely fill out the option space
		return length;
	}

	option->kind = TCP_OPTION_END;
	return (length + 3) & ~3;
		// bump to a multiple of 4 length
}


/*!
	Constructs a TCP header on \a buffer with the specified values
	for \a flags, \a seq \a ack and \a advertisedWindow.
*/
status_t
add_tcp_header(tcp_segment_header &segment, net_buffer *buffer)
{
	buffer->protocol = IPPROTO_TCP;

	uint8 optionsBuffer[32];
	uint32 optionsLength = add_options(segment, optionsBuffer, sizeof(optionsBuffer));

	NetBufferPrepend<tcp_header> bufferHeader(buffer, sizeof(tcp_header) + optionsLength);
	if (bufferHeader.Status() != B_OK)
		return bufferHeader.Status();

	tcp_header &header = bufferHeader.Data();

	header.source_port = gAddressModule->get_port((sockaddr *)&buffer->source);
	header.destination_port = gAddressModule->get_port((sockaddr *)&buffer->destination);
	header.sequence = htonl(segment.sequence);
	header.acknowledge = (segment.flags & TCP_FLAG_ACKNOWLEDGE)
		? htonl(segment.acknowledge) : 0;
	header.reserved = 0;
	header.header_length = (sizeof(tcp_header) + optionsLength) >> 2;
	header.flags = segment.flags;
	header.advertised_window = htons(segment.advertised_window);
	header.checksum = 0;
	header.urgent_offset = 0;
		// TODO: urgent pointer not yet supported

	if (optionsLength > 0)
		gBufferModule->write(buffer, sizeof(tcp_header), optionsBuffer, optionsLength);

	TRACE(("add_tcp_header(): buffer %p, flags 0x%x, seq %lu, ack %lu, win %u\n", buffer,
		segment.flags, segment.sequence, segment.acknowledge, segment.advertised_window));

	// compute and store checksum
	Checksum checksum;
	gAddressModule->checksum_address(&checksum, (sockaddr *)&buffer->source);
	gAddressModule->checksum_address(&checksum, (sockaddr *)&buffer->destination);
	checksum
		<< (uint16)htons(IPPROTO_TCP)
		<< (uint16)htons(buffer->size)
		<< Checksum::BufferHelper(buffer, gBufferModule);
	header.checksum = checksum;

	return B_OK;
}


void
process_options(tcp_segment_header &segment, net_buffer *buffer, int32 size)
{
	if (size == 0)
		return;

	tcp_option *option;
	uint8 optionsBuffer[32];
	if (gBufferModule->direct_access(buffer, sizeof(tcp_header), size,
			(void **)&option) != B_OK) {
		if (size > 32) {
			dprintf("options too large to take into account (%ld bytes)\n", size);
			return;
		}

		gBufferModule->read(buffer, sizeof(tcp_header), optionsBuffer, size);
		option = (tcp_option *)optionsBuffer;
	}

	while (size > 0) {
		uint32 length = 1;
		switch (option->kind) {
			case TCP_OPTION_END:
			case TCP_OPTION_NOP:
				break;
			case TCP_OPTION_MAX_SEGMENT_SIZE:
				segment.max_segment_size = ntohs(option->max_segment_size);
				length = 4;
				break;
			case TCP_OPTION_WINDOW_SHIFT:
				segment.has_window_shift = true;
				segment.window_shift = option->window_shift;
				length = 3;
				break;
			case TCP_OPTION_TIMESTAMP:
				// TODO: support timestamp!
				length = 10;
				break;

			default:
				length = option->length;
				// make sure we don't end up in an endless loop
				if (length == 0)
					return;
				break;
		}

		size -= length;
		option = (tcp_option *)((uint8 *)option + length);
	}
	// TODO: check if options are valid!
}


status_t
reply_with_reset(tcp_segment_header &segment, net_buffer *buffer)
{
	TRACE(("TCP: Sending RST...\n"));

	net_buffer *reply = gBufferModule->create(512);
	if (reply == NULL)
		return B_NO_MEMORY;

	gAddressModule->set_to((sockaddr *)&reply->source,
		(sockaddr *)&buffer->destination);
	gAddressModule->set_to((sockaddr *)&reply->destination,
		(sockaddr *)&buffer->source);

	tcp_segment_header outSegment;
	outSegment.flags = TCP_FLAG_RESET;
	outSegment.sequence = 0;
	outSegment.acknowledge = 0;
	outSegment.advertised_window = 0;
	outSegment.urgent_offset = 0;

	if ((segment.flags & TCP_FLAG_ACKNOWLEDGE) == 0) {
		outSegment.flags |= TCP_FLAG_ACKNOWLEDGE;
		outSegment.acknowledge = segment.sequence + buffer->size;
	} else
		outSegment.sequence = segment.acknowledge;

	status_t status = add_tcp_header(outSegment, reply);
	if (status == B_OK)
		status = gDomain->module->send_data(NULL, reply);

	if (status != B_OK)
		gBufferModule->free(reply);

	return status;
}


//	#pragma mark - protocol API


net_protocol *
tcp_init_protocol(net_socket *socket)
{
	TCPEndpoint *protocol = new (std::nothrow) TCPEndpoint(socket);
	if (protocol == NULL)
		return NULL;

	if (protocol->InitCheck() != B_OK) {
		delete protocol;
		return NULL;
	}

	TRACE(("Creating new TCPEndpoint: %p\n", protocol));
	socket->protocol = IPPROTO_TCP;
	return protocol;
}


status_t
tcp_uninit_protocol(net_protocol *protocol)
{
	TRACE(("Deleting TCPEndpoint: %p\n", protocol));
	delete (TCPEndpoint *)protocol;
	return B_OK;
}


status_t
tcp_open(net_protocol *protocol)
{
	if (gDomain == NULL && set_domain() != B_OK)
		return B_ERROR;

	return ((TCPEndpoint *)protocol)->Open();
}


status_t
tcp_close(net_protocol *protocol)
{
	return ((TCPEndpoint *)protocol)->Close();
}


status_t
tcp_free(net_protocol *protocol)
{
	return ((TCPEndpoint *)protocol)->Free();
}


status_t
tcp_connect(net_protocol *protocol, const struct sockaddr *address)
{
	return ((TCPEndpoint *)protocol)->Connect(address);
}


status_t
tcp_accept(net_protocol *protocol, struct net_socket **_acceptedSocket)
{
	return ((TCPEndpoint *)protocol)->Accept(_acceptedSocket);
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
	return ((TCPEndpoint *)protocol)->Bind(address);
}


status_t
tcp_unbind(net_protocol *protocol, struct sockaddr *address)
{
	return ((TCPEndpoint *)protocol)->Unbind(address);
}


status_t
tcp_listen(net_protocol *protocol, int count)
{
	return ((TCPEndpoint *)protocol)->Listen(count);
}


status_t
tcp_shutdown(net_protocol *protocol, int direction)
{
	return ((TCPEndpoint *)protocol)->Shutdown(direction);
}


status_t
tcp_send_data(net_protocol *protocol, net_buffer *buffer)
{
	return ((TCPEndpoint *)protocol)->SendData(buffer);
}


status_t
tcp_send_routed_data(net_protocol *protocol, struct net_route *route,
	net_buffer *buffer)
{
	// TCP never sends routed data
	return B_ERROR;
}


ssize_t
tcp_send_avail(net_protocol *protocol)
{
	return ((TCPEndpoint *)protocol)->SendAvailable();
}


status_t
tcp_read_data(net_protocol *protocol, size_t numBytes, uint32 flags,
	net_buffer **_buffer)
{
	return ((TCPEndpoint *)protocol)->ReadData(numBytes, flags, _buffer);
}


ssize_t
tcp_read_avail(net_protocol *protocol)
{
	return ((TCPEndpoint *)protocol)->ReadAvailable();
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


status_t
tcp_receive_data(net_buffer *buffer)
{
	TRACE(("TCP: Received buffer %p\n", buffer));

	if (gDomain == NULL && set_domain(buffer->interface) != B_OK)
		return B_ERROR;

	NetBufferHeader<tcp_header> bufferHeader(buffer);
	if (bufferHeader.Status() < B_OK)
		return bufferHeader.Status();

	tcp_header &header = bufferHeader.Data();

	uint16 headerLength = header.HeaderLength();
	if (headerLength < sizeof(tcp_header))
		return B_BAD_DATA;

	// compute checksum using a pseudo IP header
	Checksum checksum;
	gAddressModule->checksum_address(&checksum, (sockaddr *)&buffer->source);
	gAddressModule->checksum_address(&checksum, (sockaddr *)&buffer->destination);
	checksum << (uint16)htons(IPPROTO_TCP)
		<< (uint16)htons(buffer->size)
		<< Checksum::BufferHelper(buffer, gBufferModule);

	if (checksum != 0)
		return B_BAD_DATA;

	gAddressModule->set_port((struct sockaddr *)&buffer->source, header.source_port);
	gAddressModule->set_port((struct sockaddr *)&buffer->destination, header.destination_port);

	TRACE(("  Looking for: peer %s, local %s\n",
		AddressString(gDomain, (sockaddr *)&buffer->source, true).Data(),
		AddressString(gDomain, (sockaddr *)&buffer->destination, true).Data()));

	tcp_segment_header segment;
	segment.sequence = header.Sequence();
	segment.acknowledge = header.Acknowledge();
	segment.advertised_window = header.AdvertisedWindow();
	segment.urgent_offset = header.UrgentOffset();
	segment.flags = header.flags;
	if ((segment.flags & TCP_FLAG_SYNCHRONIZE) != 0) {
		// for now, we only process the options in the SYN segment
		// TODO: when we support timestamps, they could be handled specifically
		process_options(segment, buffer, headerLength - sizeof(tcp_header));
	}

	bufferHeader.Remove(headerLength);
		// we no longer need to keep the header around

	RecursiveLocker locker(gEndpointManager->Locker());
	int32 segmentAction = DROP;
 
	TCPEndpoint *endpoint = gEndpointManager->FindConnection(
		(struct sockaddr *)&buffer->destination, (struct sockaddr *)&buffer->source);
	if (endpoint != NULL) {
		RecursiveLocker locker(endpoint->Lock());

		switch (endpoint->State()) {
			case TIME_WAIT:
				segmentAction |= IMMEDIATE_ACKNOWLEDGE;
			case CLOSED:
				endpoint->UpdateTimeWait();
				break;

			case LISTEN:
				segmentAction = endpoint->ListenReceive(segment, buffer);
				break;

			case SYNCHRONIZE_SENT:
				segmentAction = endpoint->SynchronizeSentReceive(segment, buffer);
				break;

			case SYNCHRONIZE_RECEIVED:
			case ESTABLISHED:
			case FINISH_RECEIVED:
			case WAIT_FOR_FINISH_ACKNOWLEDGE:
			case FINISH_SENT:
			case FINISH_ACKNOWLEDGED:
			case CLOSING:
				segmentAction = endpoint->Receive(segment, buffer);
				break;
		}

		// process acknowledge action as asked for by the *Receive() method
		if (segmentAction & IMMEDIATE_ACKNOWLEDGE)
			endpoint->SendAcknowledge();
		else if (segmentAction & ACKNOWLEDGE)
			endpoint->DelayedAcknowledge();
	} else if ((segment.flags & TCP_FLAG_RESET) == 0)
		segmentAction = DROP | RESET;

	if (segmentAction & RESET) {
		// send reset
		reply_with_reset(segment, buffer);
	}
	if (segmentAction & DROP)
		gBufferModule->free(buffer);

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

	gDomain = NULL;
	gAddressModule = NULL;

	status = get_module(NET_STACK_MODULE_NAME, (module_info **)&gStackModule);
	if (status < B_OK)
		return status;
	status = get_module(NET_BUFFER_MODULE_NAME, (module_info **)&gBufferModule);
	if (status < B_OK)
		goto err1;
	status = get_module(NET_SOCKET_MODULE_NAME, (module_info **)&gSocketModule);
	if (status < B_OK)
		goto err2;
	status = get_module(NET_DATALINK_MODULE_NAME, (module_info **)&gDatalinkModule);
	if (status < B_OK)
		goto err3;

	gEndpointManager = new (std::nothrow) EndpointManager();
	if (gEndpointManager == NULL) {
		status = B_NO_MEMORY;
		goto err4;
	}
	status = gEndpointManager->InitCheck();
	if (status < B_OK)
		goto err5;

	status = gStackModule->register_domain_protocols(AF_INET, SOCK_STREAM, 0,
		"network/protocols/tcp/v1",
		"network/protocols/ipv4/v1",
		NULL);
	if (status < B_OK)
		goto err5;

	status = gStackModule->register_domain_protocols(AF_INET, SOCK_STREAM, IPPROTO_TCP,
		"network/protocols/tcp/v1",
		"network/protocols/ipv4/v1",
		NULL);
	if (status < B_OK)
		goto err5;

	status = gStackModule->register_domain_receiving_protocol(AF_INET, IPPROTO_TCP,
		"network/protocols/tcp/v1");
	if (status < B_OK)
		goto err5;

	return B_OK;

err5:
	delete gEndpointManager;
err4:
	put_module(NET_DATALINK_MODULE_NAME);
err3:
	put_module(NET_SOCKET_MODULE_NAME);
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
	delete gEndpointManager;

	put_module(NET_DATALINK_MODULE_NAME);
	put_module(NET_SOCKET_MODULE_NAME);
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
