/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef NET_SOCKET_H
#define NET_SOCKET_H


#include <net_buffer.h>
#include <sys/socket.h>

#include <lock.h>


struct net_stat;
struct selectsync;


#define NET_SOCKET_MODULE_NAME "network/stack/socket/v1"


typedef struct net_socket {
	struct net_protocol*	first_protocol;
	struct net_protocol_module_info* first_info;

	int						family;
	int						type;
	int						protocol;

	struct sockaddr_storage	address;
	struct sockaddr_storage	peer;

	int						options;
	int						linger;
	uint32					bound_to_device;

	struct {
		uint32		buffer_size;
		uint32		low_water_mark;
		bigtime_t	timeout;
	}						send, receive;

	status_t				error;
} net_socket;


struct net_socket_module_info {
	struct module_info info;

	status_t	(*open_socket)(int family, int type, int protocol,
					net_socket** _socket);
	status_t	(*close)(net_socket* socket);
	void		(*free)(net_socket* socket);

	status_t	(*readv)(net_socket* socket, const iovec* vecs,
					size_t vecCount, size_t* _length);
	status_t	(*writev)(net_socket* socket, const iovec* vecs,
					size_t vecCount, size_t* _length);
	status_t	(*control)(net_socket* socket, int32 op, void* data,
					size_t length);

	ssize_t		(*read_avail)(net_socket* socket);
	ssize_t		(*send_avail)(net_socket* socket);

	status_t	(*send_data)(net_socket* socket, net_buffer* buffer);
	status_t	(*receive_data)(net_socket* socket, size_t length,
					uint32 flags, net_buffer** _buffer);

	status_t	(*get_option)(net_socket* socket, int level, int option,
					void* value, int* _length);
	status_t	(*set_option)(net_socket* socket, int level, int option,
					const void* value, int length);

	status_t	(*get_next_stat)(uint32* cookie, int family,
					struct net_stat* stat);

	// connections
	bool		(*acquire_socket)(net_socket* socket);
	bool		(*release_socket)(net_socket* socket);

	status_t	(*spawn_pending_socket)(net_socket* parent,
					net_socket** _socket);
	status_t	(*dequeue_connected)(net_socket* parent, net_socket** _socket);
	ssize_t		(*count_connected)(net_socket* parent);
	status_t	(*set_max_backlog)(net_socket* socket, uint32 backlog);
	bool		(*has_parent)(net_socket* socket);
	status_t	(*set_connected)(net_socket* socket);
	status_t	(*set_aborted)(net_socket* socket);

	// notifications
	status_t	(*request_notification)(net_socket* socket, uint8 event,
					struct selectsync* sync);
	status_t	(*cancel_notification)(net_socket* socket, uint8 event,
					struct selectsync* sync);
	status_t	(*notify)(net_socket* socket, uint8 event, int32 value);

	// standard socket API
	int			(*accept)(net_socket* socket, struct sockaddr* address,
					socklen_t* _addressLength, net_socket** _acceptedSocket);
	int			(*bind)(net_socket* socket, const struct sockaddr* address,
					socklen_t addressLength);
	int			(*connect)(net_socket* socket, const struct sockaddr* address,
					socklen_t addressLength);
	int			(*getpeername)(net_socket* socket, struct sockaddr* address,
					socklen_t* _addressLength);
	int			(*getsockname)(net_socket* socket, struct sockaddr* address,
					socklen_t* _addressLength);
	int			(*getsockopt)(net_socket* socket, int level, int option,
					void* optionValue, int* _optionLength);
	int			(*listen)(net_socket* socket, int backlog);
	ssize_t		(*receive)(net_socket* socket, struct msghdr* , void* data,
					size_t length, int flags);
	ssize_t		(*send)(net_socket* socket, struct msghdr* , const void* data,
					size_t length, int flags);
	int			(*setsockopt)(net_socket* socket, int level, int option,
					const void* optionValue, int optionLength);
	int			(*shutdown)(net_socket* socket, int direction);
	status_t	(*socketpair)(int family, int type, int protocol,
					net_socket* _sockets[2]);
};


#endif	// NET_SOCKET_H
