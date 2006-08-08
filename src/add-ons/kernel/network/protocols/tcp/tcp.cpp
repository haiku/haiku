/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <net_protocol.h>
#include <net_stack.h>

#include <KernelExport.h>
#include <util/list.h>

#include <netinet/in.h>
#include <new>
#include <stdlib.h>

#include "tcp.h"

struct tcp_protocol : net_protocol {
};


net_protocol *
tcp_init_protocol(net_socket *socket)
{
	tcp_protocol *protocol = new (std::nothrow) tcp_protocol;
	if (protocol == NULL)
		return NULL;

	return protocol;
}


status_t
tcp_uninit_protocol(net_protocol *protocol)
{
	delete protocol;
	return B_OK;
}


status_t
tcp_open(net_protocol *protocol)
{
	return B_OK;
}


status_t
tcp_close(net_protocol *protocol)
{
	return B_OK;
}


status_t
tcp_free(net_protocol *protocol)
{
	return B_OK;
}


status_t
tcp_connect(net_protocol *protocol, const struct sockaddr *address)
{
	return B_ERROR;
}


status_t
tcp_accept(net_protocol *protocol, struct net_socket **_acceptedSocket)
{
	return B_ERROR;
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
	return B_ERROR;
}


status_t
tcp_unbind(net_protocol *protocol, struct sockaddr *address)
{
	return B_ERROR;
}


status_t
tcp_listen(net_protocol *protocol, int count)
{
	return B_ERROR;
}


status_t
tcp_shutdown(net_protocol *protocol, int direction)
{
	return B_ERROR;
}


status_t
tcp_send_data(net_protocol *protocol, net_buffer *buffer)
{
	return protocol->next->module->send_data(protocol->next, buffer);
}


status_t
tcp_send_routed_data(net_protocol *protocol, struct net_route *route,
	net_buffer *buffer)
{
	return protocol->next->module->send_routed_data(protocol->next, route, buffer);
}


ssize_t
tcp_send_avail(net_protocol *protocol)
{
	return B_ERROR;
}


status_t
tcp_read_data(net_protocol *protocol, size_t numBytes, uint32 flags,
	net_buffer **_buffer)
{
	return B_ERROR;
}


ssize_t
tcp_read_avail(net_protocol *protocol)
{
	return B_ERROR;
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
	return B_ERROR;
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
tcp_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			net_stack_module_info *stack;
			status_t status = get_module(NET_STACK_MODULE_NAME, (module_info **)&stack);
			if (status < B_OK)
				return status;

			stack->register_domain_protocols(AF_INET, SOCK_STREAM, IPPROTO_IP,
				"network/protocols/tcp/v1",
				"network/protocols/ipv4/v1",
				NULL);
			stack->register_domain_protocols(AF_INET, SOCK_STREAM, IPPROTO_TCP,
				"network/protocols/tcp/v1",
				"network/protocols/ipv4/v1",
				NULL);

			stack->register_domain_receiving_protocol(AF_INET, IPPROTO_TCP,
				"network/protocols/tcp/v1");

			put_module(NET_STACK_MODULE_NAME);
			return B_OK;
		}

		case B_MODULE_UNINIT:
			return B_OK;

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
