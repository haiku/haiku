/* Copyright 2007, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SOCKET_INTERFACE_H
#define _SOCKET_INTERFACE_H

//! Kernel interface to the socket API


#include <module.h>

#include <sys/socket.h>


#define B_SOCKET_MODULE_NAME "network/socket/v1"

typedef struct socket_module_info {
	struct module_info	info;

	int 	(*accept)(int socket, struct sockaddr *address, socklen_t *_addressLength);
	int		(*bind)(int socket, const struct sockaddr *address, socklen_t addressLength);
	int		(*connect)(int socket, const struct sockaddr *address, socklen_t addressLength);
	int     (*getpeername)(int socket, struct sockaddr *address, socklen_t *_addressLength);
	int     (*getsockname)(int socket, struct sockaddr *address, socklen_t *_addressLength);
	int     (*getsockopt)(int socket, int level, int option, void *value, socklen_t *_length);
	int		(*listen)(int socket, int backlog);
	ssize_t (*recv)(int socket, void *buffer, size_t length, int flags);
	ssize_t (*recvfrom)(int socket, void *buffer, size_t bufferLength, int flags,
				struct sockaddr *address, socklen_t *_addressLength);
	ssize_t (*recvmsg)(int socket, struct msghdr *message, int flags);
	ssize_t (*send)(int socket, const void *buffer, size_t length, int flags);
	ssize_t	(*sendmsg)(int socket, const struct msghdr *message, int flags);
	ssize_t (*sendto)(int socket, const void *message, size_t length, int flags,
				const struct sockaddr *address, socklen_t addressLength);
	int     (*setsockopt)(int socket, int level, int option, const void *value,
				socklen_t length);
	int		(*shutdown)(int socket, int how);
	int		(*socket)(int domain, int type, int protocol);
	int		(*sockatmark)(int socket);
	int		(*socketpair)(int domain, int type, int protocol, int socketVector[2]);
};

#endif	// _SOCKET_INTERFACE_H
