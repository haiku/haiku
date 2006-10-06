/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

/*!
	The socket API directly forwards all requests into the kernel stack
	via the networking stack driver.
*/

#include <net_stack_driver.h>
#include <r5_compatibility.h>

#include <errno.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>


// TODO: this is meant for debugging purposes only, and should be disabled later
static char *
stack_driver_path(void)
{
	// user-defined stack driver path?
	char *path = getenv("NET_STACK_DRIVER_PATH");
	if (path != NULL)
		return path;

	// use the default stack driver path
	return NET_STACK_DRIVER_PATH;
}


static void
convert_from_r5_sockaddr(struct sockaddr *_to, const struct sockaddr *_from)
{
	const r5_sockaddr_in *from = (r5_sockaddr_in *)_from;
	sockaddr_in *to = (sockaddr_in *)_to;

	memset(to, 0, sizeof(*to));
	to->sin_len = sizeof(*to);

	if (from->sin_family == R5_AF_INET)
		to->sin_family = AF_INET;
	else
		to->sin_family = from->sin_family;

	to->sin_port = from->sin_port;
	to->sin_addr.s_addr = from->sin_addr;
}


static void
convert_to_r5_sockaddr(struct sockaddr *_to, const struct sockaddr *_from)
{
	const sockaddr_in *from = (sockaddr_in *)_from;
	r5_sockaddr_in *to = (r5_sockaddr_in *)_to;

	memset(to, 0, sizeof(*to));
	if (from->sin_family == AF_INET)
		to->sin_family = R5_AF_INET;
	else
		to->sin_family = from->sin_family;

	to->sin_port = from->sin_port;
	to->sin_addr = from->sin_addr.s_addr;
}


static void
convert_from_r5_socket(int& family, int& type, int& protocol)
{
	switch (family) {
		case R5_AF_INET:
			family = AF_INET;
			break;
	}

	switch (type) {
		case R5_SOCK_DGRAM:
			type = SOCK_DGRAM;
			break;
		case R5_SOCK_STREAM:
			type = SOCK_STREAM;
			break;
#if 0
		case R5_SOCK_RAW:
			type = SOCK_RAW;
			break;
#endif
	}

	switch (protocol) {
		case R5_IPPROTO_UDP:
			protocol = IPPROTO_UDP;
			break;
		case R5_IPPROTO_TCP:
			protocol = R5_IPPROTO_TCP;
			break;
		case R5_IPPROTO_ICMP:
			protocol = R5_IPPROTO_ICMP;
			break;
	}
}


static void
convert_from_r5_sockopt(int& level, int& option)
{
	if (level == R5_SOL_SOCKET)
		level = SOL_SOCKET;

	switch (option) {
		case R5_SO_DEBUG:
			option = SO_DEBUG;
			break;
		case R5_SO_REUSEADDR:
			option = SO_REUSEADDR;
			break;
		case R5_SO_NONBLOCK:
			option = SO_NONBLOCK;
			break;
		case R5_SO_REUSEPORT:
			option = SO_REUSEPORT;
			break;
		case R5_SO_FIONREAD:
			// there is no SO_FIONREAD
			option = -1;
			break;
	}
}


// #pragma mark -


extern "C" int
socket(int family, int type, int protocol)
{
	int socket = open(stack_driver_path(), O_RDWR);
	if (socket < 0)
		return -1;

	if (__gR5Compatibility)
		convert_from_r5_socket(family, type, protocol);

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


extern "C" int
bind(int socket, const struct sockaddr *address, socklen_t addressLength)
{
	struct sockaddr r5addr;

	if (__gR5Compatibility) {
		convert_from_r5_sockaddr(&r5addr, address);
		address = &r5addr;
		addressLength = sizeof(struct sockaddr_in);
	}

	sockaddr_args args;
	args.address = const_cast<struct sockaddr *>(address);
	args.address_length = addressLength;

	return ioctl(socket, NET_STACK_BIND, &args, sizeof(args));
}


extern "C" int
shutdown(int socket, int how)
{
	return ioctl(socket, NET_STACK_SHUTDOWN, (void *)how, 0);
}


extern "C" int
connect(int socket, const struct sockaddr *address, socklen_t addressLength)
{
	struct sockaddr r5addr;

	if (__gR5Compatibility) {
		convert_from_r5_sockaddr(&r5addr, address);
		address = &r5addr;
		addressLength = sizeof(struct sockaddr_in);
	}

	sockaddr_args args;
	args.address = const_cast<struct sockaddr *>(address);
	args.address_length = addressLength;

	return ioctl(socket, NET_STACK_CONNECT, &args, sizeof(args));
}


extern "C" int
listen(int socket, int backlog)
{
	return ioctl(socket, NET_STACK_LISTEN, (void *)backlog, 0);
}


extern "C" int
accept(int socket, struct sockaddr *address, socklen_t *_addressLength)
{
	int acceptSocket = open(stack_driver_path(), O_RDWR);
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

	struct sockaddr r5addr;
	accept_args args;

	args.cookie = cookie;

	if (__gR5Compatibility) {
		args.address = &r5addr;
		args.address_length = sizeof(r5addr);
	} else {
		args.address = address;
		args.address_length = _addressLength ? *_addressLength : 0;
	}

	if (ioctl(socket, NET_STACK_ACCEPT, &args, sizeof(args)) < 0) {
		close(acceptSocket);
		return -1;
	}

	if (__gR5Compatibility) {
		convert_to_r5_sockaddr(address, &r5addr);
		if (_addressLength != NULL)
			*_addressLength = sizeof(struct r5_sockaddr_in);
	} else if (_addressLength != NULL)
		*_addressLength = args.address_length;

	return acceptSocket;
}


extern "C" ssize_t
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


extern "C" ssize_t
recvfrom(int socket, void *data, size_t length, int flags,
	struct sockaddr *address, socklen_t *_addressLength)
{
	struct sockaddr r5addr;

	transfer_args args;
	args.data = data;
	args.data_length = length;
	args.flags = flags;

	if (__gR5Compatibility) {
		args.address = &r5addr;
		args.address_length = sizeof(r5addr);
	} else {
		args.address = address;
		args.address_length = _addressLength ? *_addressLength : 0;
	}

	ssize_t bytesReceived = ioctl(socket, NET_STACK_RECVFROM, &args, sizeof(args));
	if (bytesReceived < 0)
		return -1;

	if (__gR5Compatibility) {
		convert_to_r5_sockaddr(address, &r5addr);
		if (_addressLength != NULL)
			*_addressLength = sizeof(struct r5_sockaddr_in);
	} else if (_addressLength != NULL)
		*_addressLength = args.address_length;

	return bytesReceived;
}


extern "C" ssize_t
recvmsg(int socket, struct msghdr *message, int flags)
{
	// TODO: implement me!
	return -1;
}


extern "C" ssize_t
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


extern "C" ssize_t
sendto(int socket, const void *data, size_t length, int flags,
	const struct sockaddr *address, socklen_t addressLength)
{
	struct sockaddr r5addr;

	if (__gR5Compatibility) {
		convert_from_r5_sockaddr(&r5addr, address);
		address = &r5addr;
		addressLength = sizeof(struct sockaddr_in);
	}

	transfer_args args;
	args.data = const_cast<void *>(data);
	args.data_length = length;
	args.flags = flags;
	args.address = const_cast<sockaddr *>(address);
	args.address_length = addressLength;

	return ioctl(socket, NET_STACK_SENDTO, &args, sizeof(args));
}


extern "C" ssize_t
sendmsg(int socket, const struct msghdr *message, int flags)
{
	// TODO: implement me!
	return -1;
}


extern "C" int
getsockopt(int socket, int level, int option, void *value, size_t *_length)
{
	if (__gR5Compatibility) {
		if (option == R5_SO_FIONREAD) {
			// there is no SO_FIONREAD in our stack; we're using FIONREAD instead
			*_length = sizeof(int);
			return ioctl(socket, FIONREAD, &value);
		}

		convert_from_r5_sockopt(level, option);
	}

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


extern "C" int
setsockopt(int socket, int level, int option, const void *value, size_t length)
{
	if (__gR5Compatibility)
		convert_from_r5_sockopt(level, option);

	sockopt_args args;
	args.level = level;
	args.option = option;
	args.value = const_cast<void *>(value);
	args.length = length;

	return ioctl(socket, NET_STACK_SETSOCKOPT, &args, sizeof(args));
}


extern "C" int
getpeername(int socket, struct sockaddr *address, socklen_t *_addressLength)
{
	struct sockaddr r5addr;

	sockaddr_args args;
	if (__gR5Compatibility) {
		args.address = &r5addr;
		args.address_length = sizeof(r5addr);
	} else {
		args.address = address;
		args.address_length = _addressLength ? *_addressLength : 0;
	}

	if (ioctl(socket, NET_STACK_GETPEERNAME, &args, sizeof(args)) < 0)
		return -1;

	if (__gR5Compatibility) {
		convert_to_r5_sockaddr(address, &r5addr);
		if (_addressLength != NULL)
			*_addressLength = sizeof(struct r5_sockaddr_in);
	} else if (_addressLength != NULL)
		*_addressLength = args.address_length;

	return 0;
}


extern "C" int
getsockname(int socket, struct sockaddr *address, socklen_t *_addressLength)
{
	struct sockaddr r5addr;

	sockaddr_args args;
	if (__gR5Compatibility) {
		args.address = &r5addr;
		args.address_length = sizeof(r5addr);
	} else {
		args.address = address;
		args.address_length = _addressLength ? *_addressLength : 0;
	}

	if (ioctl(socket, NET_STACK_GETSOCKNAME, &args, sizeof(args)) < 0)
		return -1;

	if (__gR5Compatibility) {
		convert_to_r5_sockaddr(address, &r5addr);
		if (_addressLength != NULL)
			*_addressLength = sizeof(struct r5_sockaddr_in);
	} else if (_addressLength != NULL)
		*_addressLength = args.address_length;

	return 0;
}


extern "C" int
sockatmark(int socket)
{
	// TODO: implement me!
	return -1;
}


extern "C" int
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
