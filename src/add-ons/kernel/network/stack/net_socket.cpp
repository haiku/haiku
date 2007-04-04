/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "stack_private.h"

#include <net_protocol.h>
#include <net_stack.h>
#include <net_stat.h>

#include <KernelExport.h>
#include <team.h>
#include <util/AutoLock.h>
#include <util/list.h>
#include <fs/select_sync_pool.h>

#include <new>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>


struct net_socket_private : net_socket {
	struct list_link		link;
	team_id					owner;
	uint32					max_backlog;
	uint32					child_count;
	struct list				pending_children;
	struct list				connected_children;

	struct select_sync_pool	*select_pool;
	benaphore				lock;
};


void socket_delete(net_socket *socket);
int socket_bind(net_socket *socket, const struct sockaddr *address, socklen_t addressLength);

struct list sSocketList;
benaphore sSocketLock;


static status_t
create_socket(int family, int type, int protocol, net_socket_private **_socket)
{
	struct net_socket_private *socket = new (std::nothrow) net_socket_private;
	if (socket == NULL)
		return B_NO_MEMORY;

	memset(socket, 0, sizeof(net_socket_private));
	socket->family = family;
	socket->type = type;
	socket->protocol = protocol;

	status_t status = benaphore_init(&socket->lock, "socket");
	if (status < B_OK)
		goto err1;

	// set defaults (may be overridden by the protocols)
	socket->send.buffer_size = 65535;
	socket->send.low_water_mark = 1;
	socket->send.timeout = B_INFINITE_TIMEOUT;
	socket->receive.buffer_size = 65535;
	socket->receive.low_water_mark = 1;
	socket->receive.timeout = B_INFINITE_TIMEOUT;

	list_init_etc(&socket->pending_children, offsetof(net_socket_private, link));
	list_init_etc(&socket->connected_children, offsetof(net_socket_private, link));

	status = get_domain_protocols(socket);
	if (status < B_OK)
		goto err2;

	*_socket = socket;
	return B_OK;

err2:
	benaphore_destroy(&socket->lock);
err1:
	delete socket;
	return status;
}


//	#pragma mark -


status_t
socket_open(int family, int type, int protocol, net_socket **_socket)
{
	net_socket_private *socket;
	status_t status = create_socket(family, type, protocol, &socket);
	if (status < B_OK)
		return status;
	
	status = socket->first_info->open(socket->first_protocol);
	if (status < B_OK) {
		socket_delete(socket);
		return status;
	}

	socket->owner = team_get_current_team_id();

	benaphore_lock(&sSocketLock);
	list_add_item(&sSocketList, socket);
	benaphore_unlock(&sSocketLock);

	*_socket = socket;
	return B_OK;
}


status_t
socket_close(net_socket *_socket)
{
	net_socket_private *socket = (net_socket_private *)_socket;

	if (socket->select_pool) {
		// notify all pending selects
		notify_select_event_pool(socket->select_pool, ~0);
	}

	return socket->first_info->close(socket->first_protocol);
}


status_t
socket_free(net_socket *socket)
{
	status_t status = socket->first_info->free(socket->first_protocol);
	if (status == B_BUSY)
		return B_OK;

	socket_delete(socket);
	return B_OK;
}


status_t
socket_readv(net_socket *socket, const iovec *vecs, size_t vecCount, size_t *_length)
{
	return -1;
}


status_t
socket_writev(net_socket *socket, const iovec *vecs, size_t vecCount, size_t *_length)
{
	if (socket->peer.ss_len == 0)
		return ECONNRESET;

	if (socket->address.ss_len == 0) {
		// try to bind first
		status_t status = socket_bind(socket, NULL, 0);
		if (status < B_OK)
			return status;
	}

	// TODO: useful, maybe even computed header space!
	net_buffer *buffer = gNetBufferModule.create(256);
	if (buffer == NULL)
		return ENOBUFS;

	// copy data into buffer

	for (uint32 i = 0; i < vecCount; i++) {
		if (gNetBufferModule.append(buffer, vecs[i].iov_base,
				vecs[i].iov_len) < B_OK) {
			gNetBufferModule.free(buffer);
			return ENOBUFS;
		}
	}

	memcpy(&buffer->source, &socket->address, socket->address.ss_len);
	memcpy(&buffer->destination, &socket->peer, socket->peer.ss_len);
	size_t size = buffer->size;

	ssize_t bytesWritten = socket->first_info->send_data(socket->first_protocol,
		buffer);
	if (bytesWritten < B_OK) {
		if (buffer->size != size) {
			// this appears to be a partial write
			*_length = size - buffer->size;
		}
		gNetBufferModule.free(buffer);
		return bytesWritten;
	}

	*_length = bytesWritten;
	return B_OK;
}


status_t
socket_control(net_socket *socket, int32 op, void *data, size_t length)
{
	return socket->first_info->control(socket->first_protocol,
		LEVEL_DRIVER_IOCTL, op, data, &length);
}


ssize_t
socket_read_avail(net_socket *socket)
{
	return socket->first_info->read_avail(socket->first_protocol);
}


ssize_t
socket_send_avail(net_socket *socket)
{
	return socket->first_info->send_avail(socket->first_protocol);
}


status_t
socket_send_data(net_socket *socket, net_buffer *buffer)
{
	return socket->first_info->send_data(socket->first_protocol,
		buffer);
}


status_t
socket_receive_data(net_socket *socket, size_t length, uint32 flags,
	net_buffer **_buffer)
{
	return socket->first_info->read_data(socket->first_protocol,
		length, flags, _buffer);
}


status_t
socket_get_next_stat(uint32 *_cookie, int family, struct net_stat *stat)
{
	BenaphoreLocker locker(sSocketLock);

	net_socket_private *socket = NULL;
	uint32 cookie = *_cookie;
	uint32 count = 0;
	while ((socket = (net_socket_private *)list_get_next_item(&sSocketList, socket)) != NULL) {
		// TODO: also traverse the pending connections
		if (count == cookie)
			break;

		if (family == -1 || family == socket->family)
			count++;
	}

	if (socket == NULL)
		return B_ENTRY_NOT_FOUND;

	*_cookie = count + 1;

	stat->family = socket->family;
	stat->type = socket->type;
	stat->protocol = socket->protocol;
	stat->owner = socket->owner;
	stat->state[0] = '\0';
	memcpy(&stat->address, &socket->address, sizeof(struct sockaddr_storage));
	memcpy(&stat->peer, &socket->peer, sizeof(struct sockaddr_storage));

	// fill in protocol specific data (if supported by the protocol)
	size_t length = sizeof(net_stat);
	socket->first_info->control(socket->first_protocol, socket->protocol,
		NET_STAT_SOCKET, stat, &length);

	return B_OK;
}


//	#pragma mark - connections


status_t
socket_spawn_pending(net_socket *_parent, net_socket **_socket)
{
	net_socket_private *parent = (net_socket_private *)_parent;

	BenaphoreLocker locker(parent->lock);

	// We actually accept more pending connections to compensate for those
	// that never complete, and also make sure at least a single connection
	// can always be accepted
	if (parent->child_count > 3 * parent->max_backlog / 2)
		return ENOBUFS;

	net_socket_private *socket;
	status_t status = create_socket(parent->family, parent->type, parent->protocol,
		&socket);
	if (status < B_OK)
		return status;

	// inherit parent's properties
	socket->send = parent->send;
	socket->receive = parent->receive;
	socket->options = parent->options & ~SO_ACCEPTCONN;
	socket->linger = parent->linger;
	socket->owner = parent->owner;
	memcpy(&socket->address, &parent->address, parent->address.ss_len);
	memcpy(&socket->peer, &parent->peer, parent->peer.ss_len);

	// add to the parent's list of pending connections
	list_add_item(&parent->pending_children, socket);
	socket->parent = parent;
	parent->child_count++;

	*_socket = socket;
	return B_OK;
}


void
socket_delete(net_socket *_socket)
{
	net_socket_private *socket = (net_socket_private *)_socket;

	if (socket->parent != NULL)
		panic("socket still has a parent!");

	benaphore_lock(&sSocketLock);
	list_remove_item(&sSocketList, socket);
	benaphore_unlock(&sSocketLock);

	put_domain_protocols(socket);
	benaphore_destroy(&socket->lock);
	delete_select_sync_pool(socket->select_pool);
	delete socket;
}


status_t
socket_dequeue_connected(net_socket *_parent, net_socket **_socket)
{
	net_socket_private *parent = (net_socket_private *)_parent;

	benaphore_lock(&parent->lock);

	net_socket_private *socket = (net_socket_private *)list_remove_head_item(
		&parent->connected_children);
	if (socket != NULL) {
		socket->parent = NULL;
		parent->child_count--;
		*_socket = socket;
	}

	benaphore_unlock(&parent->lock);

	if (socket == NULL)
		return B_ENTRY_NOT_FOUND;

	benaphore_lock(&sSocketLock);
	list_add_item(&sSocketList, socket);
	benaphore_unlock(&sSocketLock);

	return B_OK;
}


status_t
socket_set_max_backlog(net_socket *_socket, uint32 backlog)
{
	net_socket_private *socket = (net_socket_private *)_socket;

	// we enforce an upper limit of connections waiting to be accepted
	if (backlog > 256)
		backlog = 256;

	benaphore_lock(&socket->lock);

	// first remove the pending connections, then the already connected ones as needed	
	net_socket_private *child;
	while (socket->child_count > backlog
		&& (child = (net_socket_private *)list_remove_tail_item(&socket->pending_children)) != NULL) {
		child->parent = NULL;
		socket->child_count--;
	}
	while (socket->child_count > backlog
		&& (child = (net_socket_private *)list_remove_tail_item(&socket->connected_children)) != NULL) {
		child->parent = NULL;
		socket_delete(child);
		socket->child_count--;
	}

	socket->max_backlog = backlog;
	benaphore_unlock(&socket->lock);
	return B_OK;
}


/*!
	The socket has been connected. It will be moved to the connected queue
	of its parent socket.
*/
status_t
socket_connected(net_socket *socket)
{
	net_socket_private *parent = (net_socket_private *)socket->parent;
	if (parent == NULL)
		return B_BAD_VALUE;

	benaphore_lock(&parent->lock);

	list_remove_item(&parent->pending_children, socket);
	list_add_item(&parent->connected_children, socket);

	// notify parent
	if (parent->select_pool)
		notify_select_event_pool(parent->select_pool, B_SELECT_READ);

	benaphore_unlock(&parent->lock);
	return B_OK;
}


//	#pragma mark - notifications


status_t
socket_request_notification(net_socket *_socket, uint8 event, uint32 ref,
	selectsync *sync)
{
	net_socket_private *socket = (net_socket_private *)_socket;

	benaphore_lock(&socket->lock);

	status_t status = add_select_sync_pool_entry(&socket->select_pool, sync,
		ref, event);

	benaphore_unlock(&socket->lock);

	if (status < B_OK)
		return status;

	// check if the event is already present
	// TODO: add support for poll() types

	switch (event) {
		case B_SELECT_READ:
		{
			ssize_t available = socket_read_avail(socket);
			if ((ssize_t)socket->receive.low_water_mark <= available || available < B_OK)
				notify_select_event(sync, ref, event);
			break;
		}
		case B_SELECT_WRITE:
		{
			ssize_t available = socket_send_avail(socket);
			if ((ssize_t)socket->send.low_water_mark <= available || available < B_OK)
				notify_select_event(sync, ref, event);
			break;
		}
		case B_SELECT_ERROR:
			// TODO: B_SELECT_ERROR condition!
			break;
	}

	return B_OK;
}


status_t
socket_cancel_notification(net_socket *_socket, uint8 event, selectsync *sync)
{
	net_socket_private *socket = (net_socket_private *)_socket;

	benaphore_lock(&socket->lock);

	status_t status = remove_select_sync_pool_entry(&socket->select_pool,
		sync, event);

	benaphore_unlock(&socket->lock);
	return status;
}


status_t
socket_notify(net_socket *_socket, uint8 event, int32 value)
{
	net_socket_private *socket = (net_socket_private *)_socket;
	bool notify = true;

	switch (event) {
		case B_SELECT_READ:
			if ((ssize_t)socket->receive.low_water_mark > value && value >= B_OK)
				notify = false;
			break;

		case B_SELECT_WRITE:
			if ((ssize_t)socket->send.low_water_mark > value && value >= B_OK)
				notify = false;
			break;

		case B_SELECT_ERROR:
			socket->error = value;
			break;
	}

	benaphore_lock(&socket->lock);

	if (notify && socket->select_pool)
		notify_select_event_pool(socket->select_pool, event);

	benaphore_unlock(&socket->lock);
	return B_OK;
}


//	#pragma mark - standard socket API


int
socket_accept(net_socket *socket, struct sockaddr *address, socklen_t *_addressLength,
	net_socket **_acceptedSocket)
{
	if ((socket->options & SO_ACCEPTCONN) == 0)
		return B_BAD_VALUE;

	net_socket *accepted;
	status_t status = socket->first_info->accept(socket->first_protocol,
		&accepted);
	if (status < B_OK)
		return status;

	if (address && *_addressLength > 0) {
		memcpy(address, &accepted->peer, min_c(*_addressLength,
			min_c(accepted->peer.ss_len, sizeof(sockaddr_storage))));
		*_addressLength = accepted->peer.ss_len;
	}

	*_acceptedSocket = accepted;
	return B_OK;
}


int
socket_bind(net_socket *socket, const struct sockaddr *address, socklen_t addressLength)
{
	sockaddr empty;
	if (address == NULL) {
		// special - try to bind to an empty address, like INADDR_ANY
		memset(&empty, 0, sizeof(sockaddr));
		empty.sa_len = sizeof(sockaddr);
		empty.sa_family = socket->family;

		address = &empty;
		addressLength = sizeof(sockaddr);
	}

	if (socket->address.ss_len != 0) {
		status_t status = socket->first_info->unbind(socket->first_protocol,
			(sockaddr *)&socket->address);
		if (status < B_OK)
			return status;
	}

	memcpy(&socket->address, address, sizeof(sockaddr));

	status_t status = socket->first_info->bind(socket->first_protocol, 
		(sockaddr *)address);
	if (status < B_OK) {
		// clear address again, as binding failed
		socket->address.ss_len = 0;
	}

	return status;
}


int
socket_connect(net_socket *socket, const struct sockaddr *address, socklen_t addressLength)
{
	if (address == NULL || addressLength == 0)
		return ENETUNREACH;

	if (socket->address.ss_len == 0) {
		// try to bind first
		status_t status = socket_bind(socket, NULL, 0);
		if (status < B_OK)
			return status;
	}

	return socket->first_info->connect(socket->first_protocol, address);
}


int
socket_getpeername(net_socket *socket, struct sockaddr *address, socklen_t *_addressLength)
{
	if (socket->peer.ss_len == 0)
		return ENOTCONN;

	memcpy(address, &socket->peer, min_c(*_addressLength, socket->peer.ss_len));
	*_addressLength = socket->peer.ss_len;
	return B_OK;
}


int
socket_getsockname(net_socket *socket, struct sockaddr *address, socklen_t *_addressLength)
{
	if (socket->address.ss_len == 0)
		return ENOTCONN;

	memcpy(address, &socket->address, min_c(*_addressLength, socket->address.ss_len));
	*_addressLength = socket->address.ss_len;
	return B_OK;
}


int
socket_getsockopt(net_socket *socket, int level, int option, void *value,
	int *_length)
{
	if (level != SOL_SOCKET) {
		return socket->first_info->control(socket->first_protocol,
			level | LEVEL_GET_OPTION, option, value, (size_t *)_length);
	}

	switch (option) {
		case SO_SNDBUF:
		{
			uint32 *size = (uint32 *)value;
			*size = socket->send.buffer_size;
			*_length = sizeof(uint32);
			return B_OK;
		}

		case SO_RCVBUF:
		{
			uint32 *size = (uint32 *)value;
			*size = socket->receive.buffer_size;
			*_length = sizeof(uint32);
			return B_OK;
		}

		case SO_SNDLOWAT:
		{
			uint32 *size = (uint32 *)value;
			*size = socket->send.low_water_mark;
			*_length = sizeof(uint32);
			return B_OK;
		}

		case SO_RCVLOWAT:
		{
			uint32 *size = (uint32 *)value;
			*size = socket->receive.low_water_mark;
			*_length = sizeof(uint32);
			return B_OK;
		}

		case SO_RCVTIMEO:
		case SO_SNDTIMEO:
		{
			if (*_length < (int)sizeof(struct timeval))
				return B_BAD_VALUE;

			bigtime_t timeout;
			if (option == SO_SNDTIMEO)
				timeout = socket->send.timeout;
			else
				timeout = socket->receive.timeout;
			if (timeout == B_INFINITE_TIMEOUT)
				timeout = 0;

			struct timeval *timeval = (struct timeval *)value;
			timeval->tv_sec = timeout / 1000000LL;
			timeval->tv_usec = timeout % 1000000LL;

			*_length = sizeof(struct timeval);
			return B_OK;
		}

		case SO_NONBLOCK:
		{
			int32 *_set = (int32 *)value;
			*_set = socket->receive.timeout == 0 && socket->send.timeout == 0;
			*_length = sizeof(int32);
			return B_OK;
		}

		case SO_ACCEPTCONN:
		case SO_BROADCAST:
		case SO_DEBUG:
		case SO_DONTROUTE:
		case SO_KEEPALIVE:
		case SO_OOBINLINE:
		case SO_REUSEADDR:
		case SO_REUSEPORT:
		case SO_USELOOPBACK:
		{
			int32 *_set = (int32 *)value;
			*_set = (socket->options & option) != 0;
			*_length = sizeof(int32);
			return B_OK;
		}

		case SO_ERROR:
		{
			int32 *_set = (int32 *)value;
			*_set = socket->error;
			*_length = sizeof(int32);

			socket->error = B_OK;
				// clear error upon retrieval
			return B_OK;
		}

		default:
			break;
	}

	dprintf("socket_getsockopt: unknown option %d\n", option);
	return ENOPROTOOPT;
}


int
socket_listen(net_socket *socket, int backlog)
{
	status_t status = socket->first_info->listen(socket->first_protocol, backlog);
	if (status == B_OK)
		socket->options |= SO_ACCEPTCONN;

	return status;
}


ssize_t
socket_recv(net_socket *socket, void *data, size_t length, int flags)
{
	net_buffer *buffer;
	status_t status = socket->first_info->read_data(
		socket->first_protocol, length, flags, &buffer);
	if (status < B_OK)
		return status;

	// if 0 bytes we're received, no buffer will be created
	if (buffer == NULL)
		return 0;

	ssize_t bytesReceived = buffer->size;
	gNetBufferModule.read(buffer, 0, data, bytesReceived);
	gNetBufferModule.free(buffer);

	return bytesReceived;
}


ssize_t
socket_recvfrom(net_socket *socket, void *data, size_t length, int flags,
	struct sockaddr *address, socklen_t *_addressLength)
{
	net_buffer *buffer;
	status_t status = socket->first_info->read_data(
		socket->first_protocol, length, flags, &buffer);
	if (status < B_OK)
		return status;

	// if 0 bytes we're received, no buffer will be created
	if (buffer == NULL)
		return 0;

	ssize_t bytesReceived = buffer->size;
	gNetBufferModule.read(buffer, 0, data, bytesReceived);

	// copy source address
	if (address != NULL && *_addressLength > 0) {
		*_addressLength = min_c(buffer->source.ss_len, *_addressLength);
		memcpy(address, &buffer->source, *_addressLength);
	}

	gNetBufferModule.free(buffer);

	return bytesReceived;
}


ssize_t
socket_recvmsg(net_socket *socket, msghdr *header, int flags)
{
	net_buffer *buffer;
	iovec tmp;
	int i;

	size_t length = 0;
	for (i = 0; i < header->msg_iovlen; i++) {
		if (user_memcpy(&tmp, header->msg_iov + i, sizeof(iovec)) < B_OK)
			return B_BAD_ADDRESS;
		if (tmp.iov_len > 0 && tmp.iov_base == NULL)
			return B_BAD_ADDRESS;
		length += tmp.iov_len;
	}

	status_t status = socket->first_info->read_data(
		socket->first_protocol, length, flags, &buffer);
	if (status < B_OK)
		return status;

	// TODO: - consider the control buffer options
	//       - datagram based protocols should return the
	//         full datagram so we can cut it here with MSG_TRUNC
	//       - returning a NULL buffer when received 0 bytes
	//         may not make much sense as we still need the address
	//       - gNetBufferModule.read() uses memcpy() instead of user_memcpy

	size_t nameLen = header->msg_namelen;
	header->msg_namelen = 0;
	header->msg_flags = 0;

	if (buffer == NULL)
		return 0;

	size_t bufferSize = buffer->size, bytesReceived = 0;
	for (i = 0; i < header->msg_iovlen && bytesReceived < bufferSize; i++) {
		if (user_memcpy(&tmp, header->msg_iov + i, sizeof(iovec)) < B_OK)
			break;

		size_t toRead = min_c(bufferSize - bytesReceived, tmp.iov_len);
		if (gNetBufferModule.read(buffer, bytesReceived, tmp.iov_base,
									toRead) < B_OK)
			break;

		bytesReceived += toRead;
	}

	if (bytesReceived == buffer->size && header->msg_name != NULL) {
		header->msg_namelen = min_c(nameLen, buffer->source.ss_len);
		memcpy(header->msg_name, &buffer->source, header->msg_namelen);
	}

	gNetBufferModule.free(buffer);

	if (bytesReceived < bufferSize)
		return ENOBUFS;

	return bytesReceived;
}


ssize_t
socket_send(net_socket *socket, const void *data, size_t length, int flags)
{
	if (socket->peer.ss_len == 0)
		return EDESTADDRREQ;

	if (socket->address.ss_len == 0) {
		// try to bind first
		status_t status = socket_bind(socket, NULL, 0);
		if (status < B_OK)
			return status;
	}

	// TODO: useful, maybe even computed header space!
	net_buffer *buffer = gNetBufferModule.create(256);
	if (buffer == NULL)
		return ENOBUFS;

	// copy data into buffer
	if (gNetBufferModule.append(buffer, data, length) < B_OK) {
		gNetBufferModule.free(buffer);
		return ENOBUFS;
	}

	buffer->flags = flags;
	memcpy(&buffer->source, &socket->address, socket->address.ss_len);
	memcpy(&buffer->destination, &socket->peer, socket->peer.ss_len);

	status_t status = socket->first_info->send_data(socket->first_protocol,
		buffer);
	if (status < B_OK) {
		size_t size = buffer->size;
		gNetBufferModule.free(buffer);

		if (size != length && (status == B_INTERRUPTED || status == B_WOULD_BLOCK)) {
			// this appears to be a partial write
			return length - size;
		}

		return status;
	}

	return length;
}


ssize_t
socket_sendto(net_socket *socket, const void *data, size_t length, int flags,
	const struct sockaddr *address, socklen_t addressLength)
{
	if ((address == NULL || addressLength == 0) && socket->peer.ss_len != 0) {
		// socket is connected, we use that address:
		address = (struct sockaddr *)&socket->peer;
		addressLength = socket->peer.ss_len;
	}
	if (address == NULL || addressLength == 0) {
		// don't know where to send to:
		return EDESTADDRREQ;
	}
	if (socket->peer.ss_len != 0) {
		// an address has been given but socket is connected already:
		return EISCONN;
	}

	if (socket->address.ss_len == 0) {
		// try to bind first
		status_t status = socket_bind(socket, NULL, 0);
		if (status < B_OK)
			return status;
	}

	// TODO: useful, maybe even computed header space!
	net_buffer *buffer = gNetBufferModule.create(256);
	if (buffer == NULL)
		return ENOBUFS;

	// copy data into buffer
	if (gNetBufferModule.append(buffer, data, length) < B_OK) {
		gNetBufferModule.free(buffer);
		return ENOBUFS;
	}

	buffer->flags = flags;
	memcpy(&buffer->source, &socket->address, socket->address.ss_len);
	memcpy(&buffer->destination, address, addressLength);

	status_t status = socket->first_info->send_data(socket->first_protocol,
		buffer);
	if (status < B_OK) {
		size_t size = buffer->size;
		gNetBufferModule.free(buffer);

		if (size != length && (status == B_INTERRUPTED || status == B_WOULD_BLOCK)) {
			// this appears to be a partial write
			return length - size;
		}
		return status;
	}

	return length;
}


status_t
socket_sendmsg(net_socket *socket, msghdr *header, int flags)
{
	const sockaddr *address = (const sockaddr *)header->msg_name;
	socklen_t addressLength = header->msg_namelen;

	if (addressLength == 0)
		address = NULL;
	else if (addressLength != 0 && address == NULL)
		return B_BAD_VALUE;

	if (socket->peer.ss_len != 0) {
		if (address != NULL)
			return EISCONN;

		// socket is connected, we use that address
		address = (struct sockaddr *)&socket->peer;
		addressLength = socket->peer.ss_len;
	}
	if (address == NULL || addressLength == 0) {
		// don't know where to send to:
		return EDESTADDRREQ;
	}

	if (socket->address.ss_len == 0) {
		// try to bind first
		status_t status = socket_bind(socket, NULL, 0);
		if (status < B_OK)
			return status;
	}

	// TODO: useful, maybe even computed header space!
	net_buffer *buffer = gNetBufferModule.create(256);
	if (buffer == NULL)
		return ENOBUFS;

	size_t length = 0;

	// copy data into buffer
	for (int i = 0; i < header->msg_iovlen; i++) {
		iovec tmp;
		if (user_memcpy(&tmp, header->msg_iov + i, sizeof(iovec)) < B_OK ||
			gNetBufferModule.append(buffer, tmp.iov_base, tmp.iov_len) < B_OK) {
			gNetBufferModule.free(buffer);
			return ENOBUFS;
		}

		length += tmp.iov_len;
	}

	buffer->flags = flags;
	memcpy(&buffer->source, &socket->address, socket->address.ss_len);
	memcpy(&buffer->destination, address, addressLength);

	status_t status = socket->first_info->send_data(socket->first_protocol,
		buffer);
	if (status < B_OK) {
		size_t size = buffer->size;
		gNetBufferModule.free(buffer);

		if (size != length && (status == B_INTERRUPTED || status == B_WOULD_BLOCK)) {
			// this appears to be a partial write
			return length - size;
		}
		return status;
	}

	return length;
}


int
socket_setsockopt(net_socket *socket, int level, int option, const void *value,
	int length)
{
	if (level != SOL_SOCKET) {
		return socket->first_info->control(socket->first_protocol,
			level | LEVEL_SET_OPTION, option, (void *)value, (size_t *)&length);
	}

	switch (option) {
		// TODO: implement other options!
		case SO_LINGER:
		{
			if (length < (int)sizeof(struct linger))
				return B_BAD_VALUE;

			struct linger *linger = (struct linger *)value;
			if (linger->l_onoff) {
				socket->options |= SO_LINGER;
				socket->linger = linger->l_linger;
			} else {
				socket->options &= ~SO_LINGER;
				socket->linger = 0;
			}
			return B_OK;
		}

		case SO_SNDBUF:
			// TODO: should be handled in the protocol modules - they can actually
			//	check if setting the value is allowed and within valid bounds.
			if (length != sizeof(uint32))
				return B_BAD_VALUE;

			socket->send.buffer_size = *(const uint32 *)value;
			return B_OK;

		case SO_RCVBUF:
			// TODO: see above (SO_SNDBUF)
			if (length != sizeof(uint32))
				return B_BAD_VALUE;

			socket->receive.buffer_size = *(const uint32 *)value;
			return B_OK;

		case SO_SNDLOWAT:
			// TODO: see above (SO_SNDBUF)
			if (length != sizeof(uint32))
				return B_BAD_VALUE;

			socket->send.low_water_mark = *(const uint32 *)value;
			return B_OK;

		case SO_RCVLOWAT:
			// TODO: see above (SO_SNDBUF)
			if (length != sizeof(uint32))
				return B_BAD_VALUE;

			socket->receive.low_water_mark = *(const uint32 *)value;
			return B_OK;

		case SO_RCVTIMEO:
		case SO_SNDTIMEO:
		{
			if (length != sizeof(struct timeval))
				return B_BAD_VALUE;

			const struct timeval *timeval = (const struct timeval *)value;
			bigtime_t timeout = timeval->tv_sec * 1000000LL + timeval->tv_usec;
			if (timeout == 0)
				timeout = B_INFINITE_TIMEOUT;

			if (option == SO_SNDTIMEO)
				socket->send.timeout = timeout;
			else
				socket->receive.timeout = timeout;
			return B_OK;
		}

		case SO_NONBLOCK:
			if (length != sizeof(int32))
				return B_BAD_VALUE;

			if (*(const int32 *)value) {
				socket->send.timeout = 0;
				socket->receive.timeout = 0;
			} else {
				socket->send.timeout = B_INFINITE_TIMEOUT;
				socket->receive.timeout = B_INFINITE_TIMEOUT;
			}
			return B_OK;

		case SO_BROADCAST:
		case SO_DEBUG:
		case SO_DONTROUTE:
		case SO_KEEPALIVE:
		case SO_OOBINLINE:
		case SO_REUSEADDR:
		case SO_REUSEPORT:
		case SO_USELOOPBACK:
			if (length != sizeof(int32))
				return B_BAD_VALUE;

			if (*(const int32 *)value)
				socket->options |= option;
			else
				socket->options &= ~option;
			return B_OK;

		default:
			break;
	}

	dprintf("socket_setsockopt: unknown option %d\n", option);
	return ENOPROTOOPT;
}


int
socket_shutdown(net_socket *socket, int direction)
{
	return socket->first_info->shutdown(socket->first_protocol, direction);
}


//	#pragma mark -


static status_t
socket_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			// TODO: this is currently done in the net_stack driver
			// initialize the main stack if not done so already
			//module_info *module;
			//return get_module(NET_STARTER_MODULE_NAME, &module);
			list_init_etc(&sSocketList, offsetof(net_socket_private, link));
			return benaphore_init(&sSocketLock, "socket list");
		}
		case B_MODULE_UNINIT:
			//return put_module(NET_STARTER_MODULE_NAME);
			benaphore_destroy(&sSocketLock);
			return B_OK;

		default:
			return B_ERROR;
	}
}


net_socket_module_info gNetSocketModule = {
	{
		NET_SOCKET_MODULE_NAME,
		0,
		socket_std_ops
	},
	socket_open,
	socket_close,
	socket_free,

	socket_readv,
	socket_writev,
	socket_control,

	socket_read_avail,
	socket_send_avail,

	socket_send_data,
	socket_receive_data,

	socket_get_next_stat,

	// connections
	socket_spawn_pending,
	socket_delete,
	socket_dequeue_connected,
	socket_set_max_backlog,
	socket_connected,

	// notifications
	socket_request_notification,
	socket_cancel_notification,
	socket_notify,

	// standard socket API
	socket_accept,
	socket_bind,
	socket_connect,
	socket_getpeername,
	socket_getsockname,
	socket_getsockopt,
	socket_listen,
	socket_recv,
	socket_recvfrom,
	socket_recvmsg,
	socket_send,
	socket_sendto,
	socket_sendmsg,
	socket_setsockopt,
	socket_shutdown,
};

