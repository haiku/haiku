/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "stack_private.h"

#include <net_protocol.h>
#include <net_stack.h>

#include <KernelExport.h>
#include <util/list.h>
#include <fs/select_sync_pool.h>

#include <new>
#include <stdlib.h>
#include <string.h>


status_t
create_socket(int family, int type, int protocol, net_socket **_socket)
{
	struct net_socket *socket = new (std::nothrow) net_socket;
	if (socket == NULL)
		return B_NO_MEMORY;

	memset(socket, 0, sizeof(net_socket));
	socket->family = family;
	socket->type = type;
	socket->protocol = protocol;

	status_t status = benaphore_init(&socket->lock, "socket");
	if (status < B_OK)
		goto err1;

	// set defaults (may be overridden by the protocols)
	socket->send.buffer_size = 65536;
	socket->send.low_water_mark = 1;
	socket->send.timeout = B_INFINITE_TIMEOUT;
	socket->receive.buffer_size = 65536;
	socket->receive.low_water_mark = 1;
	socket->receive.timeout = B_INFINITE_TIMEOUT;

	socket->select_pool = NULL;

	status = get_domain_protocols(socket);
	if (status < B_OK)
		goto err2;

	status = socket->first_info->open(socket->first_protocol);
	if (status < B_OK)
		goto err3;

	*_socket = socket;
	return B_OK;

err3:
	put_domain_protocols(socket);
err2:
	benaphore_destroy(&socket->lock);
err1:
	delete socket;
	return status;
}


status_t
socket_close(net_socket *socket)
{
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

	put_domain_protocols(socket);
	benaphore_destroy(&socket->lock);
	delete_select_sync_pool(socket->select_pool);
	delete socket;

	return status;
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
		// TODO: bind?!
		return ENETUNREACH;
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

	//buffer->source = (sockaddr *)&socket->address;
	//buffer->destination = (sockaddr *)&socket->peer;
	memcpy(&buffer->source, &socket->address, socket->address.ss_len);
	memcpy(&buffer->destination, &socket->peer, socket->peer.ss_len);

	ssize_t bytesWritten = socket->first_info->send_data(socket->first_protocol,
		buffer);
	if (bytesWritten < B_OK) {
		*_length = 0;
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


//	#pragma mark - notifications


status_t
socket_request_notification(net_socket *socket, uint8 event, uint32 ref,
	selectsync *sync)
{
	benaphore_lock(&socket->lock);

	status_t status = add_select_sync_pool_entry(&socket->select_pool, sync,
		ref, event);
	if (status < B_OK) {
		benaphore_unlock(&socket->lock);
		return status;
	}

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

	benaphore_unlock(&socket->lock);
	return B_OK;
}


status_t
socket_cancel_notification(net_socket *socket, uint8 event, selectsync *sync)
{
	benaphore_lock(&socket->lock);

	status_t status = remove_select_sync_pool_entry(&socket->select_pool,
		sync, event);

	benaphore_unlock(&socket->lock);
	return status;
}


status_t
socket_notify(net_socket *socket, uint8 event, int32 value)
{
	benaphore_lock(&socket->lock);

	bool notify = true;

	switch (event) {
		case B_SELECT_READ:
		{
			if ((ssize_t)socket->receive.low_water_mark > value && value >= B_OK)
				notify = false;
			break;
		}
		case B_SELECT_WRITE:
		{
			if ((ssize_t)socket->send.low_water_mark > value && value >= B_OK)
				notify = false;
			break;
		}
		case B_SELECT_ERROR:
			socket->error = value;
			break;
	}

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
	net_socket *accepted;
	status_t status = socket->first_info->accept(socket->first_protocol,
		&accepted);
	if (status < B_OK)
		return status;

	if (address && *_addressLength > 0) {
		memcpy(address, &accepted->peer, min_c(*_addressLength, accepted->peer.ss_len));
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
	//buffer->source = (sockaddr *)&socket->address;
	//buffer->destination = (sockaddr *)&socket->peer;
	memcpy(&buffer->source, &socket->address, socket->address.ss_len);
	memcpy(&buffer->destination, &socket->peer, socket->peer.ss_len);

	status_t status = socket->first_info->send_data(socket->first_protocol, buffer);
	if (status < B_OK) {
		gNetBufferModule.free(buffer);
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

	status_t status = socket->first_info->send_data(socket->first_protocol, buffer);
	if (status < B_OK) {
		gNetBufferModule.free(buffer);
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
			if (length != sizeof(uint32))
				return B_BAD_VALUE;

			socket->send.buffer_size = *(const uint32 *)value;
			return B_OK;

		case SO_RCVBUF:
			if (length != sizeof(uint32))
				return B_BAD_VALUE;

			socket->receive.buffer_size = *(const uint32 *)value;
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
			// initialize the main stack if not done so already
			module_info *module;
			return get_module(NET_STARTER_MODULE_NAME, &module);
		}
		case B_MODULE_UNINIT:
			return put_module(NET_STARTER_MODULE_NAME);

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
	create_socket,
	socket_close,
	socket_free,

	socket_readv,
	socket_writev,
	socket_control,

	socket_read_avail,
	socket_send_avail,

	socket_send_data,
	socket_receive_data,

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
	socket_send,
	socket_sendto,
	socket_setsockopt,
	socket_shutdown,
};

