/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * This file may be used under the terms of the MIT License.
 */
#ifndef NET_STACK_DRIVER_H
#define NET_STACK_DRIVER_H


#include <OS.h>			

#include <sys/select.h>
#include <sys/socket.h>


// Forward declaration
struct sockaddr; 

#define NET_STACK_DRIVER_DEVICE	"net/stack"
#define NET_STACK_DRIVER_PATH	"/dev/" NET_STACK_DRIVER_DEVICE

enum {
	NET_STACK_IOCTL_BASE = 8800,
	NET_STACK_IOCTL_END = 8999,

	// ops not acting on an existing socket
	NET_STACK_SOCKET = NET_STACK_IOCTL_BASE,	// socket_args *
	NET_STACK_GET_COOKIE,                       // void ** 
	NET_STACK_CONTROL_NET_MODULE,				// control_net_module_args *
	NET_STACK_SYSCTL,							// sysctl_args *
	
	// ops acting on an existing socket
	NET_STACK_BIND,								// sockaddr_args *
	NET_STACK_RECVFROM,							// struct msghdr *
	NET_STACK_RECV,								// transfer_args *
	NET_STACK_SENDTO,							// struct msghdr *
	NET_STACK_SEND,								// transfer_args *
	NET_STACK_LISTEN,							// int_args * (value = backlog)
	NET_STACK_ACCEPT,							// sockaddr_args *
	NET_STACK_CONNECT,							// sockaddr_args *
	NET_STACK_SHUTDOWN,							// int_args * (value = how)
	NET_STACK_GETSOCKOPT,						// sockopt_args *
	NET_STACK_SETSOCKOPT,						// sockopt_args *
	NET_STACK_GETSOCKNAME,						// sockaddr_args *
	NET_STACK_GETPEERNAME,						// sockaddr_args *
	NET_STACK_SOCKETPAIR,						// socketpair_args *
	
	// TODO: remove R5 select() emulation
	NET_STACK_SELECT,							// select_args *
	NET_STACK_DESELECT,							// select_args *
	
	NET_STACK_NOTIFY_SOCKET_EVENT,				// notify_socket_event_args * (userland stack only)
	
	NET_STACK_IOCTL_MAX
};

struct sockaddr_args {	// used by NET_STACK_CONNECT/_BIND/_GETSOCKNAME/_GETPEERNAME
	struct sockaddr *address;
	socklen_t address_length;
};

struct sockopt_args {	// used by NET_STACK_SETSOCKOPT/_GETSOCKOPT
	int		level;
	int		option;
	void	*value;
	int		length;
};

struct transfer_args {	// used by NET_STACK_SEND/_RECV
	void	*data;
	size_t	data_length;
	int		flags;
	struct sockaddr *address;	// only used for recvfrom() and sendto()
	socklen_t address_length;	// ""
};

struct socket_args {	// used by NET_STACK_SOCKET
	int		family;
	int		type;
	int		protocol;
};

struct socketpair_args {	// used by NET_STACK_SOCKETPAIR
	void	*cookie;
};

struct accept_args {  // used by NET_STACK_ACCEPT 
	void	*cookie; 
	struct sockaddr *address; 
	socklen_t address_length; 
};

struct sysctl_args {	// used by NET_STACK_SYSCTL
	int		*name;
	uint	namelen;
	void	*oldp;
	size_t	*oldlenp;
	void	*newp;
	size_t	newlen;
};

struct control_net_module_args {	// used by NET_STACK_CONTROL_NET_MODULE
	const char *name;
	uint32	op;
	void	*data;
	size_t	length;
};

#endif /* NET_STACK_DRIVER_H */
