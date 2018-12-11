/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Andrew Galante, haiku.galante@gmail.com
 *		Hugo Santos, hugosantos@gmail.com
 */


#include "EndpointManager.h"
#include "TCPEndpoint.h"

#include <net_protocol.h>
#include <net_stat.h>

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


//#define TRACE_TCP
#ifdef TRACE_TCP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif


typedef NetBufferField<uint16, offsetof(tcp_header, checksum)> TCPChecksumField;


net_buffer_module_info *gBufferModule;
net_datalink_module_info *gDatalinkModule;
net_socket_module_info *gSocketModule;
net_stack_module_info *gStackModule;


static EndpointManager* sEndpointManagers[AF_MAX];
static rw_lock sEndpointManagersLock;


// The TCP header length is at most 64 bytes.
static const int kMaxOptionSize = 64 - sizeof(tcp_header);


/*!	Returns an endpoint manager for the specified domain, if any.
	You need to hold the sEndpointManagersLock when calling this function.
*/
static inline EndpointManager*
endpoint_manager_for_locked(int family)
{
	if (family >= AF_MAX || family < 0)
		return NULL;

	return sEndpointManagers[family];
}


/*!	Returns an endpoint manager for the specified domain, if any */
static inline EndpointManager*
endpoint_manager_for(net_domain* domain)
{
	ReadLocker _(sEndpointManagersLock);

	return endpoint_manager_for_locked(domain->family);
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

	if (segment.max_segment_size > 0 && length + 8 <= bufferSize) {
		option->kind = TCP_OPTION_MAX_SEGMENT_SIZE;
		option->length = 4;
		option->max_segment_size = htons(segment.max_segment_size);
		bump_option(option, length);
	}

	if ((segment.options & TCP_HAS_TIMESTAMPS) != 0
		&& length + 12 <= bufferSize) {
		// two NOPs so the timestamps get aligned to a 4 byte boundary
		option->kind = TCP_OPTION_NOP;
		bump_option(option, length);
		option->kind = TCP_OPTION_NOP;
		bump_option(option, length);
		option->kind = TCP_OPTION_TIMESTAMP;
		option->length = 10;
		option->timestamp.value = htonl(segment.timestamp_value);
		option->timestamp.reply = htonl(segment.timestamp_reply);
		bump_option(option, length);
	}

	if ((segment.options & TCP_HAS_WINDOW_SCALE) != 0
		&& length + 4 <= bufferSize) {
		// insert one NOP so that the subsequent data is aligned on a 4 byte boundary
		option->kind = TCP_OPTION_NOP;
		bump_option(option, length);

		option->kind = TCP_OPTION_WINDOW_SHIFT;
		option->length = 3;
		option->window_shift = segment.window_shift;
		bump_option(option, length);
	}

	if ((segment.options & TCP_SACK_PERMITTED) != 0
		&& length + 2 <= bufferSize) {
		option->kind = TCP_OPTION_SACK_PERMITTED;
		option->length = 2;
		bump_option(option, length);
	}

	if (segment.sack_count > 0) {
		int sackCount = ((int)(bufferSize - length) - 4) / sizeof(tcp_sack);
		if (sackCount > segment.sack_count)
			sackCount = segment.sack_count;

		if (sackCount > 0) {
			option->kind = TCP_OPTION_NOP;
			bump_option(option, length);
			option->kind = TCP_OPTION_NOP;
			bump_option(option, length);
			option->kind = TCP_OPTION_SACK;
			option->length = 2 + sackCount * sizeof(tcp_sack);
			memcpy(option->sack, segment.sacks, sackCount * sizeof(tcp_sack));
			bump_option(option, length);
		}
	}

	if ((length & 3) == 0) {
		// options completely fill out the option space
		return length;
	}

	option->kind = TCP_OPTION_END;
	return (length + 3) & ~3;
		// bump to a multiple of 4 length
}


static void
process_options(tcp_segment_header &segment, net_buffer *buffer, size_t size)
{
	if (size == 0)
		return;

	tcp_option *option;

	uint8 optionsBuffer[kMaxOptionSize];
	if (gBufferModule->direct_access(buffer, sizeof(tcp_header), size,
			(void **)&option) != B_OK) {
		if ((size_t)size > sizeof(optionsBuffer)) {
			dprintf("Ignoring TCP options larger than expected.\n");
			return;
		}

		gBufferModule->read(buffer, sizeof(tcp_header), optionsBuffer, size);
		option = (tcp_option *)optionsBuffer;
	}

	while (size > 0) {
		int32 length = -1;

		switch (option->kind) {
			case TCP_OPTION_END:
			case TCP_OPTION_NOP:
				length = 1;
				break;
			case TCP_OPTION_MAX_SEGMENT_SIZE:
				if (option->length == 4 && size >= 4)
					segment.max_segment_size = ntohs(option->max_segment_size);
				break;
			case TCP_OPTION_WINDOW_SHIFT:
				if (option->length == 3 && size >= 3) {
					segment.options |= TCP_HAS_WINDOW_SCALE;
					segment.window_shift = option->window_shift;
				}
				break;
			case TCP_OPTION_TIMESTAMP:
				if (option->length == 10 && size >= 10) {
					segment.options |= TCP_HAS_TIMESTAMPS;
					segment.timestamp_value = ntohl(option->timestamp.value);
					segment.timestamp_reply =
						ntohl(option->timestamp.reply);
				}
				break;
			case TCP_OPTION_SACK_PERMITTED:
				if (option->length == 2 && size >= 2)
					segment.options |= TCP_SACK_PERMITTED;
				break;
		}

		if (length < 0) {
			length = option->length;
			if (length == 0 || length > (ssize_t)size)
				break;
		}

		option = (tcp_option *)((uint8 *)option + length);
		size -= length;
	}
}


#if 0
static void
dump_tcp_header(tcp_header &header)
{
	dprintf("  source port: %u\n", ntohs(header.source_port));
	dprintf("  dest port: %u\n", ntohs(header.destination_port));
	dprintf("  sequence: %lu\n", header.Sequence());
	dprintf("  ack: %lu\n", header.Acknowledge());
	dprintf("  flags: %s%s%s%s%s%s\n", (header.flags & TCP_FLAG_FINISH) ? "FIN " : "",
		(header.flags & TCP_FLAG_SYNCHRONIZE) ? "SYN " : "",
		(header.flags & TCP_FLAG_RESET) ? "RST " : "",
		(header.flags & TCP_FLAG_PUSH) ? "PUSH " : "",
		(header.flags & TCP_FLAG_ACKNOWLEDGE) ? "ACK " : "",
		(header.flags & TCP_FLAG_URGENT) ? "URG " : "");
	dprintf("  window: %u\n", header.AdvertisedWindow());
	dprintf("  urgent offset: %u\n", header.UrgentOffset());
}
#endif


static int
dump_endpoints(int argc, char** argv)
{
	for (int i = 0; i < AF_MAX; i++) {
		EndpointManager* manager = sEndpointManagers[i];
		if (manager != NULL)
			manager->Dump();
	}

	return 0;
}


static int
dump_endpoint(int argc, char** argv)
{
	if (argc < 2) {
		kprintf("usage: tcp_endpoint [address]\n");
		return 0;
	}

	TCPEndpoint* endpoint = (TCPEndpoint*)parse_expression(argv[1]);
	endpoint->Dump();

	return 0;
}


//	#pragma mark - internal API


/*!	Creates a new endpoint manager for the specified domain, or returns
	an existing one for this domain.
*/
EndpointManager*
get_endpoint_manager(net_domain* domain)
{
	// See if there is one already
	EndpointManager* endpointManager = endpoint_manager_for(domain);
	if (endpointManager != NULL)
		return endpointManager;

	WriteLocker _(sEndpointManagersLock);

	endpointManager = endpoint_manager_for_locked(domain->family);
	if (endpointManager != NULL)
		return endpointManager;

	// There is no endpoint manager for this domain yet, so we need
	// to create one.

	endpointManager = new(std::nothrow) EndpointManager(domain);
	if (endpointManager == NULL)
		return NULL;

	if (endpointManager->Init() != B_OK) {
		delete endpointManager;
		return NULL;
	}

	sEndpointManagers[domain->family] = endpointManager;
	return endpointManager;
}


void
put_endpoint_manager(EndpointManager* endpointManager)
{
	// TODO: we may want to use reference counting instead of only discarding
	// them on unload. But since there is likely only IPv4/v6 there is not much
	// point to it.
}


const char*
name_for_state(tcp_state state)
{
	switch (state) {
		case CLOSED:
			return "closed";
		case LISTEN:
			return "listen";
		case SYNCHRONIZE_SENT:
			return "syn-sent";
		case SYNCHRONIZE_RECEIVED:
			return "syn-received";
		case ESTABLISHED:
			return "established";

		// peer closes the connection
		case FINISH_RECEIVED:
			return "close-wait";
		case WAIT_FOR_FINISH_ACKNOWLEDGE:
			return "last-ack";

		// we close the connection
		case FINISH_SENT:
			return "fin-wait1";
		case FINISH_ACKNOWLEDGED:
			return "fin-wait2";
		case CLOSING:
			return "closing";

		case TIME_WAIT:
			return "time-wait";
	}

	return "-";
}


/*!	Constructs a TCP header on \a buffer with the specified values
	for \a flags, \a seq \a ack and \a advertisedWindow.
*/
status_t
add_tcp_header(net_address_module_info* addressModule,
	tcp_segment_header& segment, net_buffer* buffer)
{
	buffer->protocol = IPPROTO_TCP;

	uint8 optionsBuffer[kMaxOptionSize];
	uint32 optionsLength = add_options(segment, optionsBuffer,
		sizeof(optionsBuffer));

	NetBufferPrepend<tcp_header> bufferHeader(buffer,
		sizeof(tcp_header) + optionsLength);
	if (bufferHeader.Status() != B_OK)
		return bufferHeader.Status();

	tcp_header& header = bufferHeader.Data();

	header.source_port = addressModule->get_port(buffer->source);
	header.destination_port = addressModule->get_port(buffer->destination);
	header.sequence = htonl(segment.sequence);
	header.acknowledge = (segment.flags & TCP_FLAG_ACKNOWLEDGE)
		? htonl(segment.acknowledge) : 0;
	header.reserved = 0;
	header.header_length = (sizeof(tcp_header) + optionsLength) >> 2;
	header.flags = segment.flags;
	header.advertised_window = htons(segment.advertised_window);
	header.checksum = 0;
	header.urgent_offset = htons(segment.urgent_offset);

	// we must detach before calculating the checksum as we may
	// not have a contiguous buffer.
	bufferHeader.Sync();

	if (optionsLength > 0) {
		gBufferModule->write(buffer, sizeof(tcp_header), optionsBuffer,
			optionsLength);
	}

	TRACE(("add_tcp_header(): buffer %p, flags 0x%x, seq %lu, ack %lu, up %u, "
		"win %u\n", buffer, segment.flags, segment.sequence,
		segment.acknowledge, segment.urgent_offset, segment.advertised_window));

	*TCPChecksumField(buffer) = Checksum::PseudoHeader(addressModule,
		gBufferModule, buffer, IPPROTO_TCP);

	return B_OK;
}


size_t
tcp_options_length(tcp_segment_header& segment)
{
	size_t length = 0;

	if (segment.max_segment_size > 0)
		length += 4;

	if (segment.options & TCP_HAS_TIMESTAMPS)
		length += 12;

	if (segment.options & TCP_HAS_WINDOW_SCALE)
		length += 4;

	if (segment.options & TCP_SACK_PERMITTED)
		length += 2;

	if (segment.sack_count > 0) {
		int sackCount = min_c((int)((kMaxOptionSize - length - 4)
			/ sizeof(tcp_sack)), segment.sack_count);
		if (sackCount > 0)
			length += 4 + sackCount * sizeof(tcp_sack);
	}

	if ((length & 3) == 0)
		return length;

	return (length + 3) & ~3;
}


//	#pragma mark - protocol API


net_protocol*
tcp_init_protocol(net_socket* socket)
{
	socket->send.buffer_size = 32768;
		// override net_socket default

	TCPEndpoint* protocol = new (std::nothrow) TCPEndpoint(socket);
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
tcp_uninit_protocol(net_protocol* protocol)
{
	TRACE(("Deleting TCPEndpoint: %p\n", protocol));
	delete (TCPEndpoint*)protocol;
	return B_OK;
}


status_t
tcp_open(net_protocol* protocol)
{
	return ((TCPEndpoint*)protocol)->Open();
}


status_t
tcp_close(net_protocol* protocol)
{
	return ((TCPEndpoint*)protocol)->Close();
}


status_t
tcp_free(net_protocol* protocol)
{
	((TCPEndpoint*)protocol)->Free();
	return B_OK;
}


status_t
tcp_connect(net_protocol* protocol, const struct sockaddr* address)
{
	return ((TCPEndpoint*)protocol)->Connect(address);
}


status_t
tcp_accept(net_protocol* protocol, struct net_socket** _acceptedSocket)
{
	return ((TCPEndpoint*)protocol)->Accept(_acceptedSocket);
}


status_t
tcp_control(net_protocol* _protocol, int level, int option, void* value,
	size_t* _length)
{
	TCPEndpoint* protocol = (TCPEndpoint*)_protocol;

	if ((level & LEVEL_MASK) == IPPROTO_TCP) {
		if (option == NET_STAT_SOCKET)
			return protocol->FillStat((net_stat*)value);
	}

	return protocol->next->module->control(protocol->next, level, option,
		value, _length);
}


status_t
tcp_getsockopt(net_protocol* _protocol, int level, int option, void* value,
	int* _length)
{
	TCPEndpoint* protocol = (TCPEndpoint*)_protocol;

	if (level == IPPROTO_TCP)
		return protocol->GetOption(option, value, _length);

	return protocol->next->module->getsockopt(protocol->next, level, option,
		value, _length);
}


status_t
tcp_setsockopt(net_protocol* _protocol, int level, int option,
	const void* _value, int length)
{
	TCPEndpoint* protocol = (TCPEndpoint*)_protocol;

	if (level == SOL_SOCKET) {
		if (option == SO_SNDBUF || option == SO_RCVBUF) {
			if (length != sizeof(int))
				return B_BAD_VALUE;

			status_t status;
			const int* value = (const int*)_value;

			if (option == SO_SNDBUF)
				status = protocol->SetSendBufferSize(*value);
			else
				status = protocol->SetReceiveBufferSize(*value);

			if (status < B_OK)
				return status;
		}
	} else if (level == IPPROTO_TCP)
		return protocol->SetOption(option, _value, length);

	return protocol->next->module->setsockopt(protocol->next, level, option,
		_value, length);
}


status_t
tcp_bind(net_protocol* protocol, const struct sockaddr* address)
{
	return ((TCPEndpoint*)protocol)->Bind(address);
}


status_t
tcp_unbind(net_protocol* protocol, struct sockaddr* address)
{
	return ((TCPEndpoint*)protocol)->Unbind(address);
}


status_t
tcp_listen(net_protocol* protocol, int count)
{
	return ((TCPEndpoint*)protocol)->Listen(count);
}


status_t
tcp_shutdown(net_protocol* protocol, int direction)
{
	return ((TCPEndpoint*)protocol)->Shutdown(direction);
}


status_t
tcp_send_data(net_protocol* protocol, net_buffer* buffer)
{
	return ((TCPEndpoint*)protocol)->SendData(buffer);
}


status_t
tcp_send_routed_data(net_protocol* protocol, struct net_route* route,
	net_buffer* buffer)
{
	// TCP never sends routed data
	return B_ERROR;
}


ssize_t
tcp_send_avail(net_protocol* protocol)
{
	return ((TCPEndpoint*)protocol)->SendAvailable();
}


status_t
tcp_read_data(net_protocol* protocol, size_t numBytes, uint32 flags,
	net_buffer** _buffer)
{
	return ((TCPEndpoint*)protocol)->ReadData(numBytes, flags, _buffer);
}


ssize_t
tcp_read_avail(net_protocol* protocol)
{
	return ((TCPEndpoint*)protocol)->ReadAvailable();
}


struct net_domain*
tcp_get_domain(net_protocol* protocol)
{
	return protocol->next->module->get_domain(protocol->next);
}


size_t
tcp_get_mtu(net_protocol* protocol, const struct sockaddr* address)
{
	return protocol->next->module->get_mtu(protocol->next, address);
}


status_t
tcp_receive_data(net_buffer* buffer)
{
	TRACE(("TCP: Received buffer %p\n", buffer));

	if (buffer->interface_address == NULL
		|| buffer->interface_address->domain == NULL)
		return B_ERROR;

	net_domain* domain = buffer->interface_address->domain;
	net_address_module_info* addressModule = domain->address_module;

	NetBufferHeaderReader<tcp_header> bufferHeader(buffer);
	if (bufferHeader.Status() < B_OK)
		return bufferHeader.Status();

	tcp_header& header = bufferHeader.Data();

	uint16 headerLength = header.HeaderLength();
	if (headerLength < sizeof(tcp_header))
		return B_BAD_DATA;

	if (Checksum::PseudoHeader(addressModule, gBufferModule, buffer,
			IPPROTO_TCP) != 0)
		return B_BAD_DATA;

	addressModule->set_port(buffer->source, header.source_port);
	addressModule->set_port(buffer->destination, header.destination_port);

	TRACE(("  Looking for: peer %s, local %s\n",
		AddressString(domain, buffer->source, true).Data(),
		AddressString(domain, buffer->destination, true).Data()));
	//dump_tcp_header(header);
	//gBufferModule->dump(buffer);

	tcp_segment_header segment(header.flags);
	segment.sequence = header.Sequence();
	segment.acknowledge = header.Acknowledge();
	segment.advertised_window = header.AdvertisedWindow();
	segment.urgent_offset = header.UrgentOffset();
	process_options(segment, buffer, headerLength - sizeof(tcp_header));

	bufferHeader.Remove(headerLength);
		// we no longer need to keep the header around

	EndpointManager* endpointManager = endpoint_manager_for(domain);
	if (endpointManager == NULL) {
		TRACE(("  No endpoint manager!\n"));
		return B_ERROR;
	}

	int32 segmentAction = DROP;

	TCPEndpoint* endpoint = endpointManager->FindConnection(
		buffer->destination, buffer->source);
	if (endpoint != NULL) {
		segmentAction = endpoint->SegmentReceived(segment, buffer);
		gSocketModule->release_socket(endpoint->socket);
	} else if ((segment.flags & TCP_FLAG_RESET) == 0)
		segmentAction = DROP | RESET;

	if ((segmentAction & RESET) != 0) {
		// send reset
		endpointManager->ReplyWithReset(segment, buffer);
	}
	if ((segmentAction & DROP) != 0)
		gBufferModule->free(buffer);

	return B_OK;
}


status_t
tcp_error_received(net_error error, net_buffer* data)
{
	return B_ERROR;
}


status_t
tcp_error_reply(net_protocol* protocol, net_buffer* cause, net_error error,
	net_error_data* errorData)
{
	return B_ERROR;
}


//	#pragma mark -


static status_t
tcp_init()
{
	rw_lock_init(&sEndpointManagersLock, "endpoint managers");

	status_t status = gStackModule->register_domain_protocols(AF_INET,
		SOCK_STREAM, 0,
		"network/protocols/tcp/v1",
		"network/protocols/ipv4/v1",
		NULL);
	if (status < B_OK)
		return status;
	status = gStackModule->register_domain_protocols(AF_INET6,
		SOCK_STREAM, 0,
		"network/protocols/tcp/v1",
		"network/protocols/ipv6/v1",
		NULL);
	if (status < B_OK)
		return status;

	status = gStackModule->register_domain_protocols(AF_INET, SOCK_STREAM,
		IPPROTO_TCP,
		"network/protocols/tcp/v1",
		"network/protocols/ipv4/v1",
		NULL);
	if (status < B_OK)
		return status;
	status = gStackModule->register_domain_protocols(AF_INET6, SOCK_STREAM,
		IPPROTO_TCP,
		"network/protocols/tcp/v1",
		"network/protocols/ipv6/v1",
		NULL);
	if (status < B_OK)
		return status;

	status = gStackModule->register_domain_receiving_protocol(AF_INET,
		IPPROTO_TCP, "network/protocols/tcp/v1");
	if (status < B_OK)
		return status;
	status = gStackModule->register_domain_receiving_protocol(AF_INET6,
		IPPROTO_TCP, "network/protocols/tcp/v1");
	if (status < B_OK)
		return status;

	add_debugger_command("tcp_endpoints", dump_endpoints,
		"lists all open TCP endpoints");
	add_debugger_command("tcp_endpoint", dump_endpoint,
		"dumps a TCP endpoint internal state");

	return B_OK;
}


static status_t
tcp_uninit()
{
	remove_debugger_command("tcp_endpoint", dump_endpoint);
	remove_debugger_command("tcp_endpoints", dump_endpoints);

	rw_lock_destroy(&sEndpointManagersLock);

	for (int i = 0; i < AF_MAX; i++) {
		delete sEndpointManagers[i];
	}

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
	0,

	tcp_init_protocol,
	tcp_uninit_protocol,
	tcp_open,
	tcp_close,
	tcp_free,
	tcp_connect,
	tcp_accept,
	tcp_control,
	tcp_getsockopt,
	tcp_setsockopt,
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
	NULL,		// deliver_data()
	tcp_error_received,
	tcp_error_reply,
	NULL,		// add_ancillary_data()
	NULL,		// process_ancillary_data()
	NULL,		// process_ancillary_data_no_container()
	NULL,		// send_data_no_buffer()
	NULL		// read_data_no_buffer()
};

module_dependency module_dependencies[] = {
	{NET_STACK_MODULE_NAME, (module_info **)&gStackModule},
	{NET_BUFFER_MODULE_NAME, (module_info **)&gBufferModule},
	{NET_DATALINK_MODULE_NAME, (module_info **)&gDatalinkModule},
	{NET_SOCKET_MODULE_NAME, (module_info **)&gSocketModule},
	{}
};

module_info *modules[] = {
	(module_info *)&sTCPModule,
	NULL
};
