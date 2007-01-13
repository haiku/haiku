/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */

/*!
	The kernel socket API directly forwards all requests into the stack
	via the networking stack driver.
*/

#include <socket_interface.h>

#include <net_stack_driver.h>

#include <sys/ioctl.h>
#include <unistd.h>


static int
socket(int family, int type, int protocol)
{
	int socket = open(NET_STACK_DRIVER_PATH, O_RDWR);
	if (socket < 0)
		return -1;

	socket_args args;
	args.family = family;
	args.type = type;
	args.protocol = protocol;

	if (ioctl(socket, NET_STACK_SOCKET, &args, sizeof(args)) < 0) {
		close(socket);
		return -1;
	}

	return socket;
}


static int
bind(int socket, const struct sockaddr *address, socklen_t addressLength)
{
	sockaddr_args args;
	args.address = const_cast<struct sockaddr *>(address);
	args.address_length = addressLength;

	return ioctl(socket, NET_STACK_BIND, &args, sizeof(args));
}


static int
shutdown(int socket, int how)
{
	return ioctl(socket, NET_STACK_SHUTDOWN, (void *)how, 0);
}


static int
connect(int socket, const struct sockaddr *address, socklen_t addressLength)
{
	sockaddr_args args;
	args.address = const_cast<struct sockaddr *>(address);
	args.address_length = addressLength;

	return ioctl(socket, NET_STACK_CONNECT, &args, sizeof(args));
}


static int
listen(int socket, int backlog)
{
	return ioctl(socket, NET_STACK_LISTEN, (void *)backlog, 0);
}


static int
accept(int socket, struct sockaddr *address, socklen_t *_addressLength)
{
	int acceptSocket = open(NET_STACK_DRIVER_PATH, O_RDWR);
	if (acceptSocket < 0)
		return -1;

	// The network stack driver will need to know to which net_stack_cookie to 
	// *bind* with the new accepted socket. It can't itself find out, so we need
	// to pass it the cookie used internally.
	// TODO: change this to a safer approach using IDs!
	void *cookie;
	if (ioctl(acceptSocket, NET_STACK_GET_COOKIE, &cookie) < 0) {
		close(acceptSocket);
		return -1;
	}

	accept_args args;
	args.cookie = cookie;
	args.address = address;
	args.address_length = _addressLength ? *_addressLength : 0;

	if (ioctl(socket, NET_STACK_ACCEPT, &args, sizeof(args)) < 0) {
		close(acceptSocket);
		return -1;
	}

	if (_addressLength != NULL)
		*_addressLength = args.address_length;

	return acceptSocket;
}


static ssize_t
recv(int socket, void *data, size_t length, int flags)
{
	transfer_args args;
	args.data = data;
	args.data_length = length;
	args.flags = flags;
	args.address = NULL;
	args.address_length = 0;

	return ioctl(socket, NET_STACK_RECV, &args, sizeof(args));
}


static ssize_t
recvfrom(int socket, void *data, size_t length, int flags,
	struct sockaddr *address, socklen_t *_addressLength)
{
	transfer_args args;
	args.data = data;
	args.data_length = length;
	args.flags = flags;
	args.address = address;
	args.address_length = _addressLength ? *_addressLength : 0;

	ssize_t bytesReceived = ioctl(socket, NET_STACK_RECVFROM, &args, sizeof(args));
	if (bytesReceived < 0)
		return -1;

	if (_addressLength != NULL)
		*_addressLength = args.address_length;

	return bytesReceived;
}


static ssize_t
recvmsg(int socket, struct msghdr *message, int flags)
{
	// TODO: implement me!
	return -1;
}


static ssize_t
send(int socket, const void *data, size_t length, int flags)
{
	transfer_args args;
	args.data = const_cast<void *>(data);
	args.data_length = length;
	args.flags = flags;
	args.address = NULL;
	args.address_length = 0;

	return ioctl(socket, NET_STACK_SEND, &args, sizeof(args));
}


static ssize_t
sendto(int socket, const void *data, size_t length, int flags,
	const struct sockaddr *address, socklen_t addressLength)
{
	transfer_args args;
	args.data = const_cast<void *>(data);
	args.data_length = length;
	args.flags = flags;
	args.address = const_cast<sockaddr *>(address);
	args.address_length = addressLength;

	return ioctl(socket, NET_STACK_SENDTO, &args, sizeof(args));
}


static ssize_t
sendmsg(int socket, const struct msghdr *message, int flags)
{
	// TODO: implement me!
	return -1;
}


static int
getsockopt(int socket, int level, int option, void *value, size_t *_length)
{
	sockopt_args args;
	args.level = level;
	args.option = option;
	args.value = value;
	args.length = _length ? *_length : 0;

	if (ioctl(socket, NET_STACK_GETSOCKOPT, &args, sizeof(args)) < 0)
		return -1;

	if (_length)
		*_length = args.length;

	return 0;
}


static int
setsockopt(int socket, int level, int option, const void *value, size_t length)
{
	sockopt_args args;
	args.level = level;
	args.option = option;
	args.value = const_cast<void *>(value);
	args.length = length;

	return ioctl(socket, NET_STACK_SETSOCKOPT, &args, sizeof(args));
}


static int
getpeername(int socket, struct sockaddr *address, socklen_t *_addressLength)
{
	sockaddr_args args;
	args.address = address;
	args.address_length = _addressLength ? *_addressLength : 0;

	if (ioctl(socket, NET_STACK_GETPEERNAME, &args, sizeof(args)) < 0)
		return -1;

	if (_addressLength != NULL)
		*_addressLength = args.address_length;

	return 0;
}


static int
getsockname(int socket, struct sockaddr *address, socklen_t *_addressLength)
{
	sockaddr_args args;
	args.address = address;
	args.address_length = _addressLength ? *_addressLength : 0;

	if (ioctl(socket, NET_STACK_GETSOCKNAME, &args, sizeof(args)) < 0)
		return -1;

	if (_addressLength != NULL)
		*_addressLength = args.address_length;

	return 0;
}


static int
sockatmark(int socket)
{
	// TODO: implement me!
	return -1;
}


static int
socketpair(int family, int type, int protocol, int socketVector[2])
{
	socketVector[0] = socket(family, type, protocol);
	if (socketVector[0] < 0)
		return -1;

	socketVector[1] = socket(family, type, protocol);
	if (socketVector[1] < 0)
		goto err1;

	// TODO: find a better way to do this!
	void *cookie;
	if (ioctl(socketVector[1], NET_STACK_GET_COOKIE, &cookie) < 0)
		goto err2;

	socketpair_args args;
	args.cookie = cookie;
		// this way driver can use the right fd/cookie! 	

	if (ioctl(socketVector[0], NET_STACK_SOCKETPAIR, &args, sizeof(args)) < 0)
		goto err2;

	return 0;

err2:
	close(socketVector[1]); 
err1:
	close(socketVector[0]);
	return -1;
}


//	#pragma mark -


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


static socket_module_info sSocketModule = {
	{
		B_SOCKET_MODULE_NAME,
		0,
		std_ops
	},

	accept,
	bind,
	connect,
	getpeername,
	getsockname,
	getsockopt,
	listen,
	recv,
	recvfrom,
	recvmsg,
	send,
	sendmsg,
	sendto,
	setsockopt,
	shutdown,
	socket,
	sockatmark,
	socketpair,
};

module_info *modules[] = {
	(module_info *)&sSocketModule,
	NULL
};
