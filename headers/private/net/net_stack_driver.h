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

#define NET_STACK_DRIVER_DEV	"net/stack"
#define NET_STACK_DRIVER_PATH	"/dev/" NET_STACK_DRIVER_DEV

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

/*
	Userland stack driver on_socket_event() callback mecanism implementation:
	the driver start a kernel thread waiting on a port for
	a NET_STACK_SOCKET_EVENT_MSG message, which come with a socket_event_data block.

	The on_socket_event() mechanism stay in driver (kernelland) because it's
	only there we can call kernel's notify_select_event() on BONE systems.
	For non-BONE systems, we use our own r5_notify_select_event()
	implementation, that could be moved into the userland net_server code,
	but it'll have split (again!) the /dev/net/stack driver code...
*/  
   
struct notify_socket_event_args {	// used by NET_STACK_NOTIFY_SOCKET_EVENT
	port_id notify_port;	// port waiting for notification, -1 = stop notify
	void	*cookie;		// this cookie value will be pass back in the socket_event_args
};

#define NET_STACK_SOCKET_EVENT_NOTIFICATION	'sevn'

struct socket_event_data {
	uint32 event;		// B_SELECT_READ, B_SELECT_WRITE or B_SELECT_ERROR
	void	*cookie;	// The cookie as set in notify_socket_event_args for this socket
};

/*
	R5.0.3 and before select() kernel support is too buggy to be used, so
	here are the structures we used to support select() on sockets, and *only* on
	sockets!
*/

struct select_args {	// used by NET_STACK_SELECT and NET_STACK_DESELECT
	struct selectsync *sync;	// in fact, it's the area_id of a r5_selectsync struct!!!
	uint32 ref;
};

struct r5_selectsync {
	sem_id lock;	// lock this r5_selectsync
	sem_id wakeup;	// sem to release to wakeup select()
	struct fd_set rbits;	// read event bits field
	struct fd_set wbits;	// write event bits field
	struct fd_set ebits;	// exception event bits field
};

#endif /* NET_STACK_DRIVER_H */
