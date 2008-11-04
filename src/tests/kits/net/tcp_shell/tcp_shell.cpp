/*
 * Copyright 2006-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#define _KERNEL_DEBUG_H
	// avoid including the private kernel debug.h header

#include "argv.h"
#include "tcp.h"
#include "utility.h"

#include <NetBufferUtilities.h>
#include <net_buffer.h>
#include <net_datalink.h>
#include <net_protocol.h>
#include <net_socket.h>
#include <net_stack.h>
#include <slab/Slab.h>
#include <util/AutoLock.h>

#include <KernelExport.h>
#include <Select.h>
#include <module.h>
#include <Locker.h>

#include <ctype.h>
#include <netinet/in.h>
#include <new>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct context {
	BLocker		lock;
	sem_id		wait_sem;
	struct list list;
	net_route	route;
	bool		server;
	thread_id	thread;
};

struct cmd_entry {
	char*	name;
	void	(*func)(int argc, char **argv);
	char*	help;
};

struct net_socket_private : net_socket {
	struct list_link		link;
	team_id					owner;
	uint32					max_backlog;
	uint32					child_count;
	struct list				pending_children;
	struct list				connected_children;

	struct select_sync_pool	*select_pool;
	mutex					lock;
};


extern "C" status_t _add_builtin_module(module_info *info);
extern "C" status_t _get_builtin_dependencies(void);
extern bool gDebugOutputEnabled;
	// from libkernelland_emu.so

extern struct net_buffer_module_info gNetBufferModule;
	// from net_buffer.cpp
extern net_address_module_info gIPv4AddressModule;
	// from ipv4_address.cpp
extern module_info *modules[];
	// from tcp.cpp


extern struct net_protocol_module_info gDomainModule;
struct net_interface gInterface;
extern struct net_socket_module_info gNetSocketModule;
struct net_protocol_module_info *gTCPModule;
struct net_socket *gServerSocket, *gClientSocket;
static struct context sClientContext, sServerContext;

static vint32 sPacketNumber = 1;
static double sRandomDrop = 0.0;
static set<uint32> sDropList;
static bigtime_t sRoundTripTime = 0;
static bool sIncreasingRoundTrip = false;
static bool sRandomRoundTrip = false;
static bool sTCPDump = true;
static bigtime_t sStartTime;
static double sRandomReorder = 0.0;
static set<uint32> sReorderList;
static bool sSimultaneousConnect = false;
static bool sSimultaneousClose = false;
static bool sServerActiveClose = false;

static struct net_domain sDomain = {
	"ipv4",
	AF_INET,
	{},
	&gDomainModule,
	&gIPv4AddressModule
};


static bool
is_server(const sockaddr* addr)
{
	return ((sockaddr_in*)addr)->sin_port == htons(1024);
}


static uint8
tcp_segment_flags(net_buffer* buffer)
{
	NetBufferHeaderReader<tcp_header> bufferHeader(buffer);
	if (bufferHeader.Status() < B_OK)
		return bufferHeader.Status();

	tcp_header &header = bufferHeader.Data();
	return header.flags;
}


static bool
is_syn(net_buffer* buffer)
{
	return (tcp_segment_flags(buffer) & TCP_FLAG_SYNCHRONIZE) != 0;
}


static bool
is_fin(net_buffer* buffer)
{
	return (tcp_segment_flags(buffer) & TCP_FLAG_FINISH) != 0;
}


//	#pragma mark - stack


status_t
std_ops(int32, ...)
{
	return B_OK;
}


net_domain *
get_domain(int family)
{
	return &sDomain;
}


status_t
register_domain_protocols(int family, int type, int protocol, ...)
{
	return B_OK;
}


status_t
register_domain_datalink_protocols(int family, int type, ...)
{
	return B_OK;
}


static status_t
register_domain_receiving_protocol(int family, int type, const char *moduleName)
{
	return B_OK;
}


static bool
dummy_is_syscall(void)
{
	return false;
}


static bool
dummy_is_restarted_syscall(void)
{
	return false;
}


static void
dummy_store_syscall_restart_timeout(bigtime_t timeout)
{
}


static net_stack_module_info gNetStackModule = {
	{
		NET_STACK_MODULE_NAME,
		0,
		std_ops
	},
	NULL, // register_domain,
	NULL, // unregister_domain,
	get_domain,

	register_domain_protocols,
	register_domain_datalink_protocols,
	register_domain_receiving_protocol,

	NULL, // get_domain_receiving_protocol,
	NULL, // put_domain_receiving_protocol,

	NULL, // register_device_deframer,
	NULL, // unregister_device_deframer,
	NULL, // register_domain_device_handler,
	NULL, // register_device_handler,
	NULL, // unregister_device_handler,
	NULL, // register_device_monitor,
	NULL, // unregister_device_monitor,
	NULL, // device_link_changed,
	NULL, // device_removed,
	NULL, // device_enqueue_buffer,

	notify_socket,

	checksum,

	init_fifo,
	uninit_fifo,
	fifo_enqueue_buffer,
	fifo_dequeue_buffer,
	clear_fifo,
	fifo_socket_enqueue_buffer,

	init_timer,
	set_timer,
	cancel_timer,
	wait_for_timer,
	is_timer_active,
	is_timer_running,

	dummy_is_syscall,
	dummy_is_restarted_syscall,
	dummy_store_syscall_restart_timeout,
	NULL, // restore_syscall_restart_timeout

	// ancillary data is not used by TCP
};


//	#pragma mark - socket


status_t
socket_create(int family, int type, int protocol, net_socket **_socket)
{
	net_protocol* domainProtocol;

	struct net_socket_private *socket = new (std::nothrow) net_socket_private;
	if (socket == NULL)
		return B_NO_MEMORY;

	memset(socket, 0, sizeof(net_socket));
	socket->family = family;
	socket->type = type;
	socket->protocol = protocol;

	mutex_init(&socket->lock, "socket");

	// set defaults (may be overridden by the protocols)
	socket->send.buffer_size = 65535;
	socket->send.low_water_mark = 1;
	socket->send.timeout = B_INFINITE_TIMEOUT;
	socket->receive.buffer_size = 65535;
	socket->receive.low_water_mark = 1;
	socket->receive.timeout = B_INFINITE_TIMEOUT;

	list_init_etc(&socket->pending_children, offsetof(net_socket_private, link));
	list_init_etc(&socket->connected_children, offsetof(net_socket_private, link));

	socket->first_protocol = gTCPModule->init_protocol(socket);
	if (socket->first_protocol == NULL) {
		fprintf(stderr, "tcp_tester: cannot create protocol\n");
		mutex_destroy(&socket->lock);
		delete socket;
		return B_ERROR;
	}

	socket->first_info = gTCPModule;

	domainProtocol = new net_protocol;
	domainProtocol->module = &gDomainModule;
	domainProtocol->socket = socket;

	socket->first_protocol->next = domainProtocol;
	socket->first_protocol->module = gTCPModule;
	socket->first_protocol->socket = socket;

	*_socket = socket;
	return B_OK;
}


void
socket_delete(net_socket *_socket)
{
	net_socket_private *socket = (net_socket_private *)_socket;

	if (socket->parent != NULL)
		panic("socket still has a parent!");

	socket->first_info->uninit_protocol(socket->first_protocol);
	mutex_destroy(&socket->lock);
	delete socket;
}


int
socket_accept(net_socket *socket, struct sockaddr *address,
	socklen_t *_addressLength, net_socket **_acceptedSocket)
{
	net_socket *accepted;
	status_t status = socket->first_info->accept(socket->first_protocol,
		&accepted);
	if (status < B_OK)
		return status;

	if (address && *_addressLength > 0) {
		memcpy(address, &accepted->peer, min_c(*_addressLength,
			accepted->peer.ss_len));
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
socket_listen(net_socket *socket, int backlog)
{
	status_t status = socket->first_info->listen(socket->first_protocol, backlog);
	if (status == B_OK)
		socket->options |= SO_ACCEPTCONN;

	return status;
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
	memcpy(buffer->source, &socket->address, socket->address.ss_len);
	memcpy(buffer->destination, &socket->peer, socket->peer.ss_len);

	status_t status = socket->first_info->send_data(socket->first_protocol, buffer);
	if (status < B_OK) {
		gNetBufferModule.free(buffer);
		return status;
	}

	return length;
}


ssize_t
socket_recv(net_socket *socket, void *data, size_t length, int flags)
{
	net_buffer *buffer;
	ssize_t status = socket->first_info->read_data(
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


status_t
socket_spawn_pending(net_socket *_parent, net_socket **_socket)
{
	net_socket_private *parent = (net_socket_private *)_parent;

	MutexLocker locker(parent->lock);

	// We actually accept more pending connections to compensate for those
	// that never complete, and also make sure at least a single connection
	// can always be accepted
	if (parent->child_count > 3 * parent->max_backlog / 2)
		return ENOBUFS;

	net_socket_private *socket;
	status_t status = socket_create(parent->family, parent->type, parent->protocol,
		(net_socket **)&socket);
	if (status < B_OK)
		return status;

	// inherit parent's properties
	socket->send = parent->send;
	socket->receive = parent->receive;
	socket->options = parent->options & ~SO_ACCEPTCONN;
	socket->linger = parent->linger;
	memcpy(&socket->address, &parent->address, parent->address.ss_len);
	memcpy(&socket->peer, &parent->peer, parent->peer.ss_len);

	// add to the parent's list of pending connections
	list_add_item(&parent->pending_children, socket);
	socket->parent = parent;
	parent->child_count++;

	*_socket = socket;
	return B_OK;
}


status_t
socket_dequeue_connected(net_socket *_parent, net_socket **_socket)
{
	net_socket_private *parent = (net_socket_private *)_parent;

	mutex_lock(&parent->lock);

	net_socket *socket = (net_socket *)list_remove_head_item(&parent->connected_children);
	if (socket != NULL) {
		socket->parent = NULL;
		parent->child_count--;
		*_socket = socket;
	}

	mutex_unlock(&parent->lock);
	return socket != NULL ? B_OK : B_ENTRY_NOT_FOUND;
}


ssize_t
socket_count_connected(net_socket *_parent)
{
	net_socket_private *parent = (net_socket_private *)_parent;

	MutexLocker _(parent->lock);

	ssize_t count = 0;
	void *item = NULL;
	while ((item = list_get_next_item(&parent->connected_children,
			item)) != NULL) {
		count++;
	}

	return count;
}


status_t
socket_set_max_backlog(net_socket *_socket, uint32 backlog)
{
	net_socket_private *socket = (net_socket_private *)_socket;

	// we enforce an upper limit of connections waiting to be accepted
	if (backlog > 256)
		backlog = 256;

	mutex_lock(&socket->lock);

	// first remove the pending connections, then the already connected ones as needed	
	net_socket *child;
	while (socket->child_count > backlog
		&& (child = (net_socket *)list_remove_tail_item(&socket->pending_children)) != NULL) {
		child->parent = NULL;
		socket->child_count--;
	}
	while (socket->child_count > backlog
		&& (child = (net_socket *)list_remove_tail_item(&socket->connected_children)) != NULL) {
		child->parent = NULL;
		socket_delete(child);
		socket->child_count--;
	}

	socket->max_backlog = backlog;
	mutex_unlock(&socket->lock);
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

	mutex_lock(&parent->lock);

	list_remove_item(&parent->pending_children, socket);
	list_add_item(&parent->connected_children, socket);

	mutex_unlock(&parent->lock);
	return B_OK;
}


status_t
socket_notify(net_socket *_socket, uint8 event, int32 value)
{
	net_socket_private *socket = (net_socket_private *)_socket;

	mutex_lock(&socket->lock);

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

	if (notify)
		;

	mutex_unlock(&socket->lock);
	return B_OK;
}


net_socket_module_info gNetSocketModule = {
	{
		NET_SOCKET_MODULE_NAME,
		0,
		std_ops
	},
	NULL, // open,
	NULL, // close,
	NULL, // free,

	NULL, // readv,
	NULL, // writev,
	NULL, // control,

	NULL, // read_avail,
	NULL, // send_avail,

	NULL, // send_data,
	NULL, // receive_data,

	NULL, // get_option,
	NULL, // set_option,
	NULL, // get_next_stat,

	// connections
	socket_spawn_pending,
	socket_delete,
	socket_dequeue_connected,
	socket_count_connected,
	socket_set_max_backlog,
	socket_connected,

	// notifications
	NULL, // request_notification,
	NULL, // cancel_notification,
	socket_notify,

	// standard socket API
	NULL, // accept,
	NULL, // bind,
	NULL, // connect,
	NULL, // getpeername,
	NULL, // getsockname,
	NULL, // getsockopt,
	NULL, // listen,
	NULL, // receive,
	NULL, // send,
	NULL, // setsockopt,
	NULL, // shutdown,
	NULL, // socketpair
};


//	#pragma mark - protocol


net_protocol*
init_protocol(net_socket** _socket)
{
	net_socket *socket;
	status_t status = socket_create(AF_INET, SOCK_STREAM, IPPROTO_TCP, &socket);
	if (status < B_OK)
		return NULL;

	status = socket->first_info->open(socket->first_protocol);
	if (status < B_OK) {
		fprintf(stderr, "tcp_tester: cannot open client: %s\n", strerror(status));
		socket_delete(socket);
		return NULL;
	}

	*_socket = socket;
	return socket->first_protocol;
}


void
close_protocol(net_protocol* protocol)
{
	gTCPModule->close(protocol);
	if (gTCPModule->free(protocol) == B_OK)
		gTCPModule->uninit_protocol(protocol);
		//socket_delete(protocol->socket);
}


//	#pragma mark - datalink


status_t
datalink_send_data(struct net_route *route, net_buffer *buffer)
{
	struct context* context = (struct context*)route->gateway;

	buffer->interface = &gInterface;

	context->lock.Lock();
	list_add_item(&context->list, buffer);
	context->lock.Unlock();

	release_sem(context->wait_sem);
	return B_OK;
}


status_t
datalink_send_datagram(net_protocol *protocol, net_domain *domain,
	net_buffer *buffer)
{
	panic("called");
	return B_ERROR;
}


struct net_route *
get_route(struct net_domain *_domain, const struct sockaddr *address)
{
	if (is_server(address)) {
		// to the server
		return &sServerContext.route;
	}

	return &sClientContext.route;
}


net_datalink_module_info gNetDatalinkModule = {
	{
		NET_DATALINK_MODULE_NAME,
		0,
		std_ops
	},

	NULL, //datalink_control,
	datalink_send_data,
	datalink_send_datagram,

	NULL, //is_local_address,
	NULL, //datalink_get_interface,
	NULL, //datalink_get_interface_with_address,

	NULL, //add_route,
	NULL, //remove_route,
	get_route,
	NULL, //put_route,
	NULL, //register_route_info,
	NULL, //unregister_route_info,
	NULL, //update_route_info
};


//	#pragma mark - domain


status_t
domain_open(net_protocol *protocol)
{
	return B_OK;
}


status_t
domain_close(net_protocol *protocol)
{
	return B_OK;
}


status_t
domain_free(net_protocol *protocol)
{
	return B_OK;
}


status_t
domain_connect(net_protocol *protocol, const struct sockaddr *address)
{
	return B_ERROR;
}


status_t
domain_accept(net_protocol *protocol, struct net_socket **_acceptedSocket)
{
	return EOPNOTSUPP;
}


status_t
domain_control(net_protocol *protocol, int level, int option, void *value,
	size_t *_length)
{
	return B_ERROR;
}


status_t
domain_bind(net_protocol *protocol, const struct sockaddr *address)
{
	memcpy(&protocol->socket->address, address, sizeof(struct sockaddr_in));
	protocol->socket->address.ss_len = sizeof(struct sockaddr_in);
		// explicitly set length, as our callers can't be trusted to
		// always provide the correct length!
	return B_OK;
}


status_t
domain_unbind(net_protocol *protocol, struct sockaddr *address)
{
	return B_OK;
}


status_t
domain_listen(net_protocol *protocol, int count)
{
	return EOPNOTSUPP;
}


status_t
domain_shutdown(net_protocol *protocol, int direction)
{
	return EOPNOTSUPP;
}


status_t
domain_send_data(net_protocol *protocol, net_buffer *buffer)
{
	// find route
	struct net_route *route = get_route(&sDomain, (sockaddr *)&buffer->destination);
	if (route == NULL)
		return ENETUNREACH;

	return datalink_send_data(route, buffer);
}


status_t
domain_send_routed_data(net_protocol *protocol, struct net_route *route,
	net_buffer *buffer)
{
	return datalink_send_data(route, buffer);
}


ssize_t
domain_send_avail(net_protocol *protocol)
{
	return B_ERROR;
}


status_t
domain_read_data(net_protocol *protocol, size_t numBytes, uint32 flags,
	net_buffer **_buffer)
{
	return B_ERROR;
}


ssize_t
domain_read_avail(net_protocol *protocol)
{
	return B_ERROR;
}


struct net_domain *
domain_get_domain(net_protocol *protocol)
{
	return &sDomain;
}


size_t
domain_get_mtu(net_protocol *protocol, const struct sockaddr *address)
{
	return 1480;
		// 1500 ethernet - IPv4 header
}


status_t
domain_receive_data(net_buffer *buffer)
{
	static bigtime_t lastTime = 0;

	uint32 packetNumber = atomic_add(&sPacketNumber, 1);

	bool drop = false;
	if (sDropList.find(packetNumber) != sDropList.end()
		|| (sRandomDrop > 0.0 && (1.0 * rand() / RAND_MAX) > sRandomDrop))
		drop = true;

	if (!drop && (sRoundTripTime > 0 || sRandomRoundTrip || sIncreasingRoundTrip)) {
		bigtime_t add = 0;
		if (sRandomRoundTrip)
			add = (bigtime_t)(1.0 * rand() / RAND_MAX * 500000) - 250000;
		if (sIncreasingRoundTrip)
			sRoundTripTime += (bigtime_t)(1.0 * rand() / RAND_MAX * 150000);

		snooze(sRoundTripTime / 2 + add);
	}

	if (sTCPDump) {
		NetBufferHeaderReader<tcp_header> bufferHeader(buffer);
		if (bufferHeader.Status() < B_OK)
			return bufferHeader.Status();

		tcp_header &header = bufferHeader.Data();

		bigtime_t now = system_time();
		if (lastTime == 0)
			lastTime = now;

		printf("\33[0m% 3ld %8.6f (%8.6f) ", packetNumber, (now - sStartTime) / 1000000.0,
			(now - lastTime) / 1000000.0);
		lastTime = now;

		if (is_server((sockaddr *)buffer->source))
			printf("\33[31mserver > client: ");
		else
			printf("client > server: ");

		int32 length = buffer->size - header.HeaderLength();

		if ((header.flags & TCP_FLAG_PUSH) != 0)
			putchar('P');
		if ((header.flags & TCP_FLAG_SYNCHRONIZE) != 0)
			putchar('S');
		if ((header.flags & TCP_FLAG_FINISH) != 0)
			putchar('F');
		if ((header.flags & TCP_FLAG_RESET) != 0)
			putchar('R');
		if ((header.flags
			& (TCP_FLAG_SYNCHRONIZE | TCP_FLAG_FINISH | TCP_FLAG_PUSH | TCP_FLAG_RESET)) == 0)
			putchar('.');

		printf(" %lu:%lu (%lu)", header.Sequence(), header.Sequence() + length, length);
		if ((header.flags & TCP_FLAG_ACKNOWLEDGE) != 0)
			printf(" ack %lu", header.Acknowledge());

		printf(" win %u", header.AdvertisedWindow());

		if (header.HeaderLength() > sizeof(tcp_header)) {
			int32 size = header.HeaderLength() - sizeof(tcp_header);

			tcp_option *option;
			uint8 optionsBuffer[1024];
			if (gBufferModule->direct_access(buffer, sizeof(tcp_header),
					size, (void **)&option) != B_OK) {
				if (size > 1024) {
					printf("options too large to take into account (%ld bytes)\n", size);
					size = 1024;
				}

				gBufferModule->read(buffer, sizeof(tcp_header), optionsBuffer, size);
				option = (tcp_option *)optionsBuffer;
			}

			while (size > 0) {
				uint32 length = 1;
				switch (option->kind) {
					case TCP_OPTION_END:
					case TCP_OPTION_NOP:
						break;
					case TCP_OPTION_MAX_SEGMENT_SIZE:
						printf(" <mss %u>", ntohs(option->max_segment_size));
						length = 4;
						break;
					case TCP_OPTION_WINDOW_SHIFT:
						printf(" <ws %u>", option->window_shift);
						length = 3;
						break;
					case TCP_OPTION_TIMESTAMP:
						printf(" <ts %lu:%lu>", option->timestamp.value, option->timestamp.reply);
						length = 10;
						break;

					default:
						length = option->length;
						// make sure we don't end up in an endless loop
						if (length == 0)
							size = 0;
						break;
				}

				size -= length;
				option = (tcp_option *)((uint8 *)option + length);
			}
		}

		if (drop)
			printf(" <DROPPED>");
		printf("\33[0m\n");
	} else if (drop)
		printf("<**** DROPPED %ld ****>\n", packetNumber);

	if (drop) {
		gNetBufferModule.free(buffer);
		return B_OK;
	}

	return gTCPModule->receive_data(buffer);
}


status_t
domain_error(uint32 code, net_buffer *data)
{
	return B_ERROR;
}


status_t
domain_error_reply(net_protocol *protocol, net_buffer *causedError, uint32 code,
	void *errorData)
{
	return B_ERROR;
}


net_protocol_module_info gDomainModule = {
	{
		NULL,
		0,
		std_ops
	},
	NET_PROTOCOL_ATOMIC_MESSAGES,

	NULL, // init
	NULL, // uninit
	domain_open,
	domain_close,
	domain_free,
	domain_connect,
	domain_accept,
	domain_control,
	NULL, // getsockopt
	NULL, // setsockopt
	domain_bind,
	domain_unbind,
	domain_listen,
	domain_shutdown,
	domain_send_data,
	domain_send_routed_data,
	domain_send_avail,
	domain_read_data,
	domain_read_avail,
	domain_get_domain,
	domain_get_mtu,
	domain_receive_data,
	NULL, // deliver_data
	domain_error,
	domain_error_reply,
	NULL, // attach_ancillary_data
	NULL, // process_ancillary_data
};


//	#pragma mark - test


int32
receiving_thread(void* _data)
{
	struct context* context = (struct context*)_data;
	struct net_buffer* reorderBuffer = NULL;

	while (true) {
		status_t status;
		do {
			status = acquire_sem(context->wait_sem);
		} while (status == B_INTERRUPTED);

		if (status < B_OK)
			break;

		while (true) {
			context->lock.Lock();
			net_buffer* buffer = (net_buffer*)list_remove_head_item(
				&context->list);
			context->lock.Unlock();

			if (buffer == NULL)
				break;

			if (sSimultaneousConnect && context->server && is_syn(buffer)) {
				// delay getting the SYN request, and connect as well
				sockaddr_in address;
				memset(&address, 0, sizeof(address));
				address.sin_family = AF_INET;
				address.sin_port = htons(1023);
				address.sin_addr.s_addr = htonl(0xc0a80001);

				status_t status = socket_connect(gServerSocket,
					(struct sockaddr *)&address, sizeof(struct sockaddr));
				if (status < B_OK)
					fprintf(stderr, "tcp_tester: simultaneous connect failed: %s\n", strerror(status));

				sSimultaneousConnect = false;
			}
			if (sSimultaneousClose && !context->server && is_fin(buffer)) {
				close_protocol(gClientSocket->first_protocol);
				sSimultaneousClose = false;
			}
			if ((sRandomReorder > 0.0
					|| sReorderList.find(sPacketNumber) != sReorderList.end())
				&& reorderBuffer == NULL
				&& (1.0 * rand() / RAND_MAX) > sRandomReorder) {
				reorderBuffer = buffer;
			} else {
				if (sDomain.module->receive_data(buffer) < B_OK)
					gNetBufferModule.free(buffer);

				if (reorderBuffer != NULL) {
					if (sDomain.module->receive_data(reorderBuffer) < B_OK)
						gNetBufferModule.free(reorderBuffer);
					reorderBuffer = NULL;
				}
			}
		}
	}

	return 0;
}


int32
server_thread(void*)
{
	while (true) {
		// main accept() loop
		net_socket* connectionSocket;
		sockaddr_in address;
		socklen_t size = sizeof(struct sockaddr_in);
		status_t status = socket_accept(gServerSocket,
			(struct sockaddr *)&address, &size, &connectionSocket);
		if (status < B_OK) {
			fprintf(stderr, "SERVER: accepting failed: %s\n", strerror(status));
			break;
		}

		printf("server: got connection from %08x\n", address.sin_addr.s_addr);

		char buffer[1024];
		ssize_t bytesRead;
		while ((bytesRead = socket_recv(connectionSocket, buffer,
				sizeof(buffer), 0)) > 0) {
			printf("server: received %ld bytes\n", bytesRead);

			if (sServerActiveClose) {
				printf("server: active close\n");
				close_protocol(connectionSocket->first_protocol);
				return 0;
			}
		}
		if (bytesRead < 0)
			printf("server: receiving failed: %s\n", strerror(bytesRead));
		else
			printf("server: peer closed connection.\n");

		snooze(1000000);
		close_protocol(connectionSocket->first_protocol);
	}

	return 0;
}


void
setup_server()
{
	sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_len = sizeof(sockaddr_in);
	address.sin_family = AF_INET;
	address.sin_port = htons(1024);
	address.sin_addr.s_addr = INADDR_ANY;

	status_t status = socket_bind(gServerSocket, (struct sockaddr*)&address,
		sizeof(struct sockaddr));
	if (status < B_OK) {
		fprintf(stderr, "tcp_tester: cannot bind server: %s\n", strerror(status));
		exit(1);
	}
	status = socket_listen(gServerSocket, 40);
	if (status < B_OK) {
		fprintf(stderr, "tcp_tester: server cannot listen: %s\n",
			strerror(status));
		exit(1);
	}

	thread_id serverThread = spawn_thread(server_thread, "server",
		B_NORMAL_PRIORITY, NULL);
	if (serverThread < B_OK) {
		fprintf(stderr, "tcp_tester: cannot start server: %s\n",
			strerror(serverThread));
		exit(1);
	}

	resume_thread(serverThread);
}


void
setup_context(struct context& context, bool server)
{
	list_init(&context.list);
	context.route.interface = &gInterface;
	context.route.gateway = (sockaddr *)&context;
		// backpointer to the context
	context.route.mtu = 1500;
	context.server = server;
	context.wait_sem = create_sem(0, "receive wait");

	context.thread = spawn_thread(receiving_thread,
		server ? "server receiver" : "client receiver", B_NORMAL_PRIORITY,
		&context);
	resume_thread(context.thread);
}


void
cleanup_context(struct context& context)
{
	delete_sem(context.wait_sem);

	status_t status;
	wait_for_thread(context.thread, &status);
}


//	#pragma mark -


static void do_help(int argc, char** argv);


static void
do_connect(int argc, char** argv)
{
	sSimultaneousConnect = false;

	int port = 1024;
	if (argc > 1) {
		if (!strcmp(argv[1], "-s"))
			sSimultaneousConnect = true;
		else if (isdigit(argv[1][0]))
			port = atoi(argv[1]);
		else {
			fprintf(stderr, "usage: connect [-s|<port-number>]\n");
			return;
		}
	}

	if (sSimultaneousConnect) {
		// bind to a port first, so the other end can find us
		sockaddr_in address;
		memset(&address, 0, sizeof(address));
		address.sin_family = AF_INET;
		address.sin_port = htons(1023);
		address.sin_addr.s_addr = htonl(0xc0a80001);

		status_t status = socket_bind(gClientSocket, (struct sockaddr *)&address,
			sizeof(struct sockaddr));
		if (status < B_OK) {
			fprintf(stderr, "Could not bind to port 1023: %s\n", strerror(status));
			sSimultaneousConnect = false;
			return;
		}
	}

	sStartTime = system_time();

	sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htonl(0xc0a80001);

	status_t status = socket_connect(gClientSocket, (struct sockaddr *)&address,
		sizeof(struct sockaddr));
	if (status < B_OK)
		fprintf(stderr, "tcp_tester: could not connect: %s\n", strerror(status));
}


static void
do_send(int argc, char** argv)
{
	size_t size = 1024;
	if (argc > 1 && isdigit(argv[1][0])) {
		char *unit;
		size = strtoul(argv[1], &unit, 0);
		if (unit != NULL && unit[0]) {
			if (unit[0] == 'k' || unit[0] == 'K')
				size *= 1024;
			else if (unit[0] == 'm' || unit[0] == 'M')
				size *= 1024 * 1024;
			else {
				fprintf(stderr, "unknown unit specified!\n");
				return;
			}
		}
	} else if (argc > 1) {
		fprintf(stderr, "invalid args!\n");
		return;
	}

	if (size > 4 * 1024 * 1024) {
		printf("amount to send will be limited to 4 MB\n");
		size = 4 * 1024 * 1024;
	}

	char *buffer = (char *)malloc(size);
	if (buffer == NULL) {
		fprintf(stderr, "not enough memory!\n");
		return;
	}

	// initialize buffer with some not so random data
	for (uint32 i = 0; i < size; i++) {
		buffer[i] = (char)(i & 0xff);
	}

	ssize_t bytesWritten = socket_send(gClientSocket, buffer, size, 0);
	if (bytesWritten < B_OK) {
		fprintf(stderr, "failed sending buffer: %s\n", strerror(bytesWritten));
		return;
	}
}


static void
do_close(int argc, char** argv)
{
	sSimultaneousClose = false;
	sServerActiveClose = true;

	if (argc > 1) {
		if (!strcmp(argv[1], "-s"))
			sSimultaneousClose = true;
		else {
			fprintf(stderr, "usage: close [-s]\n");
			return;
		}
	}

	gClientSocket->send.timeout = 0;

	char buffer[32767] = {'q'};
	ssize_t bytesWritten = socket_send(gClientSocket, buffer, sizeof(buffer), 0);
	if (bytesWritten < B_OK) {
		fprintf(stderr, "failed sending buffer: %s\n", strerror(bytesWritten));
		return;
	}
}


static void
do_drop(int argc, char** argv)
{
	if (argc == 1) {
		// show list of dropped packets
		if (sRandomDrop > 0.0)
			printf("Drop probability is %f\n", sRandomDrop);

		printf("Drop pakets:\n");

		set<uint32>::iterator iterator = sDropList.begin();
		uint32 count = 0;
		for (; iterator != sDropList.end(); iterator++) {
			printf("%4lu\n", *iterator);
			count++;
		}

		if (count == 0)
			printf("<empty>\n");
	} else if (!strcmp(argv[1], "-f")) {
		// flush drop list
		sDropList.clear();
		puts("drop list cleared.");
	} else if (!strcmp(argv[1], "-r")) {
		if (argc < 3) {
			fprintf(stderr, "No drop probability specified.\n");
			return;
		}

		sRandomDrop = atof(argv[2]);
		if (sRandomDrop < 0.0)
			sRandomDrop = 0;
		else if (sRandomDrop > 1.0)
			sRandomDrop = 1.0;
	} else if (isdigit(argv[1][0])) {
		// add to drop list
		for (int i = 1; i < argc; i++) {
			uint32 packet = strtoul(argv[i], NULL, 0);
			if (packet == 0) {
				fprintf(stderr, "invalid packet number: %s\n", argv[i]);
				break;
			}

			sDropList.insert(packet);
		}
	} else {
		// print usage
		puts("usage: drop <packet-number> [...]\n"
			"   or: drop -r <probability>\n\n"
			"   or: drop [-f]\n\n"
			"Specifiying -f flushes the drop list, -r sets the probability a packet\n"
			"is dropped; if you called drop without any arguments, the current\n"
			"drop list is dumped.");
	}
}


static void
do_reorder(int argc, char** argv)
{
	if (argc == 1) {
		// show list of dropped packets
		if (sRandomReorder > 0.0)
			printf("Reorder probability is %f\n", sRandomReorder);

		printf("Reorder packets:\n");

		set<uint32>::iterator iterator = sReorderList.begin();
		uint32 count = 0;
		for (; iterator != sReorderList.end(); iterator++) {
			printf("%4lu\n", *iterator);
			count++;
		}

		if (count == 0)
			printf("<empty>\n");
	} else if (!strcmp(argv[1], "-f")) {
		// flush reorder list
		sReorderList.clear();
		puts("reorder list cleared.");
	} else if (!strcmp(argv[1], "-r")) {
		if (argc < 3) {
			fprintf(stderr, "No reorder probability specified.\n");
			return;
		}

		sRandomReorder = atof(argv[2]);
		if (sRandomReorder < 0.0)
			sRandomReorder = 0;
		else if (sRandomReorder > 1.0)
			sRandomReorder = 1.0;
	} else if (isdigit(argv[1][0])) {
		// add to reorder list
		for (int i = 1; i < argc; i++) {
			uint32 packet = strtoul(argv[i], NULL, 0);
			if (packet == 0) {
				fprintf(stderr, "invalid packet number: %s\n", argv[i]);
				break;
			}

			sReorderList.insert(packet);
		}
	} else {
		// print usage
		puts("usage: reorder <packet-number> [...]\n"
			"   or: reorder -r <probability>\n\n"
			"   or: reorder [-f]\n\n"
			"Specifiying -f flushes the reorder list, -r sets the probability a packet\n"
			"is reordered; if you called reorder without any arguments, the current\n"
			"reorder list is dumped.");
	}
}


static void
do_round_trip_time(int argc, char** argv)
{
	if (argc == 1) {
		// show current time
		printf("Current round trip time: %g ms\n", sRoundTripTime / 1000.0);
	} else if (!strcmp(argv[1], "-r")) {
		// toggle random time
		sRandomRoundTrip = !sRandomRoundTrip;
		printf("Round trip time is now %s.\n", sRandomRoundTrip ? "random" : "fixed");
	} else if (!strcmp(argv[1], "-i")) {
		// toggle increasing time
		sIncreasingRoundTrip = !sIncreasingRoundTrip;
		printf("Round trip time is now %s.\n", sIncreasingRoundTrip ? "increasing" : "fixed");
	} else if (isdigit(argv[1][0])) {
		// set time
		sRoundTripTime = 1000LL * strtoul(argv[1], NULL, 0);
	} else {
		// print usage
		puts("usage: rtt <time in ms>\n"
			"   or: rtt [-r|-i]\n\n"
			"Specifiying -r sets random time, -i causes the times to increase over time;\n"
			"witout any arguments, the current time is printed.");
	}
}


static void
do_dprintf(int argc, char** argv)
{
	if (argc > 1)
		gDebugOutputEnabled = !strcmp(argv[1], "on");
	else
		gDebugOutputEnabled = !gDebugOutputEnabled;

	printf("debug output turned %s.\n", gDebugOutputEnabled ? "on" : "off");
}


static cmd_entry sBuiltinCommands[] = {
	{"connect", do_connect, "Connects the client"},
	{"send", do_send, "Sends data from the client to the server"},
	{"close", do_close, "Performs an active or simultaneous close"},
	{"dprintf", do_dprintf, "Toggles debug output"},
	{"drop", do_drop, "Lets you drop packets during transfer"},
	{"reorder", do_reorder, "Lets you reorder packets during transfer"},
	{"help", do_help, "prints this help text"},
	{"rtt", do_round_trip_time, "Specifies the round trip time"},
	{"quit", NULL, "exits the application"},
	{NULL, NULL, NULL},
};


static void
do_help(int argc, char** argv)
{
	printf("Available commands:\n");

	for (cmd_entry* command = sBuiltinCommands; command->name != NULL; command++) {
		printf("%8s - %s\n", command->name, command->help);
	}
}


//	#pragma mark -


int
main(int argc, char** argv)
{
	status_t status = init_timers();
	if (status < B_OK) {
		fprintf(stderr, "tcp_tester: Could not initialize timers: %s\n",
			strerror(status));
		return 1;
	}

	_add_builtin_module((module_info*)&gNetStackModule);
	_add_builtin_module((module_info*)&gNetBufferModule);
	_add_builtin_module((module_info*)&gNetSocketModule);
	_add_builtin_module((module_info*)&gNetDatalinkModule);
	_add_builtin_module(modules[0]);
	if (_get_builtin_dependencies() < B_OK) {
		fprintf(stderr, "tcp_tester: Could not initialize modules: %s\n",
			strerror(status));
		return 1;
	}

	sockaddr_in interfaceAddress;
	interfaceAddress.sin_len = sizeof(sockaddr_in);
	interfaceAddress.sin_family = AF_INET;
	interfaceAddress.sin_addr.s_addr = htonl(0xc0a80001);
	gInterface.address = (sockaddr*)&interfaceAddress;
	gInterface.domain = &sDomain;

	status = get_module("network/protocols/tcp/v1", (module_info **)&gTCPModule);
	if (status < B_OK) {
		fprintf(stderr, "tcp_tester: Could not open TCP module: %s\n",
			strerror(status));
		return 1;
	}

	net_protocol* client = init_protocol(&gClientSocket);
	if (client == NULL)
		return 1;
	net_protocol* server = init_protocol(&gServerSocket);
	if (server == NULL)
		return 1;

	setup_context(sClientContext, false);
	setup_context(sServerContext, true);

	printf("*** Server: %p (%ld), Client: %p (%ld)\n", server,
		sServerContext.thread, client, sClientContext.thread);

	setup_server();

	while (true) {
		printf("> ");
		fflush(stdout);

		char line[1024];
		if (fgets(line, sizeof(line), stdin) == NULL)
			break;

        argc = 0;
        argv = build_argv(line, &argc);
        if (argv == NULL || argc == 0)
            continue;

        int length = strlen(argv[0]);

#if 0
		char *newLine = strchr(line, '\n');
		if (newLine != NULL)
			newLine[0] = '\0';
#endif

		if (!strcmp(argv[0], "quit")
			|| !strcmp(argv[0], "exit")
			|| !strcmp(argv[0], "q"))
			break;

		bool found = false;

		for (cmd_entry* command = sBuiltinCommands; command->name != NULL; command++) {
			if (!strncmp(command->name, argv[0], length)) {
				command->func(argc, argv);
				found = true;
				break;
			}
		}

		if (!found)
			fprintf(stderr, "Unknown command \"%s\". Type \"help\" for a list of commands.\n", argv[0]);

		free(argv);
	}

	close_protocol(client);
	close_protocol(server);

	snooze(2000000);

	cleanup_context(sClientContext);
	cleanup_context(sServerContext);

	put_module("network/protocols/tcp/v1");
	uninit_timers();
	return 0;
}
