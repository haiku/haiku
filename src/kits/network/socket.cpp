/*
 * Copyright 2002-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

/*!
	The socket API directly forwards all requests into the kernel stack
	via the networking stack driver.
*/

#include <r5_compatibility.h>

#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <syscall_utils.h>

#include <syscalls.h>


static void
convert_from_r5_sockaddr(struct sockaddr *_to, const struct sockaddr *_from)
{
	const r5_sockaddr_in *from = (r5_sockaddr_in *)_from;
	sockaddr_in *to = (sockaddr_in *)_to;

	memset(to, 0, sizeof(sockaddr));
	to->sin_len = sizeof(sockaddr);

	if (from == NULL)
		return;

	if (from->sin_family == R5_AF_INET)
		to->sin_family = AF_INET;
	else
		to->sin_family = from->sin_family;

	to->sin_port = from->sin_port;
	to->sin_addr.s_addr = from->sin_addr;
}


static void
convert_to_r5_sockaddr(struct sockaddr *_to,
	const struct sockaddr *_from)
{
	const sockaddr_in *from = (sockaddr_in *)_from;
	r5_sockaddr_in *to = (r5_sockaddr_in *)_to;

	if (to == NULL)
		return;

	memset(to, 0, sizeof(r5_sockaddr_in));

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
			protocol = IPPROTO_TCP;
			break;
		case R5_IPPROTO_ICMP:
			protocol = IPPROTO_ICMP;
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
	if (check_r5_compatibility())
		convert_from_r5_socket(family, type, protocol);

	RETURN_AND_SET_ERRNO(_kern_socket(family, type, protocol));
}


extern "C" int
bind(int socket, const struct sockaddr *address, socklen_t addressLength)
{
	struct sockaddr haikuAddr;

	if (check_r5_compatibility()) {
		convert_from_r5_sockaddr(&haikuAddr, address);
		address = &haikuAddr;
		addressLength = sizeof(struct sockaddr_in);
	}

	RETURN_AND_SET_ERRNO(_kern_bind(socket, address, addressLength));
}


extern "C" int
shutdown(int socket, int how)
{
	RETURN_AND_SET_ERRNO(_kern_shutdown_socket(socket, how));
}


extern "C" int
connect(int socket, const struct sockaddr *address, socklen_t addressLength)
{
	struct sockaddr haikuAddr;

	if (check_r5_compatibility()) {
		convert_from_r5_sockaddr(&haikuAddr, address);
		address = &haikuAddr;
		addressLength = sizeof(struct sockaddr_in);
	}

	RETURN_AND_SET_ERRNO_TEST_CANCEL(
		_kern_connect(socket, address, addressLength));
}


extern "C" int
listen(int socket, int backlog)
{
	RETURN_AND_SET_ERRNO(_kern_listen(socket, backlog));
}


extern "C" int
accept(int socket, struct sockaddr *_address, socklen_t *_addressLength)
{
	bool r5compatible = check_r5_compatibility();
	struct sockaddr haikuAddr;

	sockaddr* address;
	socklen_t addressLength;

	if (r5compatible && _address != NULL) {
		address = &haikuAddr;
		addressLength = sizeof(haikuAddr);
	} else {
		address = _address;
		addressLength = _addressLength ? *_addressLength : 0;
	}

	int acceptSocket = _kern_accept(socket, address, &addressLength);

	pthread_testcancel();

	if (acceptSocket < 0) {
		errno = acceptSocket;
		return -1;
	}

	if (r5compatible && _address != NULL) {
		convert_to_r5_sockaddr(_address, &haikuAddr);
		if (_addressLength != NULL)
			*_addressLength = sizeof(struct r5_sockaddr_in);
	} else if (_addressLength != NULL)
		*_addressLength = addressLength;

	return acceptSocket;
}


extern "C" ssize_t
recv(int socket, void *data, size_t length, int flags)
{
	RETURN_AND_SET_ERRNO_TEST_CANCEL(_kern_recv(socket, data, length, flags));
}


extern "C" ssize_t
recvfrom(int socket, void *data, size_t length, int flags,
	struct sockaddr *_address, socklen_t *_addressLength)
{
	bool r5compatible = check_r5_compatibility();
	struct sockaddr haikuAddr;

	sockaddr* address;
	socklen_t addressLength;

	if (r5compatible && _address != NULL) {
		address = &haikuAddr;
		addressLength = sizeof(haikuAddr);
	} else {
		address = _address;
		addressLength = _addressLength ? *_addressLength : 0;
	}

	ssize_t bytesReceived = _kern_recvfrom(socket, data, length, flags,
		address, &addressLength);

	pthread_testcancel();

	if (bytesReceived < 0) {
		errno = bytesReceived;
		return -1;
	}

	if (r5compatible) {
		convert_to_r5_sockaddr(_address, &haikuAddr);
		if (_addressLength != NULL)
			*_addressLength = sizeof(struct r5_sockaddr_in);
	} else if (_addressLength != NULL)
		*_addressLength = addressLength;

	return bytesReceived;
}


extern "C" ssize_t
recvmsg(int socket, struct msghdr *message, int flags)
{
	RETURN_AND_SET_ERRNO_TEST_CANCEL(_kern_recvmsg(socket, message, flags));
}


extern "C" ssize_t
send(int socket, const void *data, size_t length, int flags)
{
	RETURN_AND_SET_ERRNO_TEST_CANCEL(_kern_send(socket, data, length, flags));
}


extern "C" ssize_t
sendto(int socket, const void *data, size_t length, int flags,
	const struct sockaddr *address, socklen_t addressLength)
{
	struct sockaddr haikuAddr;

	if (check_r5_compatibility()) {
		convert_from_r5_sockaddr(&haikuAddr, address);
		address = &haikuAddr;
		addressLength = sizeof(struct sockaddr_in);
	}

	RETURN_AND_SET_ERRNO_TEST_CANCEL(
		_kern_sendto(socket, data, length, flags, address, addressLength));
}


extern "C" ssize_t
sendmsg(int socket, const struct msghdr *message, int flags)
{
	RETURN_AND_SET_ERRNO_TEST_CANCEL(_kern_sendmsg(socket, message, flags));
}


extern "C" int
getsockopt(int socket, int level, int option, void *value, socklen_t *_length)
{
	if (check_r5_compatibility()) {
		if (option == R5_SO_FIONREAD) {
			// there is no SO_FIONREAD in our stack; we're using FIONREAD instead
			*_length = sizeof(int);
			return ioctl(socket, FIONREAD, &value);
		}

		convert_from_r5_sockopt(level, option);
	}

	RETURN_AND_SET_ERRNO(_kern_getsockopt(socket, level, option, value,
		_length));
}


extern "C" int
setsockopt(int socket, int level, int option, const void *value,
	socklen_t length)
{
	if (check_r5_compatibility())
		convert_from_r5_sockopt(level, option);

	RETURN_AND_SET_ERRNO(_kern_setsockopt(socket, level, option, value,
		length));
}


extern "C" int
getpeername(int socket, struct sockaddr *_address, socklen_t *_addressLength)
{
	bool r5compatible = check_r5_compatibility();
	struct sockaddr haikuAddr;

	sockaddr* address;
	socklen_t addressLength;

	if (r5compatible && _address != NULL) {
		address = &haikuAddr;
		addressLength = sizeof(haikuAddr);
	} else {
		address = _address;
		addressLength = _addressLength ? *_addressLength : 0;
	}

	status_t error = _kern_getpeername(socket, address, &addressLength);
	if (error != B_OK) {
		errno = error;
		return -1;
	}

	if (r5compatible) {
		convert_to_r5_sockaddr(_address, &haikuAddr);
		if (_addressLength != NULL)
			*_addressLength = sizeof(struct r5_sockaddr_in);
	} else if (_addressLength != NULL)
		*_addressLength = addressLength;

	return 0;
}


extern "C" int
getsockname(int socket, struct sockaddr *_address, socklen_t *_addressLength)
{
	bool r5compatible = check_r5_compatibility();
	struct sockaddr haikuAddr;

	sockaddr* address;
	socklen_t addressLength;

	if (r5compatible && _address != NULL) {
		address = &haikuAddr;
		addressLength = sizeof(haikuAddr);
	} else {
		address = _address;
		addressLength = _addressLength ? *_addressLength : 0;
	}

	status_t error = _kern_getsockname(socket, address, &addressLength);
	if (error != B_OK) {
		errno = error;
		return -1;
	}

	if (r5compatible) {
		convert_to_r5_sockaddr(_address, &haikuAddr);
		if (_addressLength != NULL)
			*_addressLength = sizeof(struct r5_sockaddr_in);
	} else if (_addressLength != NULL)
		*_addressLength = addressLength;

	return 0;
}


extern "C" int
sockatmark(int socket)
{
	RETURN_AND_SET_ERRNO(_kern_sockatmark(socket));
}


extern "C" int
socketpair(int family, int type, int protocol, int socketVector[2])
{
	RETURN_AND_SET_ERRNO(_kern_socketpair(family, type, protocol,
		socketVector));
}
