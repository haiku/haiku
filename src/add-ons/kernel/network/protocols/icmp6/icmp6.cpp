/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <net_datalink.h>
#include <net_protocol.h>
#include <net_stack.h>
#include <net_datalink_protocol.h>
#include <NetUtilities.h>
#include <NetBufferUtilities.h>

#include <KernelExport.h>
#include <util/list.h>

#include <netinet/icmp6.h>
#include <netinet/in.h>
#include <new>
#include <stdlib.h>
#include <string.h>

#include <ipv6_datagram/ndp.h>


#define TRACE_ICMP6
#ifdef TRACE_ICMP6
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


typedef NetBufferField<uint16, offsetof(icmp6_hdr, icmp6_cksum)> ICMP6ChecksumField;


net_buffer_module_info *gBufferModule;
static net_stack_module_info *sStackModule;
static net_ndp_module_info *sIPv6NDPModule;


net_protocol *
icmp6_init_protocol(net_socket *socket)
{
	net_protocol *protocol = new (std::nothrow) net_protocol;
	if (protocol == NULL)
		return NULL;

	return protocol;
}


status_t
icmp6_uninit_protocol(net_protocol *protocol)
{
	delete protocol;
	return B_OK;
}


status_t
icmp6_open(net_protocol *protocol)
{
	return B_OK;
}


status_t
icmp6_close(net_protocol *protocol)
{
	return B_OK;
}


status_t
icmp6_free(net_protocol *protocol)
{
	return B_OK;
}


status_t
icmp6_connect(net_protocol *protocol, const struct sockaddr *address)
{
	return B_ERROR;
}


status_t
icmp6_accept(net_protocol *protocol, struct net_socket **_acceptedSocket)
{
	return EOPNOTSUPP;
}


status_t
icmp6_control(net_protocol *protocol, int level, int option, void *value,
	size_t *_length)
{
	return protocol->next->module->control(protocol->next, level, option,
		value, _length);
}


status_t
icmp6_getsockopt(net_protocol *protocol, int level, int option,
	void *value, int *length)
{
	return protocol->next->module->getsockopt(protocol->next, level, option,
		value, length);
}


status_t
icmp6_setsockopt(net_protocol *protocol, int level, int option,
	const void *value, int length)
{
	return protocol->next->module->setsockopt(protocol->next, level, option,
		value, length);
}


status_t
icmp6_bind(net_protocol *protocol, const struct sockaddr *address)
{
	return B_ERROR;
}


status_t
icmp6_unbind(net_protocol *protocol, struct sockaddr *address)
{
	return B_ERROR;
}


status_t
icmp6_listen(net_protocol *protocol, int count)
{
	return EOPNOTSUPP;
}


status_t
icmp6_shutdown(net_protocol *protocol, int direction)
{
	return EOPNOTSUPP;
}


status_t
icmp6_send_data(net_protocol *protocol, net_buffer *buffer)
{
	return protocol->next->module->send_data(protocol->next, buffer);
}


status_t
icmp6_send_routed_data(net_protocol *protocol, struct net_route *route,
	net_buffer *buffer)
{
	return protocol->next->module->send_routed_data(protocol->next, route, buffer);
}


ssize_t
icmp6_send_avail(net_protocol *protocol)
{
	return B_ERROR;
}


status_t
icmp6_read_data(net_protocol *protocol, size_t numBytes, uint32 flags,
	net_buffer **_buffer)
{
	return B_ERROR;
}


ssize_t
icmp6_read_avail(net_protocol *protocol)
{
	return B_ERROR;
}


struct net_domain *
icmp6_get_domain(net_protocol *protocol)
{
	return protocol->next->module->get_domain(protocol->next);
}


size_t
icmp6_get_mtu(net_protocol *protocol, const struct sockaddr *address)
{
	return protocol->next->module->get_mtu(protocol->next, address);
}


static net_domain*
get_domain(struct net_buffer* buffer)
{
	net_domain* domain;
	if (buffer->interface_address != NULL)
		domain = buffer->interface_address->domain;
	else
		domain = sStackModule->get_domain(buffer->source->sa_family);

	if (domain == NULL || domain->module == NULL)
		return NULL;

	return domain;
}


status_t
icmp6_receive_data(net_buffer *buffer)
{
	TRACE(("ICMPv6 received some data, buffer length %" B_PRIu32 "\n",
		buffer->size));

	net_domain* domain = get_domain(buffer);
	if (domain == NULL)
		return B_ERROR;

	NetBufferHeaderReader<icmp6_hdr> bufferHeader(buffer);
	if (bufferHeader.Status() < B_OK)
		return bufferHeader.Status();

	icmp6_hdr &header = bufferHeader.Data();

	TRACE(("  got type %u, code %u, checksum 0x%x\n", header.icmp6_type,
			header.icmp6_code, header.icmp6_cksum));

	net_address_module_info* addressModule = domain->address_module;

	// compute and check the checksum
 	if (Checksum::PseudoHeader(addressModule, gBufferModule, buffer,
 			IPPROTO_ICMPV6) != 0)
 		return B_BAD_DATA;

	switch (header.icmp6_type) {
		case ICMP6_ECHO_REPLY:
			break;

		case ICMP6_ECHO_REQUEST:
		{
			if (buffer->interface_address != NULL) {
				// We only reply to echo requests of our local interface; we
				// don't reply to broadcast requests
				if (!domain->address_module->equal_addresses(
						buffer->interface_address->local, buffer->destination))
					break;
			}

			net_buffer *reply = gBufferModule->duplicate(buffer);
			if (reply == NULL)
				return B_NO_MEMORY;

			gBufferModule->swap_addresses(reply);

			// There already is an ICMP header, and we'll reuse it
			NetBufferHeaderReader<icmp6_hdr> header(reply);

			header->icmp6_type = ICMP6_ECHO_REPLY;
			header->icmp6_code = 0;
			header->icmp6_cksum = 0;

			header.Sync();

			*ICMP6ChecksumField(reply) = Checksum::PseudoHeader(addressModule,
				gBufferModule, buffer, IPPROTO_ICMPV6);

			status_t status = domain->module->send_data(NULL, reply);
			if (status < B_OK) {
				gBufferModule->free(reply);
				return status;
			}
		}

		default:
			// unrecognized messages go to neighbor discovery protocol handler
			return sIPv6NDPModule->receive_data(buffer);
	}

	gBufferModule->free(buffer);
	return B_OK;
}


status_t
icmp6_deliver_data(net_protocol *protocol, net_buffer *buffer)
{
	// TODO: does this look OK?
	return icmp6_receive_data(buffer);
}


status_t
icmp6_error_received(net_error code, net_buffer* data)
{
 	return B_ERROR;
}


status_t
icmp6_error_reply(net_protocol* protocol, net_buffer* buffer, net_error error,
	net_error_data* errorData)
{
	return B_ERROR;
}


//	#pragma mark -


static status_t
icmp6_init()
{
	sStackModule->register_domain_protocols(AF_INET6, SOCK_DGRAM, IPPROTO_ICMPV6,
		"network/protocols/icmp6/v1",
		"network/protocols/ipv6/v1",
		NULL);

	sStackModule->register_domain_receiving_protocol(AF_INET6, IPPROTO_ICMPV6,
		"network/protocols/icmp6/v1");

	return B_OK;
}


static status_t
icmp6_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return icmp6_init();

		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


net_protocol_module_info sICMP6Module = {
	{
		"network/protocols/icmp6/v1",
		0,
		icmp6_std_ops
	},
	NET_PROTOCOL_ATOMIC_MESSAGES,

	icmp6_init_protocol,
	icmp6_uninit_protocol,
	icmp6_open,
	icmp6_close,
	icmp6_free,
	icmp6_connect,
	icmp6_accept,
	icmp6_control,
	icmp6_getsockopt,
	icmp6_setsockopt,
	icmp6_bind,
	icmp6_unbind,
	icmp6_listen,
	icmp6_shutdown,
	icmp6_send_data,
	icmp6_send_routed_data,
	icmp6_send_avail,
	icmp6_read_data,
	icmp6_read_avail,
	icmp6_get_domain,
	icmp6_get_mtu,
	icmp6_receive_data,
	icmp6_deliver_data,
	icmp6_error_received,
	icmp6_error_reply,
	NULL,		// add_ancillary_data()
	NULL,		// process_ancillary_data()
	NULL,		// process_ancillary_data_no_container()
	NULL,		// send_data_no_buffer()
	NULL		// read_data_no_buffer()
};

module_dependency module_dependencies[] = {
	{NET_STACK_MODULE_NAME, (module_info **)&sStackModule},
	{NET_BUFFER_MODULE_NAME, (module_info **)&gBufferModule},
	{"network/datalink_protocols/ipv6_datagram/ndp/v1",
		(module_info **)&sIPv6NDPModule},
	{}
};

module_info *modules[] = {
	(module_info *)&sICMP6Module,
	NULL
};
