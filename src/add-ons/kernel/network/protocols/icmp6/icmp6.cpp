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

#include <algorithm>
#include <netinet/icmp6.h>
#include <netinet/ip6.h>
#include <netinet6/in6.h>
#include <new>
#include <stdlib.h>
#include <string.h>

#include <ipv6_datagram/ndp.h>


//#define TRACE_ICMP6
#ifdef TRACE_ICMP6
#	define TRACE(x...) dprintf(x)
#else
#	define TRACE(x...) ;
#endif


typedef NetBufferField<uint16, offsetof(icmp6_hdr, icmp6_cksum)> ICMP6ChecksumField;


net_buffer_module_info *gBufferModule;
static net_stack_module_info *sStackModule;
static net_ndp_module_info *sIPv6NDPModule;


#ifdef TRACE_ICMP6


static const char*
net_error_to_string(net_error error)
{
#define CODE(x) case x: return #x;
	switch (error) {
		CODE(B_NET_ERROR_REDIRECT_HOST)
		CODE(B_NET_ERROR_UNREACH_NET)
		CODE(B_NET_ERROR_UNREACH_HOST)
		CODE(B_NET_ERROR_UNREACH_PROTOCOL)
		CODE(B_NET_ERROR_UNREACH_PORT)
		CODE(B_NET_ERROR_UNREACH_SOURCE_FAIL)
		CODE(B_NET_ERROR_UNREACH_NET_UNKNOWN)
		CODE(B_NET_ERROR_UNREACH_HOST_UNKNOWN)
		CODE(B_NET_ERROR_UNREACH_ISOLATED)
		CODE(B_NET_ERROR_UNREACH_NET_PROHIBITED)
		CODE(B_NET_ERROR_UNREACH_HOST_PROHIBITED)
		CODE(B_NET_ERROR_UNREACH_NET_TOS)
		CODE(B_NET_ERROR_UNREACH_HOST_TOS)
		CODE(B_NET_ERROR_UNREACH_FILTER_PROHIBITED)
		CODE(B_NET_ERROR_UNREACH_HOST_PRECEDENCE)
		CODE(B_NET_ERROR_UNREACH_PRECEDENCE_CUTOFF)
		CODE(B_NET_ERROR_MESSAGE_SIZE)
		CODE(B_NET_ERROR_TRANSIT_TIME_EXCEEDED)
		CODE(B_NET_ERROR_REASSEMBLY_TIME_EXCEEDED)
		CODE(B_NET_ERROR_PARAMETER_PROBLEM)
		CODE(B_NET_ERROR_QUENCH)
		CODE(B_NET_ERROR_UNREACH_BEYONDSCOPE)
		default:
			return "unknown";
	}
#undef CODE
}


#endif	// TRACE_ICMP6


static void
fill_sockaddr_in6(sockaddr_in6* target, in6_addr address)
{
	target->sin6_family = AF_INET6;
	target->sin6_len = sizeof(sockaddr_in6);
	target->sin6_port = 0;
	target->sin6_addr = address;
}


static net_error
icmp6_to_net_error(uint8 type, uint8 code)
{
	switch (type) {
		case ICMP6_DST_UNREACH:
			switch (code) {
				case ICMP6_DST_UNREACH_NOROUTE:
					return B_NET_ERROR_UNREACH_NET;
				case ICMP6_DST_UNREACH_ADMIN:
					return B_NET_ERROR_UNREACH_NET_PROHIBITED;
				case ICMP6_DST_UNREACH_BEYONDSCOPE:
					return B_NET_ERROR_UNREACH_BEYONDSCOPE;
				case ICMP6_DST_UNREACH_ADDR:
					return B_NET_ERROR_UNREACH_HOST;
				case ICMP6_DST_UNREACH_NOPORT:
					return B_NET_ERROR_UNREACH_PORT;
				default:
					break;
			}
		case ICMP6_PARAM_PROB:
			return B_NET_ERROR_PARAMETER_PROBLEM;

		case ICMP6_PACKET_TOO_BIG:
			return B_NET_ERROR_MESSAGE_SIZE;

		case ICMP6_TIME_EXCEEDED:
			switch (code) {
				case ICMP6_TIME_EXCEED_TRANSIT:
					return B_NET_ERROR_TRANSIT_TIME_EXCEEDED;
				case ICMP6_TIME_EXCEED_REASSEMBLY:
					return B_NET_ERROR_REASSEMBLY_TIME_EXCEEDED;
				default:
					break;
			}
		default:
			break;
	}

	return (net_error)0;
}


static void
net_error_to_icmp(net_error error, uint8& type, uint8& code)
{
	switch (error) {
		// unreach
		case B_NET_ERROR_UNREACH_NET:
			type = ICMP6_DST_UNREACH;
			code = ICMP6_DST_UNREACH_NOROUTE;
			break;
		case B_NET_ERROR_UNREACH_HOST:
			type = ICMP6_DST_UNREACH;
			code = ICMP6_DST_UNREACH_ADDR;
			break;
		case B_NET_ERROR_UNREACH_PORT:
			type = ICMP6_DST_UNREACH;
			code = ICMP6_DST_UNREACH_NOPORT;
			break;
		case B_NET_ERROR_UNREACH_NET_PROHIBITED:
			type = ICMP6_DST_UNREACH;
			code = ICMP6_DST_UNREACH_ADMIN;
			break;
		case B_NET_ERROR_UNREACH_BEYONDSCOPE:
			type = ICMP6_DST_UNREACH;
			code = ICMP6_DST_UNREACH_BEYONDSCOPE;
			break;

		// time exceeded
		case B_NET_ERROR_TRANSIT_TIME_EXCEEDED:
			type = ICMP6_TIME_EXCEEDED;
			code = ICMP6_TIME_EXCEED_TRANSIT;
			break;
		case B_NET_ERROR_REASSEMBLY_TIME_EXCEEDED:
			type = ICMP6_TIME_EXCEEDED;
			code = ICMP6_TIME_EXCEED_REASSEMBLY;
			break;

		// other
		case B_NET_ERROR_PARAMETER_PROBLEM:
			type = ICMP6_PARAM_PROB;
			code = 0;
			break;

		default:
			break;
	}
}


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
	TRACE("ICMPv6 received some data, buffer length %" B_PRIu32 "\n",
		buffer->size);

	net_domain* domain = get_domain(buffer);
	if (domain == NULL)
		return B_ERROR;

	NetBufferHeaderReader<icmp6_hdr> bufferHeader(buffer);
	if (bufferHeader.Status() < B_OK)
		return bufferHeader.Status();

	icmp6_hdr &header = bufferHeader.Data();

	TRACE("  got type %u, code %u, checksum 0x%x\n", header.icmp6_type,
			header.icmp6_code, header.icmp6_cksum);

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

		case ICMP6_DST_UNREACH:
		case ICMP6_PACKET_TOO_BIG:
		case ICMP6_TIME_EXCEEDED:
		case ICMP6_PARAM_PROB:
		{
			net_domain* domain = get_domain(buffer);
			if (domain == NULL)
				break;

			net_error error = icmp6_to_net_error(header.icmp6_type,
				header.icmp6_code);
			if (error == 0)
				break;

			net_error_data dataStorage = {};
			net_error_data* data = NULL;
			if (error == B_NET_ERROR_MESSAGE_SIZE) {
				data = &dataStorage;
				data->mtu = ntohl(header.icmp6_mtu);

				// IPv6 minimum fragment size is 1280 bytes, so if the "next MTU"
				// is smaller than that, we can be sure it's invalid.
				if (data->mtu < 1280)
					data = NULL;
			}

			// Deliver the error to the domain protocol which will
			// propagate the error to the upper protocols
			bufferHeader.Remove();
			return domain->module->error_received(error, data, buffer);
		}

		default:
			// unrecognized messages go to neighbor discovery protocol handler
			return sIPv6NDPModule->receive_data(buffer);
	}

	gBufferModule->free(buffer);
	return B_OK;
}


status_t
icmp6_error_received(net_error code, net_error_data* errorData, net_buffer* data)
{
	return B_ERROR;
}


/*!	Sends an ICMPv6 error message to the source of the \a buffer causing the
	error.
*/
status_t
icmp6_error_reply(net_protocol* protocol, net_buffer* buffer, net_error error,
	net_error_data* errorData)
{
	TRACE("icmp6_error_reply(code %s)\n", net_error_to_string(error));

	uint8 icmpType = 0;
	uint8 icmpCode = 0;
	net_error_to_icmp(error, icmpType, icmpCode);

	TRACE("  icmp type %u, code %u\n", icmpType, icmpCode);

	struct ip6_hdr header;
	if (gBufferModule->restore_header(buffer, 0, &header, sizeof(header)) != B_OK)
		return B_BAD_VALUE;

	// Check if we actually have an IPv6 header now
	if ((header.ip6_vfc & IPV6_VERSION_MASK) != IPV6_VERSION) {
		TRACE("  no IPv6 header found\n");
		return B_BAD_VALUE;
	}

	// RFC 4443 2.4 (e.1) (e.2):
	// ICMP error message should not be sent on reception of
	// an ICMP error message, or a redirect
	if (header.ip6_nxt == IPPROTO_ICMPV6) {
		uint8 type = 0;
		if (gBufferModule->restore_header(buffer, sizeof(header), &type,
				1) != B_OK || (type & ICMP6_INFOMSG_MASK) == 0 || type == ND_REDIRECT) {
			return B_ERROR;
		}
	}

	// RFC 4443 2.4 (e.3) (e.4):
	// a datagram to an IP multicast or broadcast address,
	// except for ICMP6_PACKET_TOO_BIG ICMP6_PARAM_PROB ICMP6_PARAMPROB_OPTION
	if ((buffer->msg_flags & (MSG_BCAST | MSG_MCAST)) != 0
		&& icmpType != ICMP6_PACKET_TOO_BIG && icmpType != ICMP6_PARAM_PROB
		&& icmpType != ICMP6_PARAMPROB_OPTION) {
		return B_ERROR;
	}

	net_buffer* reply = gBufferModule->create(256);
	if (reply == NULL)
		return B_NO_MEMORY;

	if (buffer->destination->sa_family == AF_INET6) {
		memcpy(reply->source, buffer->destination, buffer->destination->sa_len);
		memcpy(reply->destination, buffer->source, buffer->source->sa_len);
	} else {
		fill_sockaddr_in6((sockaddr_in6*)reply->source, header.ip6_dst);
		fill_sockaddr_in6((sockaddr_in6*)reply->destination, header.ip6_src);
	}

	// Now prepare the ICMP header

	NetBufferPrepend<icmp6_hdr> icmpHeader(reply);
	status_t status = icmpHeader.Status();
	if (status != B_OK) {
		gBufferModule->free(reply);
		return status;
	}
	icmpHeader->icmp6_type = icmpType;
	icmpHeader->icmp6_code = icmpCode;
	icmpHeader->icmp6_cksum = 0;
	icmpHeader->icmp6_data32[0] = 0;

	if (errorData != NULL) {
		switch (error) {
			case B_NET_ERROR_PARAMETER_PROBLEM:
				icmpHeader->icmp6_pptr = htonl(errorData->error_offset);
				break;
			case B_NET_ERROR_MESSAGE_SIZE:
				icmpHeader->icmp6_mtu = htonl(errorData->mtu);
				break;

			default:
				break;
		}
	}

	icmpHeader.Sync();

	// Append IP header + 8 byte of the original datagram
	status = gBufferModule->append_restored_header(reply, buffer, 0,
		std::min(sizeof(ip6_hdr) + 8, sizeof(ip6_hdr) + ntohs(header.ip6_plen)));
	if (status == B_OK) {
		net_domain* domain = get_domain(buffer);

		if (domain == NULL)
			status = B_ERROR;
		else {
			reply->protocol = IPPROTO_ICMPV6;
			*ICMP6ChecksumField(reply) = Checksum::PseudoHeader(domain->address_module,
				gBufferModule, reply, IPPROTO_ICMPV6);

			TRACE("  send ICMPv6 message %p to %s\n", reply,
				AddressString(domain->address_module, reply->destination, true).Data());

			status = domain->module->send_data(NULL, reply);
		}
	}
	if (status != B_OK)
		gBufferModule->free(reply);

	return status;
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
	NULL,		// deliver_data
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
