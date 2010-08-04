/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <sys/un.h>

#include <new>

#include <AutoDeleter.h>

#include <fs/fd.h>
#include <lock.h>
#include <util/AutoLock.h>
#include <vfs.h>

#include <net_buffer.h>
#include <net_protocol.h>
#include <net_socket.h>
#include <net_stack.h>

#include "UnixAddressManager.h"
#include "UnixEndpoint.h"


#define UNIX_MODULE_DEBUG_LEVEL	0
#define UNIX_DEBUG_LEVEL		UNIX_MODULE_DEBUG_LEVEL
#include "UnixDebug.h"


extern net_protocol_module_info gUnixModule;
	// extern only for forwarding

net_stack_module_info *gStackModule;
net_socket_module_info *gSocketModule;
net_buffer_module_info *gBufferModule;
UnixAddressManager gAddressManager;

static struct net_domain *sDomain;


void
destroy_scm_rights_descriptors(const ancillary_data_header* header,
	void* data)
{
	int count = header->len / sizeof(file_descriptor*);
	file_descriptor** descriptors = (file_descriptor**)data;

	for (int i = 0; i < count; i++) {
		if (descriptors[i] != NULL) {
			close_fd(descriptors[i]);
			put_fd(descriptors[i]);
		}
	}
}


// #pragma mark -


net_protocol *
unix_init_protocol(net_socket *socket)
{
	TRACE("[%ld] unix_init_protocol(%p)\n", find_thread(NULL), socket);

	UnixEndpoint* endpoint = new(std::nothrow) UnixEndpoint(socket);
	if (endpoint == NULL)
		return NULL;

	status_t error = endpoint->Init();
	if (error != B_OK) {
		delete endpoint;
		return NULL;
	}

	return endpoint;
}


status_t
unix_uninit_protocol(net_protocol *_protocol)
{
	TRACE("[%ld] unix_uninit_protocol(%p)\n", find_thread(NULL), _protocol);
	((UnixEndpoint*)_protocol)->Uninit();
	return B_OK;
}


status_t
unix_open(net_protocol *_protocol)
{
	return ((UnixEndpoint*)_protocol)->Open();
}


status_t
unix_close(net_protocol *_protocol)
{
	return ((UnixEndpoint*)_protocol)->Close();
}


status_t
unix_free(net_protocol *_protocol)
{
	return ((UnixEndpoint*)_protocol)->Free();
}


status_t
unix_connect(net_protocol *_protocol, const struct sockaddr *address)
{
	return ((UnixEndpoint*)_protocol)->Connect(address);
}


status_t
unix_accept(net_protocol *_protocol, struct net_socket **_acceptedSocket)
{
	return ((UnixEndpoint*)_protocol)->Accept(_acceptedSocket);
}


status_t
unix_control(net_protocol *protocol, int level, int option, void *value,
	size_t *_length)
{
	return B_BAD_VALUE;
}


status_t
unix_getsockopt(net_protocol *protocol, int level, int option, void *value,
	int *_length)
{
	UnixEndpoint* endpoint = (UnixEndpoint*)protocol;

	if (level == SOL_SOCKET && option == SO_PEERCRED) {
		if (*_length < (int)sizeof(ucred))
			return B_BAD_VALUE;

		*_length = sizeof(ucred);

		return endpoint->GetPeerCredentials((ucred*)value);
	}

	return gSocketModule->get_option(protocol->socket, level, option, value,
		_length);
}


status_t
unix_setsockopt(net_protocol *protocol, int level, int option,
	const void *_value, int length)
{
	UnixEndpoint* endpoint = (UnixEndpoint*)protocol;

	if (level == SOL_SOCKET) {
		if (option == SO_RCVBUF) {
			if (length != sizeof(int))
				return B_BAD_VALUE;

			status_t error = endpoint->SetReceiveBufferSize(*(int*)_value);
			if (error != B_OK)
				return error;
		} else if (option == SO_SNDBUF) {
			// We don't have a receive buffer, so silently ignore this one.
		}
	}

	return gSocketModule->set_option(protocol->socket, level, option,
		_value, length);
}


status_t
unix_bind(net_protocol *_protocol, const struct sockaddr *_address)
{
	return ((UnixEndpoint*)_protocol)->Bind(_address);
}


status_t
unix_unbind(net_protocol *_protocol, struct sockaddr *_address)
{
	return ((UnixEndpoint*)_protocol)->Unbind();
}


status_t
unix_listen(net_protocol *_protocol, int count)
{
	return ((UnixEndpoint*)_protocol)->Listen(count);
}


status_t
unix_shutdown(net_protocol *_protocol, int direction)
{
	return ((UnixEndpoint*)_protocol)->Shutdown(direction);
}


status_t
unix_send_routed_data(net_protocol *_protocol, struct net_route *route,
	net_buffer *buffer)
{
	return B_ERROR;
}


status_t
unix_send_data(net_protocol *_protocol, net_buffer *buffer)
{
	return B_ERROR;
}


ssize_t
unix_send_avail(net_protocol *_protocol)
{
	return ((UnixEndpoint*)_protocol)->Sendable();
}


status_t
unix_read_data(net_protocol *_protocol, size_t numBytes, uint32 flags,
	net_buffer **_buffer)
{
	return B_ERROR;
}


ssize_t
unix_read_avail(net_protocol *_protocol)
{
	return ((UnixEndpoint*)_protocol)->Receivable();
}


struct net_domain *
unix_get_domain(net_protocol *protocol)
{
	return sDomain;
}


size_t
unix_get_mtu(net_protocol *protocol, const struct sockaddr *address)
{
	return UNIX_MAX_TRANSFER_UNIT;
}


status_t
unix_receive_data(net_buffer *buffer)
{
	return B_ERROR;
}


status_t
unix_deliver_data(net_protocol *_protocol, net_buffer *buffer)
{
	return B_ERROR;
}


status_t
unix_error_received(net_error error, net_buffer *data)
{
	return B_ERROR;
}


status_t
unix_error_reply(net_protocol *protocol, net_buffer *cause, net_error error,
	net_error_data *errorData)
{
	return B_ERROR;
}


status_t
unix_add_ancillary_data(net_protocol *self, ancillary_data_container *container,
	const cmsghdr *header)
{
	TRACE("[%ld] unix_add_ancillary_data(%p, %p, %p (level: %d, type: %d, "
		"len: %d))\n", find_thread(NULL), self, container, header,
		header->cmsg_level, header->cmsg_type, (int)header->cmsg_len);

	// we support only SCM_RIGHTS
	if (header->cmsg_level != SOL_SOCKET || header->cmsg_type != SCM_RIGHTS)
		return B_BAD_VALUE;

	int* fds = (int*)CMSG_DATA(header);
	int count = (header->cmsg_len - CMSG_ALIGN(sizeof(cmsghdr))) / sizeof(int);
	if (count == 0)
		return B_BAD_VALUE;

	file_descriptor** descriptors = new(std::nothrow) file_descriptor*[count];
	if (descriptors == NULL)
		return ENOBUFS;
	ArrayDeleter<file_descriptor*> _(descriptors);
	memset(descriptors, 0, sizeof(file_descriptor*) * count);

	// get the file descriptors
	io_context* ioContext = get_current_io_context(!gStackModule->is_syscall());

	status_t error = B_OK;
	for (int i = 0; i < count; i++) {
		descriptors[i] = get_open_fd(ioContext, fds[i]);
		if (descriptors[i] == NULL) {
			error = EBADF;
			break;
		}
	}

	// attach the ancillary data to the container
	if (error == B_OK) {
		ancillary_data_header header;
		header.level = SOL_SOCKET;
		header.type = SCM_RIGHTS;
		header.len = count * sizeof(file_descriptor*);

		TRACE("[%ld] unix_add_ancillary_data(): adding %d FDs to "
			"container\n", find_thread(NULL), count);

		error = gStackModule->add_ancillary_data(container, &header,
			descriptors, destroy_scm_rights_descriptors, NULL);
	}

	// cleanup on error
	if (error != B_OK) {
		for (int i = 0; i < count; i++) {
			if (descriptors[i] != NULL) {
				close_fd(descriptors[i]);
				put_fd(descriptors[i]);
			}
		}
	}

	return error;
}


ssize_t
unix_process_ancillary_data(net_protocol *self,
	const ancillary_data_header *header, const void *data, void *buffer,
	size_t bufferSize)
{
	TRACE("[%ld] unix_process_ancillary_data(%p, %p (level: %d, type: %d, "
		"len: %lu), %p, %p, %lu)\n", find_thread(NULL), self, header,
		header->level, header->type, header->len, data, buffer, bufferSize);

	// we support only SCM_RIGHTS
	if (header->level != SOL_SOCKET || header->type != SCM_RIGHTS)
		return B_BAD_VALUE;

	int count = header->len / sizeof(file_descriptor*);
	file_descriptor** descriptors = (file_descriptor**)data;

	// check if there's enough space in the buffer
	size_t neededBufferSpace = CMSG_SPACE(sizeof(int) * count);
	if (bufferSize < neededBufferSpace)
		return B_BAD_VALUE;

	// init header
	cmsghdr* messageHeader = (cmsghdr*)buffer;
	messageHeader->cmsg_level = header->level;
	messageHeader->cmsg_type = header->type;
	messageHeader->cmsg_len = CMSG_LEN(sizeof(int) * count);

	// create FDs for the current process
	int* fds = (int*)CMSG_DATA(messageHeader);
	io_context* ioContext = get_current_io_context(!gStackModule->is_syscall());

	status_t error = B_OK;
	for (int i = 0; i < count; i++) {
		// Get an additional reference which will go to the FD table index. The
		// reference and open reference acquired in unix_add_ancillary_data()
		// will be released when the container is destroyed.
		inc_fd_ref_count(descriptors[i]);
		fds[i] = new_fd(ioContext, descriptors[i]);

		if (fds[i] < 0) {
			error = fds[i];
			put_fd(descriptors[i]);

			// close FD indices
			for (int k = i - 1; k >= 0; k--)
				close_fd_index(ioContext, fds[k]);
			break;
		}
	}

	return error == B_OK ? neededBufferSpace : error;
}


ssize_t
unix_send_data_no_buffer(net_protocol *_protocol, const iovec *vecs,
	size_t vecCount, ancillary_data_container *ancillaryData,
	const struct sockaddr *address, socklen_t addressLength)
{
	return ((UnixEndpoint*)_protocol)->Send(vecs, vecCount, ancillaryData);
}


ssize_t
unix_read_data_no_buffer(net_protocol *_protocol, const iovec *vecs,
	size_t vecCount, ancillary_data_container **_ancillaryData,
	struct sockaddr *_address, socklen_t *_addressLength)
{
	return ((UnixEndpoint*)_protocol)->Receive(vecs, vecCount, _ancillaryData,
		_address, _addressLength);
}


// #pragma mark -


status_t
init_unix()
{
	new(&gAddressManager) UnixAddressManager;
	status_t error = gAddressManager.Init();
	if (error != B_OK)
		return error;

	error = gStackModule->register_domain_protocols(AF_UNIX, SOCK_STREAM, 0,
		"network/protocols/unix/v1", NULL);
	if (error != B_OK) {
		gAddressManager.~UnixAddressManager();
		return error;
	}

	error = gStackModule->register_domain(AF_UNIX, "unix", &gUnixModule,
		&gAddressModule, &sDomain);
	if (error != B_OK) {
		gAddressManager.~UnixAddressManager();
		return error;
	}

	return B_OK;
}


status_t
uninit_unix()
{
	gStackModule->unregister_domain(sDomain);

	gAddressManager.~UnixAddressManager();

	return B_OK;
}


static status_t
unix_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return init_unix();
		case B_MODULE_UNINIT:
			return uninit_unix();

		default:
			return B_ERROR;
	}
}


net_protocol_module_info gUnixModule = {
	{
		"network/protocols/unix/v1",
		0,
		unix_std_ops
	},
	0,	// NET_PROTOCOL_ATOMIC_MESSAGES,

	unix_init_protocol,
	unix_uninit_protocol,
	unix_open,
	unix_close,
	unix_free,
	unix_connect,
	unix_accept,
	unix_control,
	unix_getsockopt,
	unix_setsockopt,
	unix_bind,
	unix_unbind,
	unix_listen,
	unix_shutdown,
	unix_send_data,
	unix_send_routed_data,
	unix_send_avail,
	unix_read_data,
	unix_read_avail,
	unix_get_domain,
	unix_get_mtu,
	unix_receive_data,
	unix_deliver_data,
	unix_error_received,
	unix_error_reply,
	unix_add_ancillary_data,
	unix_process_ancillary_data,
	NULL,
	unix_send_data_no_buffer,
	unix_read_data_no_buffer
};

module_dependency module_dependencies[] = {
	{NET_STACK_MODULE_NAME, (module_info **)&gStackModule},
	{NET_BUFFER_MODULE_NAME, (module_info **)&gBufferModule},
//	{NET_DATALINK_MODULE_NAME, (module_info **)&sDatalinkModule},
	{NET_SOCKET_MODULE_NAME, (module_info **)&gSocketModule},
	{}
};

module_info *modules[] = {
	(module_info *)&gUnixModule,
	NULL
};
