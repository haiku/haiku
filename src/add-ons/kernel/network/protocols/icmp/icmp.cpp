/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


/*!	RFC 792 details the ICMP protocol, RFC 1122 lists when an ICMP error must,
	shall, or must not be sent.
*/


#include <algorithm>
#include <netinet/in.h>
#include <new>
#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>
#include <OS.h>

#include <icmp.h>
#include <net_datalink.h>
#include <net_protocol.h>
#include <net_stack.h>
#include <NetBufferUtilities.h>

//#include <util/list.h>

#include "ipv4.h"


#define TRACE_ICMP
#ifdef TRACE_ICMP
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


struct icmp_header {
	uint8	type;
	uint8	code;
	uint16	checksum;
	union {
		struct {
			uint16	id;
			uint16	sequence;
		} echo;
		struct {
			in_addr_t gateway;
		} redirect;
		struct {
			uint16	_reserved;
			uint16	next_mtu;
		} path_mtu;
		uint32	gateway;
		struct {
			uint16 unused;
			uint16 mtu;
		} frag;

		uint32 zero;
	};
};

typedef NetBufferField<uint16, offsetof(icmp_header, checksum)>
	ICMPChecksumField;


#define ICMP_TYPE_ECHO_REPLY	0
#define ICMP_TYPE_UNREACH		3
#define ICMP_TYPE_REDIRECT		5
#define ICMP_TYPE_ECHO_REQUEST	8

// type unreach codes
#define ICMP_CODE_UNREACH_NEED_FRAGMENT	4
	// this is used for path MTU discovery


struct icmp_protocol : net_protocol {
};


net_buffer_module_info* gBufferModule;
static net_stack_module_info* sStackModule;


static net_domain*
get_domain(struct net_buffer* buffer)
{
	net_domain* domain;
	if (buffer->interface != NULL)
		domain = buffer->interface->domain;
	else
		domain = sStackModule->get_domain(buffer->source->sa_family);

	if (domain == NULL || domain->module == NULL)
		return NULL;

	return domain;
}


static bool
is_icmp_error(uint8 type)
{
	return type == ICMP_TYPE_UNREACH
		|| type == ICMP_TYPE_PARAM_PROBLEM
		|| type == ICMP_TYPE_REDIRECT
		|| type == ICMP_TYPE_TIME_EXCEEDED
		|| type == ICMP_TYPE_SOURCE_QUENCH;
}


// #pragma mark - module API


net_protocol*
icmp_init_protocol(net_socket* socket)
{
	icmp_protocol* protocol = new(std::nothrow) icmp_protocol;
	if (protocol == NULL)
		return NULL;

	return protocol;
}


status_t
icmp_uninit_protocol(net_protocol* protocol)
{
	delete protocol;
	return B_OK;
}


status_t
icmp_open(net_protocol* protocol)
{
	return B_OK;
}


status_t
icmp_close(net_protocol* protocol)
{
	return B_OK;
}


status_t
icmp_free(net_protocol* protocol)
{
	return B_OK;
}


status_t
icmp_connect(net_protocol* protocol, const struct sockaddr* address)
{
	return B_ERROR;
}


status_t
icmp_accept(net_protocol* protocol, struct net_socket** _acceptedSocket)
{
	return EOPNOTSUPP;
}


status_t
icmp_control(net_protocol* protocol, int level, int option, void* value,
	size_t* _length)
{
	return protocol->next->module->control(protocol->next, level, option,
		value, _length);
}


status_t
icmp_getsockopt(net_protocol* protocol, int level, int option, void* value,
	int* length)
{
	return protocol->next->module->getsockopt(protocol->next, level, option,
		value, length);
}


status_t
icmp_setsockopt(net_protocol* protocol, int level, int option,
	const void* value, int length)
{
	return protocol->next->module->setsockopt(protocol->next, level, option,
		value, length);
}


status_t
icmp_bind(net_protocol* protocol, const struct sockaddr* address)
{
	return B_ERROR;
}


status_t
icmp_unbind(net_protocol* protocol, struct sockaddr* address)
{
	return B_ERROR;
}


status_t
icmp_listen(net_protocol* protocol, int count)
{
	return EOPNOTSUPP;
}


status_t
icmp_shutdown(net_protocol* protocol, int direction)
{
	return EOPNOTSUPP;
}


status_t
icmp_send_data(net_protocol* protocol, net_buffer* buffer)
{
	return protocol->next->module->send_data(protocol->next, buffer);
}


status_t
icmp_send_routed_data(net_protocol* protocol, struct net_route* route,
	net_buffer* buffer)
{
	return protocol->next->module->send_routed_data(protocol->next, route,
		buffer);
}


ssize_t
icmp_send_avail(net_protocol* protocol)
{
	return B_ERROR;
}


status_t
icmp_read_data(net_protocol* protocol, size_t numBytes, uint32 flags,
	net_buffer** _buffer)
{
	return B_ERROR;
}


ssize_t
icmp_read_avail(net_protocol* protocol)
{
	return B_ERROR;
}


struct net_domain* 
icmp_get_domain(net_protocol* protocol)
{
	return protocol->next->module->get_domain(protocol->next);
}


size_t
icmp_get_mtu(net_protocol* protocol, const struct sockaddr* address)
{
	return protocol->next->module->get_mtu(protocol->next, address);
}


status_t
icmp_receive_data(net_buffer* buffer)
{
	TRACE("ICMP received some data, buffer length %lu\n", buffer->size);

	net_domain* domain;
	if (buffer->interface != NULL)
		domain = buffer->interface->domain;
	else
		domain = sStackModule->get_domain(buffer->source->sa_family);

	if (domain == NULL || domain->module == NULL)
		return B_ERROR;

	NetBufferHeaderReader<icmp_header> bufferHeader(buffer);
	if (bufferHeader.Status() < B_OK)
		return bufferHeader.Status();

	icmp_header& header = bufferHeader.Data();
	uint8 type = header.type;

	TRACE("  got type %u, code %u, checksum %u\n", header.type, header.code,
		ntohs(header.checksum));
	TRACE("  computed checksum: %ld\n",
		gBufferModule->checksum(buffer, 0, buffer->size, true));

	if (gBufferModule->checksum(buffer, 0, buffer->size, true) != 0)
		return B_BAD_DATA;

	switch (type) {
		case ICMP_TYPE_ECHO_REPLY:
			break;

		case ICMP_TYPE_ECHO_REQUEST:
		{
			net_domain* domain = get_domain(buffer);
			if (domain == NULL)
				break;

			if (buffer->interface != NULL) {
				// We only reply to echo requests of our local interface; we
				// don't reply to broadcast requests
				if (!domain->address_module->equal_addresses(
						buffer->interface->address, buffer->destination))
					break;
			}

			net_buffer* reply = gBufferModule->duplicate(buffer);
			if (reply == NULL)
				return B_NO_MEMORY;

			gBufferModule->swap_addresses(reply);

			// There already is an ICMP header, and we'll reuse it
			NetBufferHeaderReader<icmp_header> newHeader(reply);

			newHeader->type = type == ICMP_TYPE_ECHO_REPLY;
			newHeader->code = 0;
			newHeader->checksum = 0;

			newHeader.Sync();

			*ICMPChecksumField(reply) = gBufferModule->checksum(reply, 0,
					reply->size, true);

			status_t status = domain->module->send_data(NULL, reply);
			if (status < B_OK) {
				gBufferModule->free(reply);
				return status;
			}
			break;
		}

		case ICMP_TYPE_UNREACH:
		case ICMP_TYPE_SOURCE_QUENCH:
		case ICMP_TYPE_PARAM_PROBLEM:
		case ICMP_TYPE_TIME_EXCEEDED:
		{
			net_domain* domain = get_domain(buffer);
			if (domain == NULL)
				break;

			uint32 error = icmp_encode(header.type, header.code);
			if (error > 0) {
				// Deliver the error to the domain protocol which will
				// propagate the error to the upper protocols
				return domain->module->error_received(error, buffer);
			}
			break;
		}

		case ICMP_TYPE_REDIRECT:
			// TODO: Update the routing table
		case ICMP_TYPE_TIMESTAMP_REQUEST:
		case ICMP_TYPE_TIMESTAMP_REPLY:
		case ICMP_TYPE_INFO_REQUEST:
		case ICMP_TYPE_INFO_REPLY:
		default:
			// RFC 1122 3.2.2:
			// Unknown ICMP messages are silently discarded
			dprintf("ICMP: received unhandled type %u, code %u\n", header.type,
				header.code);
			break;
	}

	gBufferModule->free(buffer);
	return B_OK;
}


status_t
icmp_error_received(uint32 code, net_buffer* data)
{
	return B_ERROR;
}


/*!	Sends an ICMP error message to the source of the \a buffer causing the
	error.
*/
status_t
icmp_error_reply(net_protocol* protocol, net_buffer* buffer, uint32 code,
	void* errorData)
{
	TRACE("icmp_error_reply(code %#" B_PRIx32 ")\n", code);

	uint8 icmpType, icmpCode;
	icmp_decode(code, icmpType, icmpCode);

	NetBufferHeaderReader<ipv4_header> bufferHeader(buffer);
	size_t offset = 0;

	// TODO: get rid of network_header
	ipv4_header* header = (ipv4_header*)buffer->network_header;
	if (header == NULL) {
		if (bufferHeader.Status() != B_OK)
			return bufferHeader.Status();

		header = &bufferHeader.Data();
		offset = header->HeaderLength();
	}

	char originalData[64];
	size_t originalSize
		= std::min(buffer->size - offset, sizeof(originalData));
	if (originalSize < 8) {
		// We need at least 8 bytes of data of the original data
		return B_ERROR;
	}

	status_t status = gBufferModule->read(buffer, offset, originalData,
		originalSize);
	if (status != B_OK)
		return status;

	// RFC 1122 3.2.2:
	// ICMP error message should not be sent on reception of
	// an ICMP error message,
	if (header->protocol == IPPROTO_ICMP) {
		icmp_header* icmpHeader = (icmp_header*)originalData;
		if (is_icmp_error(icmpHeader->code))
			return B_ERROR;
	}

	// a datagram to an IP multicast or broadcast address,
	if ((buffer->flags & (MSG_BCAST | MSG_MCAST)) != 0) 
		return B_ERROR;

	// a non-initial fragment
	if ((header->FragmentOffset() & IP_FRAGMENT_OFFSET_MASK) != 0)
		return B_ERROR;

	net_buffer* reply = gBufferModule->create(256);
	if (reply == NULL)
		return B_NO_MEMORY;
		
	memcpy(reply->source, buffer->destination, buffer->destination->sa_len);
	memcpy(reply->destination, buffer->source, buffer->source->sa_len);

	// Now prepare the ICMP header
	NetBufferPrepend<icmp_header> icmpHeader(reply);
	icmpHeader->type = icmpType;
	icmpHeader->code = icmpCode;
	icmpHeader->gateway = errorData ? *((uint32*)errorData) : 0;
	icmpHeader->checksum = 0;
	icmpHeader.Sync();
	*ICMPChecksumField(reply)
		= gBufferModule->checksum(reply, 0, reply->size, true);

	// Append IP header + 64 bits of the original datagram
	gBufferModule->append(reply, header, sizeof(ipv4_header));

	net_domain* domain = get_domain(buffer);
	if (domain == NULL)
		return B_ERROR;

	status = domain->module->send_data(NULL, reply);
	if (status != B_OK)
		gBufferModule->free(reply);

	return status;
}


//	#pragma mark -


static status_t
icmp_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			sStackModule->register_domain_protocols(AF_INET, SOCK_DGRAM,
				IPPROTO_ICMP,
				"network/protocols/icmp/v1",
				"network/protocols/ipv4/v1",
				NULL);

			sStackModule->register_domain_receiving_protocol(AF_INET,
				IPPROTO_ICMP, "network/protocols/icmp/v1");
			return B_OK;
		}

		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


net_protocol_module_info sICMPModule = {
	{
		"network/protocols/icmp/v1",
		0,
		icmp_std_ops
	},
	NET_PROTOCOL_ATOMIC_MESSAGES,

	icmp_init_protocol,
	icmp_uninit_protocol,
	icmp_open,
	icmp_close,
	icmp_free,
	icmp_connect,
	icmp_accept,
	icmp_control,
	icmp_getsockopt,
	icmp_setsockopt,
	icmp_bind,
	icmp_unbind,
	icmp_listen,
	icmp_shutdown,
	icmp_send_data,
	icmp_send_routed_data,
	icmp_send_avail,
	icmp_read_data,
	icmp_read_avail,
	icmp_get_domain,
	icmp_get_mtu,
	icmp_receive_data,
	NULL,		// deliver_data()
	icmp_error_received,
	icmp_error_reply,
	NULL,		// add_ancillary_data()
	NULL,		// process_ancillary_data()
	NULL,		// process_ancillary_data_no_container()
	NULL,		// send_data_no_buffer()
	NULL		// read_data_no_buffer()
};

module_dependency module_dependencies[] = {
	{NET_STACK_MODULE_NAME, (module_info**)&sStackModule},
	{NET_BUFFER_MODULE_NAME, (module_info**)&gBufferModule},
	{}
};

module_info* modules[] = {
	(module_info*)&sICMPModule,
	NULL
};
