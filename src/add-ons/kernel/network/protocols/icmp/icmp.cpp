/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <net_datalink.h>
#include <net_protocol.h>
#include <net_stack.h>
#include <NetBufferUtilities.h>

#include <KernelExport.h>
#include <util/list.h>

#include <netinet/in.h>
#include <new>
#include <stdlib.h>
#include <string.h>

//#define TRACE_ICMP
#ifdef TRACE_ICMP
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
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
		uint32 zero;
	};
};

typedef NetBufferField<uint16, offsetof(icmp_header, checksum)> ICMPChecksumField;

#define ICMP_TYPE_ECHO_REPLY	0
#define ICMP_TYPE_UNREACH		3
#define ICMP_TYPE_REDIRECT		5
#define ICMP_TYPE_ECHO_REQUEST	8

// type unreach codes
#define ICMP_CODE_UNREACH_NEED_FRAGMENT	4	// this is used for path MTU discovery

struct icmp_protocol : net_protocol {
};


net_buffer_module_info *gBufferModule;
static net_stack_module_info *sStackModule;


net_protocol *
icmp_init_protocol(net_socket *socket)
{
	icmp_protocol *protocol = new (std::nothrow) icmp_protocol;
	if (protocol == NULL)
		return NULL;

	return protocol;
}


status_t
icmp_uninit_protocol(net_protocol *protocol)
{
	delete protocol;
	return B_OK;
}


status_t
icmp_open(net_protocol *protocol)
{
	return B_OK;
}


status_t
icmp_close(net_protocol *protocol)
{
	return B_OK;
}


status_t
icmp_free(net_protocol *protocol)
{
	return B_OK;
}


status_t
icmp_connect(net_protocol *protocol, const struct sockaddr *address)
{
	return B_ERROR;
}


status_t
icmp_accept(net_protocol *protocol, struct net_socket **_acceptedSocket)
{
	return EOPNOTSUPP;
}


status_t
icmp_control(net_protocol *protocol, int level, int option, void *value,
	size_t *_length)
{
	return protocol->next->module->control(protocol->next, level, option,
		value, _length);
}


status_t
icmp_bind(net_protocol *protocol, struct sockaddr *address)
{
	return B_ERROR;
}


status_t
icmp_unbind(net_protocol *protocol, struct sockaddr *address)
{
	return B_ERROR;
}


status_t
icmp_listen(net_protocol *protocol, int count)
{
	return EOPNOTSUPP;
}


status_t
icmp_shutdown(net_protocol *protocol, int direction)
{
	return EOPNOTSUPP;
}


status_t
icmp_send_data(net_protocol *protocol, net_buffer *buffer)
{
	return protocol->next->module->send_data(protocol->next, buffer);
}


status_t
icmp_send_routed_data(net_protocol *protocol, struct net_route *route,
	net_buffer *buffer)
{
	return protocol->next->module->send_routed_data(protocol->next, route, buffer);
}


ssize_t
icmp_send_avail(net_protocol *protocol)
{
	return B_ERROR;
}


status_t
icmp_read_data(net_protocol *protocol, size_t numBytes, uint32 flags,
	net_buffer **_buffer)
{
	return B_ERROR;
}


ssize_t
icmp_read_avail(net_protocol *protocol)
{
	return B_ERROR;
}


struct net_domain *
icmp_get_domain(net_protocol *protocol)
{
	return protocol->next->module->get_domain(protocol->next);
}


size_t
icmp_get_mtu(net_protocol *protocol, const struct sockaddr *address)
{
	return protocol->next->module->get_mtu(protocol->next, address);
}


status_t
icmp_receive_data(net_buffer *buffer)
{
	TRACE(("ICMP received some data, buffer length %lu\n", buffer->size));

	NetBufferHeaderReader<icmp_header> bufferHeader(buffer);
	if (bufferHeader.Status() < B_OK)
		return bufferHeader.Status();

	icmp_header &header = bufferHeader.Data();

	TRACE(("  got type %u, code %u, checksum %u\n", header.type, header.code,
		ntohs(header.checksum)));
	TRACE(("  computed checksum: %ld\n", gBufferModule->checksum(buffer, 0, buffer->size, true)));

	if (gBufferModule->checksum(buffer, 0, buffer->size, true) != 0)
		return B_BAD_DATA;

	switch (header.type) {
		case ICMP_TYPE_ECHO_REPLY:
			break;

		case ICMP_TYPE_ECHO_REQUEST:
		{
			net_domain *domain;
			if (buffer->interface != NULL)
				domain = buffer->interface->domain;
			else
				domain = sStackModule->get_domain(buffer->source.ss_family);
			if (domain == NULL || domain->module == NULL)
				break;

			net_buffer *reply = gBufferModule->duplicate(buffer);
			if (reply == NULL)
				return B_NO_MEMORY;

			// switch source/destination address
			memcpy(&reply->source, &buffer->destination, buffer->destination.ss_len);
			memcpy(&reply->destination, &buffer->source, buffer->source.ss_len);

			// There already is an ICMP header, and we'll reuse it
			NetBufferHeaderReader<icmp_header> header(reply);

			header->type = ICMP_TYPE_ECHO_REPLY;
			header->code = 0;
			header->checksum = 0;

			header.Sync();

			*ICMPChecksumField(reply) = gBufferModule->checksum(reply, 0,
					reply->size, true);

			status_t status = domain->module->send_data(NULL, reply);
			if (status < B_OK) {
				gBufferModule->free(reply);
				return status;
			}
		}
	}

	gBufferModule->free(buffer);
	return B_OK;
}


status_t
icmp_error(uint32 code, net_buffer *data)
{
	return B_ERROR;
}


status_t
icmp_error_reply(net_protocol *protocol, net_buffer *causedError, uint32 code,
	void *errorData)
{
	return B_ERROR;
}


//	#pragma mark -


static status_t
icmp_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			sStackModule->register_domain_protocols(AF_INET, SOCK_DGRAM, IPPROTO_ICMP,
				"network/protocols/icmp/v1",
				"network/protocols/ipv4/v1",
				NULL);

			sStackModule->register_domain_receiving_protocol(AF_INET, IPPROTO_ICMP,
				"network/protocols/icmp/v1");
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
	icmp_init_protocol,
	icmp_uninit_protocol,
	icmp_open,
	icmp_close,
	icmp_free,
	icmp_connect,
	icmp_accept,
	icmp_control,
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
	NULL,
	icmp_error,
	icmp_error_reply,
};

module_dependency module_dependencies[] = {
	{NET_STACK_MODULE_NAME, (module_info **)&sStackModule},
	{NET_BUFFER_MODULE_NAME, (module_info **)&gBufferModule},
	{}
};

module_info *modules[] = {
	(module_info *)&sICMPModule,
	NULL
};
