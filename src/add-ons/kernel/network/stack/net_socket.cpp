/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "stack_private.h"

#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <new>

#include <Drivers.h>
#include <KernelExport.h>
#include <Select.h>

#include <AutoDeleter.h>
#include <team.h>
#include <util/AutoLock.h>
#include <util/list.h>
#include <WeakReferenceable.h>

#include <fs/select_sync_pool.h>
#include <kernel.h>

#include <net_protocol.h>
#include <net_stack.h>
#include <net_stat.h>

#include "ancillary_data.h"
#include "utility.h"


//#define TRACE_SOCKET
#ifdef TRACE_SOCKET
#	define TRACE(x...) dprintf(STACK_DEBUG_PREFIX x)
#else
#	define TRACE(x...) ;
#endif


struct net_socket_private;
typedef DoublyLinkedList<net_socket_private> SocketList;

struct net_socket_private : net_socket,
		DoublyLinkedListLinkImpl<net_socket_private>,
		BWeakReferenceable {
	net_socket_private();
	~net_socket_private();

	void RemoveFromParent();

	BWeakReference<net_socket_private> parent;
	team_id						owner;
	uint32						max_backlog;
	uint32						child_count;
	SocketList					pending_children;
	SocketList					connected_children;

	struct select_sync_pool*	select_pool;
	mutex						lock;

	bool						is_connected;
	bool						is_in_socket_list;
};


int socket_bind(net_socket* socket, const struct sockaddr* address,
	socklen_t addressLength);
int socket_setsockopt(net_socket* socket, int level, int option,
	const void* value, int length);
ssize_t socket_read_avail(net_socket* socket);

static SocketList sSocketList;
static mutex sSocketLock;


net_socket_private::net_socket_private()
	:
	owner(-1),
	max_backlog(0),
	child_count(0),
	select_pool(NULL),
	is_connected(false),
	is_in_socket_list(false)
{
	first_protocol = NULL;
	first_info = NULL;
	options = 0;
	linger = 0;
	bound_to_device = 0;
	error = 0;

	address.ss_len = 0;
	peer.ss_len = 0;

	mutex_init(&lock, "socket");

	// set defaults (may be overridden by the protocols)
	send.buffer_size = 65535;
	send.low_water_mark = 1;
	send.timeout = B_INFINITE_TIMEOUT;
	receive.buffer_size = 65535;
	receive.low_water_mark = 1;
	receive.timeout = B_INFINITE_TIMEOUT;
}


net_socket_private::~net_socket_private()
{
	TRACE("delete net_socket %p\n", this);

	if (parent != NULL)
		panic("socket still has a parent!");

	if (is_in_socket_list) {
		MutexLocker _(sSocketLock);
		sSocketList.Remove(this);
	}

	mutex_lock(&lock);

	// also delete all children of this socket
	while (net_socket_private* child = pending_children.RemoveHead()) {
		child->RemoveFromParent();
	}
	while (net_socket_private* child = connected_children.RemoveHead()) {
		child->RemoveFromParent();
	}

	mutex_unlock(&lock);

	put_domain_protocols(this);

	mutex_destroy(&lock);
}


void
net_socket_private::RemoveFromParent()
{
	ASSERT(!is_in_socket_list && parent != NULL);

	parent = NULL;

	mutex_lock(&sSocketLock);
	sSocketList.Add(this);
	mutex_unlock(&sSocketLock);

	is_in_socket_list = true;

	ReleaseReference();
}


//	#pragma mark -


static size_t
compute_user_iovec_length(iovec* userVec, uint32 count)
{
	size_t length = 0;

	for (uint32 i = 0; i < count; i++) {
		iovec vec;
		if (user_memcpy(&vec, userVec + i, sizeof(iovec)) < B_OK)
			return 0;

		length += vec.iov_len;
	}

	return length;
}


static status_t
create_socket(int family, int type, int protocol, net_socket_private** _socket)
{
	struct net_socket_private* socket = new(std::nothrow) net_socket_private;
	if (socket == NULL)
		return B_NO_MEMORY;
	status_t status = socket->InitCheck();
	if (status != B_OK) {
		delete socket;
		return status;
	}

	socket->family = family;
	socket->type = type;
	socket->protocol = protocol;

	status = get_domain_protocols(socket);
	if (status != B_OK) {
		delete socket;
		return status;
	}

	TRACE("create net_socket %p (%u.%u.%u):\n", socket, socket->family,
		socket->type, socket->protocol);

#ifdef TRACE_SOCKET
	net_protocol* current = socket->first_protocol;
	for (int i = 0; current != NULL; current = current->next, i++)
		TRACE("  [%d] %p  %s\n", i, current, current->module->info.name);
#endif

	*_socket = socket;
	return B_OK;
}


static status_t
add_ancillary_data(net_socket* socket, ancillary_data_container* container,
	void* data, size_t dataLen)
{
	cmsghdr* header = (cmsghdr*)data;

	if (dataLen == 0)
		return B_OK;

	if (socket->first_info->add_ancillary_data == NULL)
		return B_NOT_SUPPORTED;

	while (true) {
		if (header->cmsg_len < CMSG_LEN(0) || header->cmsg_len > dataLen)
			return B_BAD_VALUE;

		status_t status = socket->first_info->add_ancillary_data(
			socket->first_protocol, container, header);
		if (status != B_OK)
			return status;

		if (dataLen <= _ALIGN(header->cmsg_len))
			break;
		dataLen -= _ALIGN(header->cmsg_len);
		header = (cmsghdr*)((uint8*)header + _ALIGN(header->cmsg_len));
	}

	return B_OK;
}


static status_t
process_ancillary_data(net_socket* socket, ancillary_data_container* container,
	msghdr* messageHeader)
{
	uint8* dataBuffer = (uint8*)messageHeader->msg_control;
	int dataBufferLen = messageHeader->msg_controllen;

	if (container == NULL || dataBuffer == NULL) {
		messageHeader->msg_controllen = 0;
		return B_OK;
	}

	ancillary_data_header header;
	void* data = NULL;

	while ((data = next_ancillary_data(container, data, &header)) != NULL) {
		if (socket->first_info->process_ancillary_data == NULL)
			return B_NOT_SUPPORTED;

		ssize_t bytesWritten = socket->first_info->process_ancillary_data(
			socket->first_protocol, &header, data, dataBuffer, dataBufferLen);
		if (bytesWritten < 0)
			return bytesWritten;

		dataBuffer += bytesWritten;
		dataBufferLen -= bytesWritten;
	}

	messageHeader->msg_controllen -= dataBufferLen;

	return B_OK;
}


static status_t
process_ancillary_data(net_socket* socket,
	net_buffer* buffer, msghdr* messageHeader)
{
	void *dataBuffer = messageHeader->msg_control;
	ssize_t bytesWritten;

	if (dataBuffer == NULL) {
		messageHeader->msg_controllen = 0;
		return B_OK;
	}

	if (socket->first_info->process_ancillary_data_no_container == NULL)
		return B_NOT_SUPPORTED;

	bytesWritten = socket->first_info->process_ancillary_data_no_container(
		socket->first_protocol, buffer, dataBuffer,
		messageHeader->msg_controllen);
	if (bytesWritten < 0)
		return bytesWritten;
	messageHeader->msg_controllen = bytesWritten;

	return B_OK;
}


static ssize_t
socket_receive_no_buffer(net_socket* socket, msghdr* header, void* data,
	size_t length, int flags)
{
	iovec stackVec = { data, length };
	iovec* vecs = header ? header->msg_iov : &stackVec;
	int vecCount = header ? header->msg_iovlen : 1;
	sockaddr* address = header ? (sockaddr*)header->msg_name : NULL;
	socklen_t* addressLen = header ? &header->msg_namelen : NULL;

	ancillary_data_container* ancillaryData = NULL;
	ssize_t bytesRead = socket->first_info->read_data_no_buffer(
		socket->first_protocol, vecs, vecCount, &ancillaryData, address,
		addressLen);
	if (bytesRead < 0)
		return bytesRead;

	CObjectDeleter<ancillary_data_container> ancillaryDataDeleter(ancillaryData,
		&delete_ancillary_data_container);

	// process ancillary data
	if (header != NULL) {
		status_t status = process_ancillary_data(socket, ancillaryData, header);
		if (status != B_OK)
			return status;

		header->msg_flags = 0;
	}

	return bytesRead;
}


#if ENABLE_DEBUGGER_COMMANDS


static void
print_socket_line(net_socket_private* socket, const char* prefix)
{
	BReference<net_socket_private> parent;
	if (socket->parent.PrivatePointer() != NULL)
		parent = socket->parent.GetReference();
	kprintf("%s%p %2d.%2d.%2d %6" B_PRId32 " %p %p  %p%s\n", prefix, socket,
		socket->family, socket->type, socket->protocol, socket->owner,
		socket->first_protocol, socket->first_info, parent.Get(),
		parent.Get() != NULL ? socket->is_connected ? " (c)" : " (p)" : "");
}


static int
dump_socket(int argc, char** argv)
{
	if (argc < 2) {
		kprintf("usage: %s [address]\n", argv[0]);
		return 0;
	}

	net_socket_private* socket = (net_socket_private*)parse_expression(argv[1]);

	kprintf("SOCKET %p\n", socket);
	kprintf("  family.type.protocol: %d.%d.%d\n",
		socket->family, socket->type, socket->protocol);
	BReference<net_socket_private> parent;
	if (socket->parent.PrivatePointer() != NULL)
		parent = socket->parent.GetReference();
	kprintf("  parent:               %p\n", parent.Get());
	kprintf("  first protocol:       %p\n", socket->first_protocol);
	kprintf("  first module_info:    %p\n", socket->first_info);
	kprintf("  options:              %x\n", socket->options);
	kprintf("  linger:               %d\n", socket->linger);
	kprintf("  bound to device:      %" B_PRIu32 "\n", socket->bound_to_device);
	kprintf("  owner:                %" B_PRId32 "\n", socket->owner);
	kprintf("  max backlog:          %" B_PRId32 "\n", socket->max_backlog);
	kprintf("  is connected:         %d\n", socket->is_connected);
	kprintf("  child_count:          %" B_PRIu32 "\n", socket->child_count);

	if (socket->child_count == 0)
		return 0;

	kprintf("    pending children:\n");
	SocketList::Iterator iterator = socket->pending_children.GetIterator();
	while (net_socket_private* child = iterator.Next()) {
		print_socket_line(child, "      ");
	}

	kprintf("    connected children:\n");
	iterator = socket->connected_children.GetIterator();
	while (net_socket_private* child = iterator.Next()) {
		print_socket_line(child, "      ");
	}

	return 0;
}


static int
dump_sockets(int argc, char** argv)
{
	kprintf("address        kind  owner protocol   module_info parent\n");

	SocketList::Iterator iterator = sSocketList.GetIterator();
	while (net_socket_private* socket = iterator.Next()) {
		print_socket_line(socket, "");

		SocketList::Iterator childIterator
			= socket->pending_children.GetIterator();
		while (net_socket_private* child = childIterator.Next()) {
			print_socket_line(child, " ");
		}

		childIterator = socket->connected_children.GetIterator();
		while (net_socket_private* child = childIterator.Next()) {
			print_socket_line(child, " ");
		}
	}

	return 0;
}


#endif	// ENABLE_DEBUGGER_COMMANDS


//	#pragma mark -


status_t
socket_open(int family, int type, int protocol, net_socket** _socket)
{
	net_socket_private* socket;
	status_t status = create_socket(family, type, protocol, &socket);
	if (status != B_OK)
		return status;

	status = socket->first_info->open(socket->first_protocol);
	if (status != B_OK) {
		delete socket;
		return status;
	}

	socket->owner = team_get_current_team_id();
	socket->is_in_socket_list = true;

	mutex_lock(&sSocketLock);
	sSocketList.Add(socket);
	mutex_unlock(&sSocketLock);

	*_socket = socket;
	return B_OK;
}


status_t
socket_close(net_socket* _socket)
{
	net_socket_private* socket = (net_socket_private*)_socket;
	return socket->first_info->close(socket->first_protocol);
}


void
socket_free(net_socket* _socket)
{
	net_socket_private* socket = (net_socket_private*)_socket;
	socket->first_info->free(socket->first_protocol);
	socket->ReleaseReference();
}


status_t
socket_readv(net_socket* socket, const iovec* vecs, size_t vecCount,
	size_t* _length)
{
	return -1;
}


status_t
socket_writev(net_socket* socket, const iovec* vecs, size_t vecCount,
	size_t* _length)
{
	if (socket->peer.ss_len == 0)
		return ECONNRESET;

	if (socket->address.ss_len == 0) {
		// try to bind first
		status_t status = socket_bind(socket, NULL, 0);
		if (status != B_OK)
			return status;
	}

	// TODO: useful, maybe even computed header space!
	net_buffer* buffer = gNetBufferModule.create(256);
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

	memcpy(buffer->source, &socket->address, socket->address.ss_len);
	memcpy(buffer->destination, &socket->peer, socket->peer.ss_len);
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
socket_control(net_socket* socket, uint32 op, void* data, size_t length)
{
	switch (op) {
		case FIONBIO:
		{
			if (data == NULL)
				return B_BAD_VALUE;

			int value;
			if (is_syscall()) {
				if (!IS_USER_ADDRESS(data)
					|| user_memcpy(&value, data, sizeof(int)) != B_OK) {
					return B_BAD_ADDRESS;
				}
			} else
				value = *(int*)data;

			return socket_setsockopt(socket, SOL_SOCKET, SO_NONBLOCK, &value,
				sizeof(int));
		}

		case FIONREAD:
		{
			if (data == NULL)
				return B_BAD_VALUE;

			int available = (int)socket_read_avail(socket);
			if (available < 0)
				return available;

			if (is_syscall()) {
				if (!IS_USER_ADDRESS(data)
					|| user_memcpy(data, &available, sizeof(available))
						!= B_OK) {
					return B_BAD_ADDRESS;
				}
			} else
				*(int*)data = available;

			return B_OK;
		}

		case B_SET_BLOCKING_IO:
		case B_SET_NONBLOCKING_IO:
		{
			int value = op == B_SET_NONBLOCKING_IO;
			return socket_setsockopt(socket, SOL_SOCKET, SO_NONBLOCK, &value,
				sizeof(int));
		}
	}

	return socket->first_info->control(socket->first_protocol,
		LEVEL_DRIVER_IOCTL, op, data, &length);
}


ssize_t
socket_read_avail(net_socket* socket)
{
	return socket->first_info->read_avail(socket->first_protocol);
}


ssize_t
socket_send_avail(net_socket* socket)
{
	return socket->first_info->send_avail(socket->first_protocol);
}


status_t
socket_send_data(net_socket* socket, net_buffer* buffer)
{
	return socket->first_info->send_data(socket->first_protocol,
		buffer);
}


status_t
socket_receive_data(net_socket* socket, size_t length, uint32 flags,
	net_buffer** _buffer)
{
	status_t status = socket->first_info->read_data(socket->first_protocol,
		length, flags, _buffer);
	if (status != B_OK)
		return status;

	if (*_buffer && length < (*_buffer)->size) {
		// discard any data behind the amount requested
		gNetBufferModule.trim(*_buffer, length);
	}

	return status;
}


status_t
socket_get_next_stat(uint32* _cookie, int family, struct net_stat* stat)
{
	MutexLocker locker(sSocketLock);

	net_socket_private* socket = NULL;
	SocketList::Iterator iterator = sSocketList.GetIterator();
	uint32 cookie = *_cookie;
	uint32 count = 0;

	while (true) {
		socket = iterator.Next();
		if (socket == NULL)
			return B_ENTRY_NOT_FOUND;

		// TODO: also traverse the pending connections
		if (count == cookie)
			break;

		if (family == -1 || family == socket->family)
			count++;
	}

	*_cookie = count + 1;

	stat->family = socket->family;
	stat->type = socket->type;
	stat->protocol = socket->protocol;
	stat->owner = socket->owner;
	stat->state[0] = '\0';
	memcpy(&stat->address, &socket->address, sizeof(struct sockaddr_storage));
	memcpy(&stat->peer, &socket->peer, sizeof(struct sockaddr_storage));
	stat->receive_queue_size = 0;
	stat->send_queue_size = 0;

	// fill in protocol specific data (if supported by the protocol)
	size_t length = sizeof(net_stat);
	socket->first_info->control(socket->first_protocol, socket->protocol,
		NET_STAT_SOCKET, stat, &length);

	return B_OK;
}


//	#pragma mark - connections


bool
socket_acquire(net_socket* _socket)
{
	net_socket_private* socket = (net_socket_private*)_socket;

	// During destruction, the socket might still be accessible over its
	// endpoint protocol. We need to make sure the endpoint cannot acquire the
	// socket anymore -- while not obvious, the endpoint protocol is responsible
	// for the proper locking here.
	if (socket->CountReferences() == 0)
		return false;

	socket->AcquireReference();
	return true;
}


bool
socket_release(net_socket* _socket)
{
	net_socket_private* socket = (net_socket_private*)_socket;
	return socket->ReleaseReference();
}


status_t
socket_spawn_pending(net_socket* _parent, net_socket** _socket)
{
	net_socket_private* parent = (net_socket_private*)_parent;

	TRACE("%s(%p)\n", __FUNCTION__, parent);

	MutexLocker locker(parent->lock);

	// We actually accept more pending connections to compensate for those
	// that never complete, and also make sure at least a single connection
	// can always be accepted
	if (parent->child_count > 3 * parent->max_backlog / 2)
		return ENOBUFS;

	net_socket_private* socket;
	status_t status = create_socket(parent->family, parent->type,
		parent->protocol, &socket);
	if (status != B_OK)
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
	parent->pending_children.Add(socket);
	socket->parent = parent;
	parent->child_count++;

	*_socket = socket;
	return B_OK;
}


/*!	Dequeues a connected child from a parent socket.
	It also returns a reference with the child socket.
*/
status_t
socket_dequeue_connected(net_socket* _parent, net_socket** _socket)
{
	net_socket_private* parent = (net_socket_private*)_parent;

	mutex_lock(&parent->lock);

	net_socket_private* socket = parent->connected_children.RemoveHead();
	if (socket != NULL) {
		socket->AcquireReference();
		socket->RemoveFromParent();
		parent->child_count--;
		*_socket = socket;
	}

	mutex_unlock(&parent->lock);

	if (socket == NULL)
		return B_ENTRY_NOT_FOUND;

	return B_OK;
}


ssize_t
socket_count_connected(net_socket* _parent)
{
	net_socket_private* parent = (net_socket_private*)_parent;

	MutexLocker _(parent->lock);
	return parent->connected_children.Count();
}


status_t
socket_set_max_backlog(net_socket* _socket, uint32 backlog)
{
	net_socket_private* socket = (net_socket_private*)_socket;

	// we enforce an upper limit of connections waiting to be accepted
	if (backlog > 256)
		backlog = 256;

	MutexLocker _(socket->lock);

	// first remove the pending connections, then the already connected
	// ones as needed
	net_socket_private* child;
	while (socket->child_count > backlog
		&& (child = socket->pending_children.RemoveTail()) != NULL) {
		child->RemoveFromParent();
		socket->child_count--;
	}
	while (socket->child_count > backlog
		&& (child = socket->connected_children.RemoveTail()) != NULL) {
		child->RemoveFromParent();
		socket->child_count--;
	}

	socket->max_backlog = backlog;
	return B_OK;
}


/*!	Returns whether or not this socket has a parent. The parent might not be
	valid anymore, though.
*/
bool
socket_has_parent(net_socket* _socket)
{
	net_socket_private* socket = (net_socket_private*)_socket;
	return socket->parent != NULL;
}


/*!	The socket has been connected. It will be moved to the connected queue
	of its parent socket.
*/
status_t
socket_connected(net_socket* _socket)
{
	net_socket_private* socket = (net_socket_private*)_socket;

	TRACE("socket_connected(%p)\n", socket);

	BReference<net_socket_private> parent = socket->parent.GetReference();
	if (parent.Get() == NULL)
		return B_BAD_VALUE;

	MutexLocker _(parent->lock);

	parent->pending_children.Remove(socket);
	parent->connected_children.Add(socket);
	socket->is_connected = true;

	// notify parent
	if (parent->select_pool)
		notify_select_event_pool(parent->select_pool, B_SELECT_READ);

	return B_OK;
}


/*!	The socket has been aborted. Steals the parent's reference, and releases
	it.
*/
status_t
socket_aborted(net_socket* _socket)
{
	net_socket_private* socket = (net_socket_private*)_socket;

	TRACE("socket_aborted(%p)\n", socket);

	BReference<net_socket_private> parent = socket->parent.GetReference();
	if (parent.Get() == NULL)
		return B_BAD_VALUE;

	MutexLocker _(parent->lock);

	if (socket->is_connected)
		parent->connected_children.Remove(socket);
	else
		parent->pending_children.Remove(socket);

	parent->child_count--;
	socket->RemoveFromParent();

	return B_OK;
}


//	#pragma mark - notifications


status_t
socket_request_notification(net_socket* _socket, uint8 event, selectsync* sync)
{
	net_socket_private* socket = (net_socket_private*)_socket;

	mutex_lock(&socket->lock);

	status_t status = add_select_sync_pool_entry(&socket->select_pool, sync,
		event);

	mutex_unlock(&socket->lock);

	if (status != B_OK)
		return status;

	// check if the event is already present
	// TODO: add support for poll() types

	switch (event) {
		case B_SELECT_READ:
		{
			ssize_t available = socket_read_avail(socket);
			if ((ssize_t)socket->receive.low_water_mark <= available
				|| available < B_OK)
				notify_select_event(sync, event);
			break;
		}
		case B_SELECT_WRITE:
		{
			ssize_t available = socket_send_avail(socket);
			if ((ssize_t)socket->send.low_water_mark <= available
				|| available < B_OK)
				notify_select_event(sync, event);
			break;
		}
		case B_SELECT_ERROR:
			if (socket->error != B_OK)
				notify_select_event(sync, event);
			break;
	}

	return B_OK;
}


status_t
socket_cancel_notification(net_socket* _socket, uint8 event, selectsync* sync)
{
	net_socket_private* socket = (net_socket_private*)_socket;

	MutexLocker _(socket->lock);
	return remove_select_sync_pool_entry(&socket->select_pool, sync, event);
}


status_t
socket_notify(net_socket* _socket, uint8 event, int32 value)
{
	net_socket_private* socket = (net_socket_private*)_socket;
	bool notify = true;

	switch (event) {
		case B_SELECT_READ:
			if ((ssize_t)socket->receive.low_water_mark > value
				&& value >= B_OK)
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

	MutexLocker _(socket->lock);

	if (notify && socket->select_pool != NULL) {
		notify_select_event_pool(socket->select_pool, event);

		if (event == B_SELECT_ERROR) {
			// always notify read/write on error
			notify_select_event_pool(socket->select_pool, B_SELECT_READ);
			notify_select_event_pool(socket->select_pool, B_SELECT_WRITE);
		}
	}

	return B_OK;
}


//	#pragma mark - standard socket API


int
socket_accept(net_socket* socket, struct sockaddr* address,
	socklen_t* _addressLength, net_socket** _acceptedSocket)
{
	if ((socket->options & SO_ACCEPTCONN) == 0)
		return B_BAD_VALUE;

	net_socket* accepted;
	status_t status = socket->first_info->accept(socket->first_protocol,
		&accepted);
	if (status != B_OK)
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
socket_bind(net_socket* socket, const struct sockaddr* address,
	socklen_t addressLength)
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
			(sockaddr*)&socket->address);
		if (status != B_OK)
			return status;
	}

	memcpy(&socket->address, address, sizeof(sockaddr));
	socket->address.ss_len = sizeof(sockaddr_storage);

	status_t status = socket->first_info->bind(socket->first_protocol,
		(sockaddr*)address);
	if (status != B_OK) {
		// clear address again, as binding failed
		socket->address.ss_len = 0;
	}

	return status;
}


int
socket_connect(net_socket* socket, const struct sockaddr* address,
	socklen_t addressLength)
{
	if (address == NULL || addressLength == 0)
		return ENETUNREACH;

	if (socket->address.ss_len == 0) {
		// try to bind first
		status_t status = socket_bind(socket, NULL, 0);
		if (status != B_OK)
			return status;
	}

	return socket->first_info->connect(socket->first_protocol, address);
}


int
socket_getpeername(net_socket* socket, struct sockaddr* address,
	socklen_t* _addressLength)
{
	if (socket->peer.ss_len == 0)
		return ENOTCONN;

	memcpy(address, &socket->peer, min_c(*_addressLength, socket->peer.ss_len));
	*_addressLength = socket->peer.ss_len;
	return B_OK;
}


int
socket_getsockname(net_socket* socket, struct sockaddr* address,
	socklen_t* _addressLength)
{
	if (socket->address.ss_len == 0) {
		struct sockaddr buffer;
		memset(&buffer, 0, sizeof(buffer));
		buffer.sa_family = socket->family;

		memcpy(address, &buffer, min_c(*_addressLength, sizeof(buffer)));
		*_addressLength = sizeof(buffer);
		return B_OK;
	}

	memcpy(address, &socket->address, min_c(*_addressLength,
		socket->address.ss_len));
	*_addressLength = socket->address.ss_len;
	return B_OK;
}


status_t
socket_get_option(net_socket* socket, int level, int option, void* value,
	int* _length)
{
	if (level != SOL_SOCKET)
		return ENOPROTOOPT;

	switch (option) {
		case SO_SNDBUF:
		{
			uint32* size = (uint32*)value;
			*size = socket->send.buffer_size;
			*_length = sizeof(uint32);
			return B_OK;
		}

		case SO_RCVBUF:
		{
			uint32* size = (uint32*)value;
			*size = socket->receive.buffer_size;
			*_length = sizeof(uint32);
			return B_OK;
		}

		case SO_SNDLOWAT:
		{
			uint32* size = (uint32*)value;
			*size = socket->send.low_water_mark;
			*_length = sizeof(uint32);
			return B_OK;
		}

		case SO_RCVLOWAT:
		{
			uint32* size = (uint32*)value;
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

			struct timeval* timeval = (struct timeval*)value;
			timeval->tv_sec = timeout / 1000000LL;
			timeval->tv_usec = timeout % 1000000LL;

			*_length = sizeof(struct timeval);
			return B_OK;
		}

		case SO_NONBLOCK:
		{
			int32* _set = (int32*)value;
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
			int32* _set = (int32*)value;
			*_set = (socket->options & option) != 0;
			*_length = sizeof(int32);
			return B_OK;
		}

		case SO_TYPE:
		{
			int32* _set = (int32*)value;
			*_set = socket->type;
			*_length = sizeof(int32);
			return B_OK;
		}

		case SO_ERROR:
		{
			int32* _set = (int32*)value;
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
socket_getsockopt(net_socket* socket, int level, int option, void* value,
	int* _length)
{
	return socket->first_protocol->module->getsockopt(socket->first_protocol,
		level, option, value, _length);
}


int
socket_listen(net_socket* socket, int backlog)
{
	status_t status = socket->first_info->listen(socket->first_protocol,
		backlog);
	if (status == B_OK)
		socket->options |= SO_ACCEPTCONN;

	return status;
}


ssize_t
socket_receive(net_socket* socket, msghdr* header, void* data, size_t length,
	int flags)
{
	// If the protocol sports read_data_no_buffer() we use it.
	if (socket->first_info->read_data_no_buffer != NULL)
		return socket_receive_no_buffer(socket, header, data, length, flags);

	size_t totalLength = length;
	net_buffer* buffer;
	int i;

	// the convention to this function is that have header been
	// present, { data, length } would have been iovec[0] and is
	// always considered like that

	if (header) {
		// calculate the length considering all of the extra buffers
		for (i = 1; i < header->msg_iovlen; i++)
			totalLength += header->msg_iov[i].iov_len;
	}

	status_t status = socket->first_info->read_data(
		socket->first_protocol, totalLength, flags, &buffer);
	if (status != B_OK)
		return status;

	// process ancillary data
	if (header != NULL) {
		if (buffer != NULL && header->msg_control != NULL) {
			ancillary_data_container* container
				= gNetBufferModule.get_ancillary_data(buffer);
			if (container != NULL)
				status = process_ancillary_data(socket, container, header);
			else
				status = process_ancillary_data(socket, buffer, header);
			if (status != B_OK) {
				gNetBufferModule.free(buffer);
				return status;
			}
		} else
			header->msg_controllen = 0;
	}

	// TODO: - returning a NULL buffer when received 0 bytes
	//         may not make much sense as we still need the address
	//       - gNetBufferModule.read() uses memcpy() instead of user_memcpy

	size_t nameLen = 0;

	if (header) {
		// TODO: - consider the control buffer options
		nameLen = header->msg_namelen;
		header->msg_namelen = 0;
		header->msg_flags = 0;
	}

	if (buffer == NULL)
		return 0;

	size_t bytesReceived = buffer->size, bytesCopied = 0;

	length = min_c(bytesReceived, length);
	if (gNetBufferModule.read(buffer, 0, data, length) < B_OK) {
		gNetBufferModule.free(buffer);
		return ENOBUFS;
	}

	// if first copy was a success, proceed to following
	// copies as required
	bytesCopied += length;

	if (header) {
		// we only start considering at iovec[1]
		// as { data, length } is iovec[0]
		for (i = 1; i < header->msg_iovlen && bytesCopied < bytesReceived; i++) {
			iovec& vec = header->msg_iov[i];
			size_t toRead = min_c(bytesReceived - bytesCopied, vec.iov_len);
			if (gNetBufferModule.read(buffer, bytesCopied, vec.iov_base,
					toRead) < B_OK) {
				break;
			}

			bytesCopied += toRead;
		}

		if (header->msg_name != NULL) {
			header->msg_namelen = min_c(nameLen, buffer->source->sa_len);
			memcpy(header->msg_name, buffer->source, header->msg_namelen);
		}
	}

	gNetBufferModule.free(buffer);

	if (bytesCopied < bytesReceived) {
		if (header)
			header->msg_flags = MSG_TRUNC;

		if (flags & MSG_TRUNC)
			return bytesReceived;
	}

	return bytesCopied;
}


ssize_t
socket_send(net_socket* socket, msghdr* header, const void* data, size_t length,
	int flags)
{
	const sockaddr* address = NULL;
	socklen_t addressLength = 0;
	size_t bytesLeft = length;

	if (length > SSIZE_MAX)
		return B_BAD_VALUE;

	ancillary_data_container* ancillaryData = NULL;
	CObjectDeleter<ancillary_data_container> ancillaryDataDeleter(NULL,
		&delete_ancillary_data_container);

	if (header != NULL) {
		address = (const sockaddr*)header->msg_name;
		addressLength = header->msg_namelen;

		// get the ancillary data
		if (header->msg_control != NULL) {
			ancillaryData = create_ancillary_data_container();
			if (ancillaryData == NULL)
				return B_NO_MEMORY;
			ancillaryDataDeleter.SetTo(ancillaryData);

			status_t status = add_ancillary_data(socket, ancillaryData,
				(cmsghdr*)header->msg_control, header->msg_controllen);
			if (status != B_OK)
				return status;
		}
	}

	if (addressLength == 0)
		address = NULL;
	else if (address == NULL)
		return B_BAD_VALUE;

	if (socket->peer.ss_len != 0) {
		if (address != NULL)
			return EISCONN;

		// socket is connected, we use that address
		address = (struct sockaddr*)&socket->peer;
		addressLength = socket->peer.ss_len;
	}

	if (address == NULL || addressLength == 0) {
		// don't know where to send to:
		return EDESTADDRREQ;
	}

	if ((socket->first_info->flags & NET_PROTOCOL_ATOMIC_MESSAGES) != 0
		&& bytesLeft > socket->send.buffer_size)
		return EMSGSIZE;

	if (socket->address.ss_len == 0) {
		// try to bind first
		status_t status = socket_bind(socket, NULL, 0);
		if (status != B_OK)
			return status;
	}

	// If the protocol has a send_data_no_buffer() hook, we use that one.
	if (socket->first_info->send_data_no_buffer != NULL) {
		iovec stackVec = { (void*)data, length };
		iovec* vecs = header ? header->msg_iov : &stackVec;
		int vecCount = header ? header->msg_iovlen : 1;

		ssize_t written = socket->first_info->send_data_no_buffer(
			socket->first_protocol, vecs, vecCount, ancillaryData, address,
			addressLength);
		if (written > 0)
			ancillaryDataDeleter.Detach();
		return written;
	}

	// By convention, if a header is given, the (data, length) equals the first
	// iovec. So drop the header, if it is the only iovec. Otherwise compute
	// the size of the remaining ones.
	if (header != NULL) {
		if (header->msg_iovlen <= 1)
			header = NULL;
		else {
// TODO: The iovecs have already been copied to kernel space. Simplify!
			bytesLeft += compute_user_iovec_length(header->msg_iov + 1,
				header->msg_iovlen - 1);
		}
	}

	ssize_t bytesSent = 0;
	size_t vecOffset = 0;
	uint32 vecIndex = 0;

	while (bytesLeft > 0) {
		// TODO: useful, maybe even computed header space!
		net_buffer* buffer = gNetBufferModule.create(256);
		if (buffer == NULL)
			return ENOBUFS;

		while (buffer->size < socket->send.buffer_size
			&& buffer->size < bytesLeft) {
			if (vecIndex > 0 && vecOffset == 0) {
				// retrieve next iovec buffer from header
				iovec vec;
				if (user_memcpy(&vec, header->msg_iov + vecIndex, sizeof(iovec))
						< B_OK) {
					gNetBufferModule.free(buffer);
					return B_BAD_ADDRESS;
				}

				data = vec.iov_base;
				length = vec.iov_len;
			}

			size_t bytes = length;
			if (buffer->size + bytes > socket->send.buffer_size)
				bytes = socket->send.buffer_size - buffer->size;

			if (gNetBufferModule.append(buffer, data, bytes) < B_OK) {
				gNetBufferModule.free(buffer);
				return ENOBUFS;
			}

			if (bytes != length) {
				// partial send
				vecOffset = bytes;
				length -= vecOffset;
				data = (uint8*)data + vecOffset;
			} else if (header != NULL) {
				// proceed with next buffer, if any
				vecOffset = 0;
				vecIndex++;

				if (vecIndex >= (uint32)header->msg_iovlen)
					break;
			}
		}

		// attach ancillary data to the first buffer
		status_t status = B_OK;
		if (ancillaryData != NULL) {
			gNetBufferModule.set_ancillary_data(buffer, ancillaryData);
			ancillaryDataDeleter.Detach();
			ancillaryData = NULL;
		}

		size_t bufferSize = buffer->size;
		buffer->flags = flags;
		memcpy(buffer->source, &socket->address, socket->address.ss_len);
		memcpy(buffer->destination, address, addressLength);
		buffer->destination->sa_len = addressLength;

		if (status == B_OK) {
			status = socket->first_info->send_data(socket->first_protocol,
				buffer);
		}
		if (status != B_OK) {
			size_t sizeAfterSend = buffer->size;
			gNetBufferModule.free(buffer);

			if ((sizeAfterSend != bufferSize || bytesSent > 0)
				&& (status == B_INTERRUPTED || status == B_WOULD_BLOCK)) {
				// this appears to be a partial write
				return bytesSent + (bufferSize - sizeAfterSend);
			}
			return status;
		}

		bytesLeft -= bufferSize;
		bytesSent += bufferSize;
	}

	return bytesSent;
}


status_t
socket_set_option(net_socket* socket, int level, int option, const void* value,
	int length)
{
	if (level != SOL_SOCKET)
		return ENOPROTOOPT;

	TRACE("%s(socket %p, option %d\n", __FUNCTION__, socket, option);

	switch (option) {
		// TODO: implement other options!
		case SO_LINGER:
		{
			if (length < (int)sizeof(struct linger))
				return B_BAD_VALUE;

			struct linger* linger = (struct linger*)value;
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

			socket->send.buffer_size = *(const uint32*)value;
			return B_OK;

		case SO_RCVBUF:
			if (length != sizeof(uint32))
				return B_BAD_VALUE;

			socket->receive.buffer_size = *(const uint32*)value;
			return B_OK;

		case SO_SNDLOWAT:
			if (length != sizeof(uint32))
				return B_BAD_VALUE;

			socket->send.low_water_mark = *(const uint32*)value;
			return B_OK;

		case SO_RCVLOWAT:
			if (length != sizeof(uint32))
				return B_BAD_VALUE;

			socket->receive.low_water_mark = *(const uint32*)value;
			return B_OK;

		case SO_RCVTIMEO:
		case SO_SNDTIMEO:
		{
			if (length != sizeof(struct timeval))
				return B_BAD_VALUE;

			const struct timeval* timeval = (const struct timeval*)value;
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

			if (*(const int32*)value) {
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

			if (*(const int32*)value)
				socket->options |= option;
			else
				socket->options &= ~option;
			return B_OK;

		case SO_BINDTODEVICE:
		{
			if (length != sizeof(uint32))
				return B_BAD_VALUE;

			// TODO: we might want to check if the device exists at all
			// (although it doesn't really harm when we don't)
			socket->bound_to_device = *(const uint32*)value;
			return B_OK;
		}

		default:
			break;
	}

	dprintf("socket_setsockopt: unknown option %d\n", option);
	return ENOPROTOOPT;
}


int
socket_setsockopt(net_socket* socket, int level, int option, const void* value,
	int length)
{
	return socket->first_protocol->module->setsockopt(socket->first_protocol,
		level, option, value, length);
}


int
socket_shutdown(net_socket* socket, int direction)
{
	return socket->first_info->shutdown(socket->first_protocol, direction);
}


status_t
socket_socketpair(int family, int type, int protocol, net_socket* sockets[2])
{
	sockets[0] = NULL;
	sockets[1] = NULL;

	// create sockets
	status_t error = socket_open(family, type, protocol, &sockets[0]);
	if (error != B_OK)
		return error;

	if (error == B_OK)
		error = socket_open(family, type, protocol, &sockets[1]);

	// bind one
	if (error == B_OK)
		error = socket_bind(sockets[0], NULL, 0);

	// start listening
	if (error == B_OK)
		error = socket_listen(sockets[0], 1);

	// connect them
	if (error == B_OK) {
		error = socket_connect(sockets[1], (sockaddr*)&sockets[0]->address,
			sockets[0]->address.ss_len);
	}

	// accept a socket
	net_socket* acceptedSocket = NULL;
	if (error == B_OK)
		error = socket_accept(sockets[0], NULL, NULL, &acceptedSocket);

	if (error == B_OK) {
		// everything worked: close the listener socket
		socket_close(sockets[0]);
		socket_free(sockets[0]);
		sockets[0] = acceptedSocket;
	} else {
		// close sockets on error
		for (int i = 0; i < 2; i++) {
			if (sockets[i] != NULL) {
				socket_close(sockets[i]);
				socket_free(sockets[i]);
				sockets[i] = NULL;
			}
		}
	}

	return error;
}


//	#pragma mark -


static status_t
socket_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			new (&sSocketList) SocketList;
			mutex_init(&sSocketLock, "socket list");

#if ENABLE_DEBUGGER_COMMANDS
			add_debugger_command("sockets", dump_sockets, "lists all sockets");
			add_debugger_command("socket", dump_socket, "dumps a socket");
#endif
			return B_OK;
		}
		case B_MODULE_UNINIT:
			ASSERT(sSocketList.IsEmpty());
			mutex_destroy(&sSocketLock);

#if ENABLE_DEBUGGER_COMMANDS
			remove_debugger_command("socket", dump_socket);
			remove_debugger_command("sockets", dump_sockets);
#endif
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

	socket_get_option,
	socket_set_option,

	socket_get_next_stat,

	// connections
	socket_acquire,
	socket_release,
	socket_spawn_pending,
	socket_dequeue_connected,
	socket_count_connected,
	socket_set_max_backlog,
	socket_has_parent,
	socket_connected,
	socket_aborted,

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
	socket_receive,
	socket_send,
	socket_setsockopt,
	socket_shutdown,
	socket_socketpair
};

