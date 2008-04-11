/*
 * Copyright 2008, Haiku, Inc. All Rights Reserved.
 * This file may be used under the terms of the MIT License.
 */
#ifndef NET_STACK_INTERFACE_H
#define NET_STACK_INTERFACE_H


#include <sys/socket.h>


// name of the kernel stack interface
#define NET_STACK_INTERFACE_MODULE_NAME "network/stack/kernel_interface/v1"

// name of the userland stack interface
#define NET_STACK_USERLAND_INTERFACE_MODULE_NAME \
	"network/stack/userland_interface/v1"


struct net_socket;
struct net_stat;


struct net_stack_interface_module_info {
	module_info info;

	status_t (*open)(int family, int type, int protocol, net_socket** _socket);
	status_t (*close)(net_socket* socket);
	status_t (*free)(net_socket* socket);

	status_t (*bind)(net_socket* socket, const struct sockaddr* address,
					socklen_t addressLength);
	status_t (*shutdown)(net_socket* socket, int how);
	status_t (*connect)(net_socket* socket, const struct sockaddr* address,
					socklen_t addressLength);
	status_t (*listen)(net_socket* socket, int backlog);
	status_t (*accept)(net_socket* socket, struct sockaddr* address,
					socklen_t* _addressLength, net_socket** _acceptedSocket);

	ssize_t (*recv)(net_socket* socket, void* data, size_t length, int flags);
	ssize_t (*recvfrom)(net_socket* socket, void* data, size_t length,
					int flags, struct sockaddr* address,
					socklen_t* _addressLength);
	ssize_t (*recvmsg)(net_socket* socket, struct msghdr* message, int flags);

	ssize_t (*send)(net_socket* socket, const void* data, size_t length,
					int flags);
	ssize_t (*sendto)(net_socket* socket, const void* data, size_t length,
					int flags, const struct sockaddr* address,
					socklen_t addressLength);
	ssize_t (*sendmsg)(net_socket* socket, const struct msghdr* message,
					int flags);

	status_t (*getsockopt)(net_socket* socket, int level, int option,
					void* value, socklen_t* _length);
	status_t (*setsockopt)(net_socket* socket, int level, int option,
					const void* value, socklen_t length);

	status_t (*getpeername)(net_socket* socket, struct sockaddr* address,
					socklen_t* _addressLength);
	status_t (*getsockname)(net_socket* socket, struct sockaddr* address,
					socklen_t* _addressLength);

	int (*sockatmark)(net_socket* socket);

	status_t (*socketpair)(int family, int type, int protocol,
					net_socket* _sockets[2]);

	status_t (*ioctl)(net_socket* socket, uint32 op, void *buffer,
					size_t length);
	status_t (*select)(net_socket* socket, uint8 event,
					struct selectsync *sync);
	status_t (*deselect)(net_socket* socket, uint8 event,
					struct selectsync *sync);

	status_t (*get_next_socket_stat)(int family, uint32 *cookie,
					struct net_stat *stat);
};


#endif	// NET_STACK_INTERFACE_H
