/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include "stack_private.h"


/*!	Interface module providing networking to the kernel.
*/


static status_t
stack_interface_open(int family, int type, int protocol, net_socket** _socket)
{
	return gNetSocketModule.open_socket(family, type, protocol, _socket);
}


static status_t
stack_interface_close(net_socket* socket)
{
	return gNetSocketModule.close(socket);
}


static status_t
stack_interface_free(net_socket* socket)
{
	gNetSocketModule.free(socket);
	return B_OK;
}


static status_t
stack_interface_bind(net_socket* socket, const struct sockaddr* address,
	socklen_t addressLength)
{
	return gNetSocketModule.bind(socket, address, addressLength);
}


static status_t
stack_interface_shutdown(net_socket* socket, int how)
{
	return gNetSocketModule.shutdown(socket, how);
}


static status_t
stack_interface_connect(net_socket* socket, const struct sockaddr* address,
	socklen_t addressLength)
{
	return gNetSocketModule.connect(socket, address, addressLength);
}


static status_t
stack_interface_listen(net_socket* socket, int backlog)
{
	return gNetSocketModule.listen(socket, backlog);
}


static status_t
stack_interface_accept(net_socket* socket, struct sockaddr* address,
	socklen_t* _addressLength, net_socket** _acceptedSocket)
{
	return gNetSocketModule.accept(socket, address, _addressLength,
		_acceptedSocket);
}


static ssize_t
stack_interface_recv(net_socket* socket, void* data, size_t length, int flags)
{
	return gNetSocketModule.receive(socket, NULL, data, length, flags);
}


static ssize_t
stack_interface_recvfrom(net_socket* socket, void* data, size_t length,
	int flags, struct sockaddr* address, socklen_t* _addressLength)
{
	msghdr message;
	iovec vec = { data, length };
	message.msg_name = address;
	if (_addressLength != NULL)
		message.msg_namelen = *_addressLength;
	else
		message.msg_namelen = 0;
	message.msg_iov = &vec;
	message.msg_iovlen = 1;
	message.msg_control = NULL;
	message.msg_controllen = 0;
	message.msg_flags = 0;

	ssize_t received = gNetSocketModule.receive(socket, &message, data, length,
		flags);

	if (received >= 0 && _addressLength != NULL)
		*_addressLength = message.msg_namelen;

	return received;
}


static ssize_t
stack_interface_recvmsg(net_socket* socket, struct msghdr* message, int flags)
{
	void* buffer = NULL;
	size_t length = 0;
	if (message->msg_iovlen > 0) {
		buffer = message->msg_iov[0].iov_base;
		length = message->msg_iov[0].iov_len;
	}

	return gNetSocketModule.receive(socket, message, buffer, length, flags);
}


static ssize_t
stack_interface_send(net_socket* socket, const void* data, size_t length,
	int flags)
{
	return gNetSocketModule.send(socket, NULL, data, length, flags);
}


static ssize_t
stack_interface_sendto(net_socket* socket, const void* data, size_t length,
	int flags, const struct sockaddr* address, socklen_t addressLength)
{
	msghdr message;
	iovec vec = { (void*)data, length };
	message.msg_name = (void*)address;
	message.msg_namelen = addressLength;
	message.msg_iov = &vec;
	message.msg_iovlen = 1;
	message.msg_control = NULL;
	message.msg_controllen = 0;
	message.msg_flags = 0;

	return gNetSocketModule.send(socket, &message, data, length, flags);
}


static ssize_t
stack_interface_sendmsg(net_socket* socket, const struct msghdr* message,
	int flags)
{
	void* buffer = NULL;
	size_t length = 0;
	if (message->msg_iovlen > 0) {
		buffer = message->msg_iov[0].iov_base;
		length = message->msg_iov[0].iov_len;
	}

	return gNetSocketModule.send(socket, (msghdr*)message, buffer, length,
		flags);
}


static status_t
stack_interface_getsockopt(net_socket* socket, int level, int option,
	void* value, socklen_t* _length)
{
	int length = *_length;
	status_t error = gNetSocketModule.getsockopt(socket, level, option, value,
		&length);
	*_length = length;
	return error;
}


static status_t
stack_interface_setsockopt(net_socket* socket, int level, int option,
	const void* value, socklen_t length)
{
	return gNetSocketModule.setsockopt(socket, level, option, value, length);
}


static status_t
stack_interface_getpeername(net_socket* socket, struct sockaddr* address,
	socklen_t* _addressLength)
{
	return gNetSocketModule.getpeername(socket, address, _addressLength);
}


static status_t
stack_interface_getsockname(net_socket* socket, struct sockaddr* address,
	socklen_t* _addressLength)
{
	return gNetSocketModule.getsockname(socket, address, _addressLength);
}


static int
stack_interface_sockatmark(net_socket* socket)
{
	// TODO: sockatmark() missing
	return B_UNSUPPORTED;
}


static status_t
stack_interface_socketpair(int family, int type, int protocol,
	net_socket* _sockets[2])
{
	return gNetSocketModule.socketpair(family, type, protocol, _sockets);
}


static status_t
stack_interface_ioctl(net_socket* socket, uint32 op, void* buffer,
	size_t length)
{
	return gNetSocketModule.control(socket, op, buffer, length);
}


static status_t
stack_interface_select(net_socket* socket, uint8 event, struct selectsync* sync)
{
	return gNetSocketModule.request_notification(socket, event, sync);
}


static status_t
stack_interface_deselect(net_socket* socket, uint8 event,
	struct selectsync* sync)
{
	return gNetSocketModule.cancel_notification(socket, event, sync);
}


status_t
stack_interface_get_next_socket_stat(int family, uint32* cookie,
	struct net_stat* stat)
{
	return gNetSocketModule.get_next_stat(cookie, family, stat);
}


static status_t
stack_interface_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return init_stack();
		case B_MODULE_UNINIT:
			return uninit_stack();

		default:
			return B_ERROR;
	}
}


net_stack_interface_module_info gNetStackInterfaceModule = {
	{
		NET_STACK_INTERFACE_MODULE_NAME,
		0,
		stack_interface_std_ops
	},

	&stack_interface_open,
	&stack_interface_close,
	&stack_interface_free,

	&stack_interface_bind,
	&stack_interface_shutdown,
	&stack_interface_connect,
	&stack_interface_listen,
	&stack_interface_accept,

	&stack_interface_recv,
	&stack_interface_recvfrom,
	&stack_interface_recvmsg,

	&stack_interface_send,
	&stack_interface_sendto,
	&stack_interface_sendmsg,

	&stack_interface_getsockopt,
	&stack_interface_setsockopt,

	&stack_interface_getpeername,
	&stack_interface_getsockname,

	&stack_interface_sockatmark,

	&stack_interface_socketpair,

	&stack_interface_ioctl,
	&stack_interface_select,
	&stack_interface_deselect,

	&stack_interface_get_next_socket_stat
};
