/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <stdio.h>
#include <sys/un.h>

#include <new>

#include <lock.h>
#include <util/AutoLock.h>
#include <vfs.h>

#include <net_buffer.h>
#include <net_protocol.h>
#include <net_socket.h>
#include <net_stack.h>

#include "UnixAddressManager.h"
#include "UnixEndpoint.h"


#define UNIX_MODULE_DEBUG_LEVEL	2
#define UNIX_DEBUG_LEVEL		UNIX_MODULE_DEBUG_LEVEL
#include "UnixDebug.h"


extern net_protocol_module_info gUnixModule;
	// extern only for forwarding

net_stack_module_info *gStackModule;
net_socket_module_info *gSocketModule;
net_buffer_module_info *gBufferModule;
UnixAddressManager gAddressManager;

static struct net_domain *sDomain;


net_protocol *
unix_init_protocol(net_socket *socket)
{
	TRACE("[%ld] unix_init_protocol(%p)\n", find_thread(NULL), socket);

	UnixEndpoint* endpoint = new(std::nothrow) UnixEndpoint(socket);
	if (endpoint == NULL)
		return NULL;

	status_t error = endpoint->Init();
	if (error != B_OK) {
		delete endpoint;
		return NULL;
	}

	return endpoint;
}


status_t
unix_uninit_protocol(net_protocol *_protocol)
{
	TRACE("[%ld] unix_uninit_protocol(%p)\n", find_thread(NULL), _protocol);
	((UnixEndpoint*)_protocol)->Uninit();
	return B_OK;
}


status_t
unix_open(net_protocol *_protocol)
{
	return ((UnixEndpoint*)_protocol)->Open();
}


status_t
unix_close(net_protocol *_protocol)
{
	return ((UnixEndpoint*)_protocol)->Close();
}


status_t
unix_free(net_protocol *_protocol)
{
	return ((UnixEndpoint*)_protocol)->Free();
}


status_t
unix_connect(net_protocol *_protocol, const struct sockaddr *address)
{
	return ((UnixEndpoint*)_protocol)->Connect(address);
}


status_t
unix_accept(net_protocol *_protocol, struct net_socket **_acceptedSocket)
{
	return ((UnixEndpoint*)_protocol)->Accept(_acceptedSocket);
}


status_t
unix_control(net_protocol *protocol, int level, int option, void *value,
	size_t *_length)
{
	return B_BAD_VALUE;
}


status_t
unix_getsockopt(net_protocol *protocol, int level, int option, void *value,
	int *_length)
{
	return gSocketModule->get_option(protocol->socket, level, option, value,
		_length);
}


status_t
unix_setsockopt(net_protocol *protocol, int level, int option,
	const void *_value, int length)
{
	UnixEndpoint* endpoint = (UnixEndpoint*)protocol;

	if (level == SOL_SOCKET) {
		if (option == SO_RCVBUF) {
			if (length != sizeof(int))
				return B_BAD_VALUE;

			endpoint->SetReceiveBufferSize(*(int*)_value);
		} else if (option == SO_SNDBUF) {
			// We don't have a receive buffer, so silently ignore this one.
		}
	}

	return gSocketModule->set_option(protocol->socket, level, option,
		_value, length);
}


status_t
unix_bind(net_protocol *_protocol, const struct sockaddr *_address)
{
	return ((UnixEndpoint*)_protocol)->Bind(_address);
}


status_t
unix_unbind(net_protocol *_protocol, struct sockaddr *_address)
{
	return ((UnixEndpoint*)_protocol)->Unbind();
}


status_t
unix_listen(net_protocol *_protocol, int count)
{
	return ((UnixEndpoint*)_protocol)->Listen(count);
}


status_t
unix_shutdown(net_protocol *_protocol, int direction)
{
	return ((UnixEndpoint*)_protocol)->Shutdown(direction);
}


status_t
unix_send_routed_data(net_protocol *_protocol, struct net_route *route,
	net_buffer *buffer)
{
	return B_ERROR;
}


status_t
unix_send_data(net_protocol *_protocol, net_buffer *buffer)
{
	return ((UnixEndpoint*)_protocol)->Send(buffer);
}


ssize_t
unix_send_avail(net_protocol *_protocol)
{
	return ((UnixEndpoint*)_protocol)->Sendable();
}


status_t
unix_read_data(net_protocol *_protocol, size_t numBytes, uint32 flags,
	net_buffer **_buffer)
{
	return ((UnixEndpoint*)_protocol)->Receive(numBytes, flags, _buffer);
}


ssize_t
unix_read_avail(net_protocol *_protocol)
{
	return ((UnixEndpoint*)_protocol)->Receivable();
}


struct net_domain *
unix_get_domain(net_protocol *protocol)
{
	return sDomain;
}


size_t
unix_get_mtu(net_protocol *protocol, const struct sockaddr *address)
{
	return UNIX_MAX_TRANSFER_UNIT;
}


status_t
unix_receive_data(net_buffer *buffer)
{
	return B_ERROR;
}


status_t
unix_deliver_data(net_protocol *_protocol, net_buffer *buffer)
{
	return B_ERROR;
}


status_t
unix_error(uint32 code, net_buffer *data)
{
	return B_ERROR;
}


status_t
unix_error_reply(net_protocol *protocol, net_buffer *causedError, uint32 code,
	void *errorData)
{
	return B_ERROR;
}


// #pragma mark -


status_t
init_unix()
{
	new(&gAddressManager) UnixAddressManager;
	status_t error = gAddressManager.Init();
	if (error != B_OK)
		return error;

	error = gStackModule->register_domain_protocols(AF_UNIX, SOCK_STREAM, 0,
		"network/protocols/unix/v1", NULL);
	if (error != B_OK) {
		gAddressManager.~UnixAddressManager();
		return error;
	}

	error = gStackModule->register_domain(AF_UNIX, "unix", &gUnixModule,
		&gAddressModule, &sDomain);
	if (error != B_OK) {
		gAddressManager.~UnixAddressManager();
		return error;
	}

	return B_OK;
}


status_t
uninit_unix()
{
	gStackModule->unregister_domain(sDomain);

	gAddressManager.~UnixAddressManager();

	return B_OK;
}


static status_t
unix_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return init_unix();
		case B_MODULE_UNINIT:
			return uninit_unix();

		default:
			return B_ERROR;
	}
}


net_protocol_module_info gUnixModule = {
	{
		"network/protocols/unix/v1",
		0,
		unix_std_ops
	},
	0,	// NET_PROTOCOL_ATOMIC_MESSAGES,

	unix_init_protocol,
	unix_uninit_protocol,
	unix_open,
	unix_close,
	unix_free,
	unix_connect,
	unix_accept,
	unix_control,
	unix_getsockopt,
	unix_setsockopt,
	unix_bind,
	unix_unbind,
	unix_listen,
	unix_shutdown,
	unix_send_data,
	unix_send_routed_data,
	unix_send_avail,
	unix_read_data,
	unix_read_avail,
	unix_get_domain,
	unix_get_mtu,
	unix_receive_data,
	unix_deliver_data,
	unix_error,
	unix_error_reply,
};

module_dependency module_dependencies[] = {
	{NET_STACK_MODULE_NAME, (module_info **)&gStackModule},
	{NET_BUFFER_MODULE_NAME, (module_info **)&gBufferModule},
//	{NET_DATALINK_MODULE_NAME, (module_info **)&sDatalinkModule},
	{NET_SOCKET_MODULE_NAME, (module_info **)&gSocketModule},
	{}
};

module_info *modules[] = {
	(module_info *)&gUnixModule,
	NULL
};
