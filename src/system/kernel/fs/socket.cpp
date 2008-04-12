/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */

#include <sys/socket.h>

#include <errno.h>

#include <module.h>

#include <AutoDeleter.h>

#include <syscall_utils.h>

#include <fd.h>
#include <kernel.h>
#include <syscall_restart.h>
#include <vfs.h>

#include <net_stack_interface.h>
#include <net_stat.h>


#define MAX_SOCKET_ADDRESS_LEN	(sizeof(sockaddr_storage))
#define MAX_SOCKET_OPTION_LEN	128
#define MAX_IO_VEC_COUNT		128


static net_stack_interface_module_info* sStackInterface = NULL;
static vint32 sStackInterfaceInitialized = 0;


struct FDPutter {
	FDPutter(file_descriptor* descriptor)
		: descriptor(descriptor)
	{
	}

	~FDPutter()
	{
		if (descriptor != NULL)
			put_fd(descriptor);
	}


	file_descriptor*	descriptor;
};


static net_stack_interface_module_info*
init_stack_interface_module()
{
	// TODO: Add driver settings option to load the userland net stack.

	// load module
	net_stack_interface_module_info* module;
	status_t error = get_module(NET_STACK_INTERFACE_MODULE_NAME,
		(module_info**)&module);
	if (error != B_OK)
		return NULL;

	sStackInterface = module;	// assumed to be atomic

	// If someone else was faster getting the module, we put our reference.
	if (atomic_test_and_set(&sStackInterfaceInitialized, 1, 0) != 0)
		put_module(module->info.name);

	return module;
}


static inline net_stack_interface_module_info*
get_stack_interface_module()
{
	if (sStackInterface)
		return sStackInterface;

	return init_stack_interface_module();
}


static status_t
prepare_userland_address_result(struct sockaddr* userAddress,
	socklen_t* _addressLength, socklen_t& addressLength, bool addressRequired)
{
	// check parameters
	if (_addressLength == NULL)
		return B_BAD_VALUE;
	if (userAddress == NULL) {
		if (addressRequired)
			return B_BAD_VALUE;
	} else if (!IS_USER_ADDRESS(userAddress)
			|| !IS_USER_ADDRESS(_addressLength)) {
		return B_BAD_ADDRESS;
	}

	// copy the buffer size from userland		
	addressLength = 0;
	if (userAddress != NULL
			&& user_memcpy(&addressLength, _addressLength, sizeof(socklen_t))
				!= B_OK) {
		return B_BAD_ADDRESS;
	}

	if (addressLength > MAX_SOCKET_ADDRESS_LEN)
		addressLength = MAX_SOCKET_ADDRESS_LEN;

	return B_OK;
}


static status_t
prepare_userland_msghdr(const msghdr* userMessage, msghdr& message,
	iovec*& userVecs, iovec* vecs, void*& userAddress, char* address)
{
	if (userMessage == NULL)
		return B_BAD_VALUE;

	// copy message from userland
	if (!IS_USER_ADDRESS(userMessage)
			|| user_memcpy(&message, userMessage, sizeof(msghdr)) != B_OK) {
		return B_BAD_ADDRESS;
	}

	// copy iovecs from userland
	if (userVecs != NULL && message.msg_iovlen > 0) {
		if (message.msg_iovlen > MAX_IO_VEC_COUNT || message.msg_iov == NULL)
			return B_BAD_VALUE;
		if (!IS_USER_ADDRESS(message.msg_iov)
				|| user_memcpy(vecs, message.msg_iov,
					message.msg_iovlen * sizeof(iovec)) != B_OK) {
			return B_BAD_ADDRESS;
		}

		message.msg_iov = vecs;
	} else {
		message.msg_iov = NULL;
		message.msg_iovlen = 0;
	}

	// prepare the address field
	userAddress = message.msg_name;
	if (userAddress != NULL) {
		if (!IS_USER_ADDRESS(message.msg_name))
			return B_BAD_ADDRESS;
		if (message.msg_namelen > MAX_SOCKET_ADDRESS_LEN)
			message.msg_namelen = MAX_SOCKET_ADDRESS_LEN;

		message.msg_name = address;
	}

	return B_OK;
}


static status_t
get_socket_descriptor(int fd, bool kernel, file_descriptor*& descriptor)
{
	if (fd < 0)
		return EBADF;

	descriptor = get_fd(get_current_io_context(kernel), fd);
	if (descriptor == NULL)
		return EBADF;

	if (descriptor->type != FDTYPE_SOCKET) {
		put_fd(descriptor);
		return ENOTSOCK;
	}

	return B_OK;
}


#define GET_SOCKET_FD_OR_RETURN(fd, kernel, descriptor)	\
	do {												\
		status_t gsfdorError = get_socket_descriptor(fd, kernel, descriptor); \
		if (gsfdorError != B_OK)						\
			return gsfdorError;							\
	} while (false)


// #pragma mark - socket file descriptor


static status_t
socket_read(struct file_descriptor *descriptor, off_t pos, void *buffer,
	size_t *_length)
{
	ssize_t bytesRead = sStackInterface->recv(descriptor->u.socket, buffer,
		*_length, 0);
	*_length = bytesRead >= 0 ? bytesRead : 0;
	return bytesRead >= 0 ? B_OK : bytesRead;
}


static status_t
socket_write(struct file_descriptor *descriptor, off_t pos, const void *buffer,
	size_t *_length)
{
	ssize_t bytesWritten = sStackInterface->send(descriptor->u.socket, buffer,
		*_length, 0);
	*_length = bytesWritten >= 0 ? bytesWritten : 0;
	return bytesWritten >= 0 ? B_OK : bytesWritten;
}


static status_t
socket_ioctl(struct file_descriptor *descriptor, ulong op, void *buffer,
	size_t length)
{
	return sStackInterface->ioctl(descriptor->u.socket, op, buffer, length);
}


static status_t
socket_set_flags(struct file_descriptor *descriptor, int flags)
{
	// we ignore O_APPEND, but O_NONBLOCK we need to translate
	uint32 op = (flags & O_NONBLOCK) != 0
		? B_SET_NONBLOCKING_IO : B_SET_BLOCKING_IO;

	return sStackInterface->ioctl(descriptor->u.socket, op, NULL, 0);
}


static status_t
socket_select(struct file_descriptor *descriptor, uint8 event,
	struct selectsync *sync)
{
	return sStackInterface->select(descriptor->u.socket, event, sync);
}


static status_t
socket_deselect(struct file_descriptor *descriptor, uint8 event,
	struct selectsync *sync)
{
	return sStackInterface->deselect(descriptor->u.socket, event, sync);
}


static status_t
socket_read_stat(struct file_descriptor *descriptor, struct stat *st)
{
	st->st_dev = 0;
	st->st_ino = (addr_t)descriptor->u.socket;
	st->st_mode = S_IFSOCK | 0666;
	st->st_nlink = 1;
	st->st_uid = 0;
	st->st_gid = 0;
	st->st_size = 0;
	st->st_rdev = 0;
	st->st_blksize = 1024;	// use MTU for datagram sockets?
	time_t now = time(NULL);
	st->st_atime = now;
	st->st_mtime = now;
	st->st_ctime = now;
	st->st_crtime = now;
	st->st_type = 0;

	return B_OK;
}


static status_t
socket_close(struct file_descriptor *descriptor)
{
	return sStackInterface->close(descriptor->u.socket);
}


static void
socket_free(struct file_descriptor *descriptor)
{
	sStackInterface->free(descriptor->u.socket);
}


static struct fd_ops sSocketFDOps = {
	&socket_read,
	&socket_write,
	NULL,	// fd_seek
	&socket_ioctl,
	&socket_set_flags,
	&socket_select,
	&socket_deselect,
	NULL,	// fd_read_dir
	NULL,	// fd_rewind_dir
	&socket_read_stat,
	NULL,	// fd_write_stat
	&socket_close,
	&socket_free
};


static int
create_socket_fd(net_socket* socket, bool kernel)
{
	// allocate a file descriptor
	file_descriptor* descriptor = alloc_fd();
	if (descriptor == NULL)
		return B_NO_MEMORY;

	// init it
	descriptor->type = FDTYPE_SOCKET;
	descriptor->ops = &sSocketFDOps;
	descriptor->u.socket = socket;
	descriptor->open_mode = O_RDWR;

	// publish it
	int fd = new_fd(get_current_io_context(kernel), descriptor);
	if (fd < 0)
		free(descriptor);

	return fd;	
}


// #pragma mark - common sockets API implementation


static int
common_socket(int family, int type, int protocol, bool kernel)
{
	if (!get_stack_interface_module())
		return B_UNSUPPORTED;

	// create the socket
	net_socket* socket;
	status_t error = sStackInterface->open(family, type, protocol, &socket);
	if (error != B_OK)
		return error;

	// allocate the FD
	int fd = create_socket_fd(socket, kernel);
	if (fd < 0) {
		sStackInterface->close(socket);
		sStackInterface->free(socket);
	}

	return fd;
}


static status_t
common_bind(int fd, const struct sockaddr *address, socklen_t addressLength,
	bool kernel)
{
	file_descriptor* descriptor;
	GET_SOCKET_FD_OR_RETURN(fd, kernel, descriptor);
	FDPutter _(descriptor);

	return sStackInterface->bind(descriptor->u.socket, address, addressLength);
}


static status_t
common_shutdown(int fd, int how, bool kernel)
{
	file_descriptor* descriptor;
	GET_SOCKET_FD_OR_RETURN(fd, kernel, descriptor);
	FDPutter _(descriptor);

	return sStackInterface->shutdown(descriptor->u.socket, how);
}


static status_t
common_connect(int fd, const struct sockaddr *address,
	socklen_t addressLength, bool kernel)
{
	file_descriptor* descriptor;
	GET_SOCKET_FD_OR_RETURN(fd, kernel, descriptor);
	FDPutter _(descriptor);

	return sStackInterface->connect(descriptor->u.socket, address,
		addressLength);
}


static status_t
common_listen(int fd, int backlog, bool kernel)
{
	file_descriptor* descriptor;
	GET_SOCKET_FD_OR_RETURN(fd, kernel, descriptor);
	FDPutter _(descriptor);

	return sStackInterface->listen(descriptor->u.socket, backlog);
}


static int
common_accept(int fd, struct sockaddr *address, socklen_t *_addressLength,
	bool kernel)
{
	file_descriptor* descriptor;
	GET_SOCKET_FD_OR_RETURN(fd, kernel, descriptor);
	FDPutter _(descriptor);

	net_socket* acceptedSocket;
	status_t error = sStackInterface->accept(descriptor->u.socket, address,
		_addressLength, &acceptedSocket);
	if (error != B_OK)
		return error;

	// allocate the FD
	int acceptedFD = create_socket_fd(acceptedSocket, kernel);
	if (acceptedFD < 0) {
		sStackInterface->close(acceptedSocket);
		sStackInterface->free(acceptedSocket);
	}

	return acceptedFD;
}


static ssize_t
common_recv(int fd, void *data, size_t length, int flags, bool kernel)
{
	file_descriptor* descriptor;
	GET_SOCKET_FD_OR_RETURN(fd, kernel, descriptor);
	FDPutter _(descriptor);

	return sStackInterface->recv(descriptor->u.socket, data, length, flags);
}


static ssize_t
common_recvfrom(int fd, void *data, size_t length, int flags,
	struct sockaddr *address, socklen_t *_addressLength, bool kernel)
{
	file_descriptor* descriptor;
	GET_SOCKET_FD_OR_RETURN(fd, kernel, descriptor);
	FDPutter _(descriptor);

	return sStackInterface->recvfrom(descriptor->u.socket, data, length,
		flags, address, _addressLength);
}


static ssize_t
common_recvmsg(int fd, struct msghdr *message, int flags, bool kernel)
{
	file_descriptor* descriptor;
	GET_SOCKET_FD_OR_RETURN(fd, kernel, descriptor);
	FDPutter _(descriptor);

	return sStackInterface->recvmsg(descriptor->u.socket, message, flags);
}


static ssize_t
common_send(int fd, const void *data, size_t length, int flags, bool kernel)
{
	file_descriptor* descriptor;
	GET_SOCKET_FD_OR_RETURN(fd, kernel, descriptor);
	FDPutter _(descriptor);

	return sStackInterface->send(descriptor->u.socket, data, length, flags);
}


static ssize_t
common_sendto(int fd, const void *data, size_t length, int flags,
	const struct sockaddr *address, socklen_t addressLength, bool kernel)
{
	file_descriptor* descriptor;
	GET_SOCKET_FD_OR_RETURN(fd, kernel, descriptor);
	FDPutter _(descriptor);

	return sStackInterface->sendto(descriptor->u.socket, data, length, flags,
		address, addressLength);
}


static ssize_t
common_sendmsg(int fd, const struct msghdr *message, int flags, bool kernel)
{
	file_descriptor* descriptor;
	GET_SOCKET_FD_OR_RETURN(fd, kernel, descriptor);
	FDPutter _(descriptor);

	return sStackInterface->sendmsg(descriptor->u.socket, message, flags);
}


static status_t
common_getsockopt(int fd, int level, int option, void *value,
	socklen_t *_length, bool kernel)
{
	file_descriptor* descriptor;
	GET_SOCKET_FD_OR_RETURN(fd, kernel, descriptor);
	FDPutter _(descriptor);

	return sStackInterface->getsockopt(descriptor->u.socket, level, option,
		value, _length);
}


static status_t
common_setsockopt(int fd, int level, int option, const void *value,
	socklen_t length, bool kernel)
{
	file_descriptor* descriptor;
	GET_SOCKET_FD_OR_RETURN(fd, kernel, descriptor);
	FDPutter _(descriptor);

	return sStackInterface->setsockopt(descriptor->u.socket, level, option,
		value, length);
}


static status_t
common_getpeername(int fd, struct sockaddr *address,
	socklen_t *_addressLength, bool kernel)
{
	file_descriptor* descriptor;
	GET_SOCKET_FD_OR_RETURN(fd, kernel, descriptor);
	FDPutter _(descriptor);

	return sStackInterface->getpeername(descriptor->u.socket, address,
		_addressLength);
}


static status_t
common_getsockname(int fd, struct sockaddr *address,
	socklen_t *_addressLength, bool kernel)
{
	file_descriptor* descriptor;
	GET_SOCKET_FD_OR_RETURN(fd, kernel, descriptor);
	FDPutter _(descriptor);

	return sStackInterface->getsockname(descriptor->u.socket, address,
		_addressLength);
}


static int
common_sockatmark(int fd, bool kernel)
{
	file_descriptor* descriptor;
	GET_SOCKET_FD_OR_RETURN(fd, kernel, descriptor);
	FDPutter _(descriptor);

	return sStackInterface->sockatmark(descriptor->u.socket);
}


static status_t
common_socketpair(int family, int type, int protocol, int fds[2], bool kernel)
{
	if (!get_stack_interface_module())
		return B_UNSUPPORTED;

	net_socket* sockets[2];
	status_t error = sStackInterface->socketpair(family, type, protocol,
		sockets);
	if (error != B_OK)
		return error;

	// allocate the FDs
	for (int i = 0; i < 2; i++) {
		fds[i] = create_socket_fd(sockets[i], kernel);
		if (fds[i] < 0) {
			sStackInterface->close(sockets[i]);
			sStackInterface->free(sockets[i]);
			return fds[i];
		}
	}

	return B_OK;
}


static status_t
common_get_next_socket_stat(int family, uint32 *cookie, struct net_stat *stat)
{
	if (!get_stack_interface_module())
		return B_UNSUPPORTED;

	return sStackInterface->get_next_socket_stat(family, cookie, stat);
}


// #pragma mark - kernel sockets API


int
socket(int family, int type, int protocol)
{
	SyscallFlagUnsetter _;
	RETURN_AND_SET_ERRNO(common_socket(family, type, protocol, true));
}


int
bind(int socket, const struct sockaddr *address, socklen_t addressLength)
{
	SyscallFlagUnsetter _;
	RETURN_AND_SET_ERRNO(common_bind(socket, address, addressLength, true));
}


int
shutdown(int socket, int how)
{
	SyscallFlagUnsetter _;
	RETURN_AND_SET_ERRNO(common_shutdown(socket, how, true));
}


int
connect(int socket, const struct sockaddr *address, socklen_t addressLength)
{
	SyscallFlagUnsetter _;
	RETURN_AND_SET_ERRNO(common_connect(socket, address, addressLength, true));
}


int
listen(int socket, int backlog)
{
	SyscallFlagUnsetter _;
	RETURN_AND_SET_ERRNO(common_listen(socket, backlog, true));
}


int
accept(int socket, struct sockaddr *address, socklen_t *_addressLength)
{
	SyscallFlagUnsetter _;
	RETURN_AND_SET_ERRNO(common_accept(socket, address, _addressLength, true));
}


ssize_t
recv(int socket, void *data, size_t length, int flags)
{
	SyscallFlagUnsetter _;
	RETURN_AND_SET_ERRNO(common_recv(socket, data, length, flags, true));
}


ssize_t
recvfrom(int socket, void *data, size_t length, int flags,
	struct sockaddr *address, socklen_t *_addressLength)
{
	SyscallFlagUnsetter _;
	RETURN_AND_SET_ERRNO(common_recvfrom(socket, data, length, flags, address,
		_addressLength, true));
}


ssize_t
recvmsg(int socket, struct msghdr *message, int flags)
{
	SyscallFlagUnsetter _;
	RETURN_AND_SET_ERRNO(common_recvmsg(socket, message, flags, true));
}


ssize_t
send(int socket, const void *data, size_t length, int flags)
{
	SyscallFlagUnsetter _;
	RETURN_AND_SET_ERRNO(common_send(socket, data, length, flags, true));
}


ssize_t
sendto(int socket, const void *data, size_t length, int flags,
	const struct sockaddr *address, socklen_t addressLength)
{
	SyscallFlagUnsetter _;
	RETURN_AND_SET_ERRNO(common_sendto(socket, data, length, flags, address,
		addressLength, true));
}


ssize_t
sendmsg(int socket, const struct msghdr *message, int flags)
{
	SyscallFlagUnsetter _;
	RETURN_AND_SET_ERRNO(common_sendmsg(socket, message, flags, true));
}


int
getsockopt(int socket, int level, int option, void *value, socklen_t *_length)
{
	SyscallFlagUnsetter _;
	RETURN_AND_SET_ERRNO(common_getsockopt(socket, level, option, value,
		_length, true));
}


int
setsockopt(int socket, int level, int option, const void *value,
	socklen_t length)
{
	SyscallFlagUnsetter _;
	RETURN_AND_SET_ERRNO(common_setsockopt(socket, level, option, value,
		length, true));
}


int
getpeername(int socket, struct sockaddr *address, socklen_t *_addressLength)
{
	SyscallFlagUnsetter _;
	RETURN_AND_SET_ERRNO(common_getpeername(socket, address, _addressLength,
		true));
}


int
getsockname(int socket, struct sockaddr *address, socklen_t *_addressLength)
{
	SyscallFlagUnsetter _;
	RETURN_AND_SET_ERRNO(common_getsockname(socket, address, _addressLength,
		true));
}


int
sockatmark(int socket)
{
	SyscallFlagUnsetter _;
	RETURN_AND_SET_ERRNO(common_sockatmark(socket, true));
}


int
socketpair(int family, int type, int protocol, int socketVector[2])
{
	SyscallFlagUnsetter _;
	RETURN_AND_SET_ERRNO(common_socketpair(family, type, protocol,
		socketVector, true));
}


// #pragma mark - syscalls


int
_user_socket(int family, int type, int protocol)
{
	SyscallRestartWrapper<int> result;
	return result = common_socket(family, type, protocol, false);
}


status_t
_user_bind(int socket, const struct sockaddr *userAddress,
	socklen_t addressLength)
{
	// check parameters and copy address from userland
	if (userAddress == NULL || addressLength > MAX_SOCKET_ADDRESS_LEN)
		return B_BAD_VALUE;

	char address[MAX_SOCKET_ADDRESS_LEN];
	if (!IS_USER_ADDRESS(userAddress)
			|| user_memcpy(address, userAddress, addressLength) != B_OK) {
		return B_BAD_ADDRESS;
	}

	SyscallRestartWrapper<status_t> error;
	return error = common_bind(socket, (sockaddr*)address, addressLength,
		false);
}


status_t
_user_shutdown_socket(int socket, int how)
{
	SyscallRestartWrapper<status_t> error;
	return error = common_shutdown(socket, how, false);
}


status_t
_user_connect(int socket, const struct sockaddr *userAddress,
	socklen_t addressLength)
{
	// check parameters and copy address from userland
	if (userAddress == NULL || addressLength > MAX_SOCKET_ADDRESS_LEN)
		return B_BAD_VALUE;

	char address[MAX_SOCKET_ADDRESS_LEN];
	if (!IS_USER_ADDRESS(userAddress)
			|| user_memcpy(address, userAddress, addressLength) != B_OK) {
		return B_BAD_ADDRESS;
	}

	SyscallRestartWrapper<status_t> error;

	return error = common_connect(socket, (sockaddr*)address, addressLength,
		false);
}


status_t
_user_listen(int socket, int backlog)
{
	SyscallRestartWrapper<status_t> error;
	return error = common_listen(socket, backlog, false);
}


int
_user_accept(int socket, struct sockaddr *userAddress,
	socklen_t *_addressLength)
{
	// check parameters
	socklen_t addressLength = 0;
	status_t error = prepare_userland_address_result(userAddress,
		_addressLength, addressLength, false);
	if (error != B_OK)
		return error;

	// accept()
	SyscallRestartWrapper<int> result;

	char address[MAX_SOCKET_ADDRESS_LEN];
	result = common_accept(socket,
		userAddress != NULL ? (sockaddr*)address : NULL, &addressLength, false);

	// copy address size and address back to userland
	if (user_memcpy(_addressLength, &addressLength,
			sizeof(socklen_t)) != B_OK
		|| userAddress != NULL
			&& user_memcpy(userAddress, address, addressLength) != B_OK) {
		_user_close(result);
		return B_BAD_ADDRESS;
	}

	return result;
}


ssize_t
_user_recv(int socket, void *data, size_t length, int flags)
{
	SyscallRestartWrapper<ssize_t> result;
	return result = common_recv(socket, data, length, flags, false);
}


ssize_t
_user_recvfrom(int socket, void *data, size_t length, int flags,
	struct sockaddr *userAddress, socklen_t *_addressLength)
{
	// check parameters
	socklen_t addressLength = 0;
	status_t error = prepare_userland_address_result(userAddress,
		_addressLength, addressLength, false);
	if (error != B_OK)
		return error;
	
	// recvfrom()
	SyscallRestartWrapper<ssize_t> result;

	char address[MAX_SOCKET_ADDRESS_LEN];
	result = common_recvfrom(socket, data, length, flags,
		userAddress != NULL ? (sockaddr*)address : NULL, &addressLength, false);
	if (result < (ssize_t)0)
		return result;

	// copy address size and address back to userland
	if (user_memcpy(_addressLength, &addressLength,
			sizeof(socklen_t)) != B_OK
		|| userAddress != NULL
			&& user_memcpy(userAddress, address, addressLength) != B_OK) {
		return B_BAD_ADDRESS;
	}

	return result;
}


ssize_t
_user_recvmsg(int socket, struct msghdr *userMessage, int flags)
{
	// copy message from userland
	msghdr message;
	iovec* userVecs = message.msg_iov;
	iovec vecs[MAX_IO_VEC_COUNT];
	void* userAddress = message.msg_name;
	char address[MAX_SOCKET_ADDRESS_LEN];

	status_t error = prepare_userland_msghdr(userMessage, message, userVecs,
		vecs, userAddress, address);
	if (error != B_OK)
		return error;

	// recvmsg()
	SyscallRestartWrapper<ssize_t> result;

	result = common_recvmsg(socket, &message, flags, false);
	if (result < (ssize_t)0)
		return result;

	// copy the address and address length back to userland
	if (userAddress != NULL && user_memcpy(userAddress, address,
			message.msg_namelen) != B_OK
		|| user_memcpy(&userMessage->msg_namelen, &message.msg_namelen,
			sizeof(message.msg_namelen)) != B_OK) {
		return B_BAD_ADDRESS;
	}

	return result;
}


ssize_t
_user_send(int socket, const void *data, size_t length, int flags)
{
	SyscallRestartWrapper<ssize_t> result;
	return result = common_send(socket, data, length, flags, false);
}


ssize_t
_user_sendto(int socket, const void *data, size_t length, int flags,
	const struct sockaddr *userAddress, socklen_t addressLength)
{
// TODO: If this is a connection-mode socket, the address parameter is
// supposed to be ignored.
	if (userAddress == NULL || addressLength <= 0
			|| addressLength > MAX_SOCKET_ADDRESS_LEN) {
		return B_BAD_VALUE;
	}

	// copy address from userland
	char address[MAX_SOCKET_ADDRESS_LEN];
	if (!IS_USER_ADDRESS(userAddress)
			|| user_memcpy(address, userAddress, addressLength) != B_OK) {
		return B_BAD_ADDRESS;
	}

	// sendto()
	SyscallRestartWrapper<ssize_t> result;

	return result = common_sendto(socket, data, length, flags,
		(sockaddr*)address, addressLength, false);
}


ssize_t
_user_sendmsg(int socket, const struct msghdr *userMessage, int flags)
{
	// copy message from userland
	msghdr message;
	iovec* userVecs = message.msg_iov;
	iovec vecs[MAX_IO_VEC_COUNT];
	void* userAddress = message.msg_name;
	char address[MAX_SOCKET_ADDRESS_LEN];

	status_t error = prepare_userland_msghdr(userMessage, message, userVecs,
		vecs, userAddress, address);
	if (error != B_OK)
		return error;

	// copy the address from userland
	if (userAddress != NULL
			&& user_memcpy(address, userAddress, message.msg_namelen) != B_OK) {
		return B_BAD_ADDRESS;
	}

	// sendmsg()
	SyscallRestartWrapper<ssize_t> result;

	return result = common_sendmsg(socket, &message, flags, false);
}


status_t
_user_getsockopt(int socket, int level, int option, void *userValue,
	socklen_t *_length)
{
	// check params
	if (userValue == NULL || _length == NULL)
		return B_BAD_VALUE;
	if (!IS_USER_ADDRESS(userValue) || !IS_USER_ADDRESS(_length))
		return B_BAD_ADDRESS;

	// copy length from userland
	socklen_t length;
	if (user_memcpy(&length, _length, sizeof(socklen_t)) != B_OK)
		return B_BAD_ADDRESS;

	if (length > MAX_SOCKET_OPTION_LEN)
		return B_BAD_VALUE;

	// getsockopt()
	char value[MAX_SOCKET_OPTION_LEN];
	SyscallRestartWrapper<status_t> error;
	error = common_getsockopt(socket, level, option, value, &length,
		false);
	if (error != (status_t)B_OK)
		return error;

	// copy value back to userland
	if (user_memcpy(userValue, value, length) != B_OK)
		return B_BAD_ADDRESS;

	return B_OK;
}


status_t
_user_setsockopt(int socket, int level, int option, const void *userValue,
	socklen_t length)
{
	// check params
	if (userValue == NULL || length > MAX_SOCKET_OPTION_LEN)
		return B_BAD_VALUE;

	// copy value from userland
	char value[MAX_SOCKET_OPTION_LEN];
	if (!IS_USER_ADDRESS(userValue)
			|| user_memcpy(value, userValue, length) != B_OK) {
		return B_BAD_ADDRESS;
	}

	// setsockopt();
	SyscallRestartWrapper<status_t> error;
	return error = common_setsockopt(socket, level, option, value, length,
		false);
}


status_t
_user_getpeername(int socket, struct sockaddr *userAddress,
	socklen_t *_addressLength)
{
	// check parameters
	socklen_t addressLength = 0;
	SyscallRestartWrapper<status_t> error;
	error = prepare_userland_address_result(userAddress, _addressLength,
		addressLength, true);
	if (error != (status_t)B_OK)
		return error;
	
	// getpeername()
	char address[MAX_SOCKET_ADDRESS_LEN];
	error = common_getpeername(socket, (sockaddr*)address, &addressLength,
		false);
	if (error != (status_t)B_OK)
		return error;

	// copy address size and address back to userland
	if (user_memcpy(_addressLength, &addressLength,
			sizeof(socklen_t)) != B_OK
		|| userAddress != NULL
			&& user_memcpy(userAddress, address, addressLength) != B_OK) {
		return B_BAD_ADDRESS;
	}

	return B_OK;
}


status_t
_user_getsockname(int socket, struct sockaddr *userAddress,
	socklen_t *_addressLength)
{
	// check parameters
	socklen_t addressLength = 0;
	SyscallRestartWrapper<status_t> error;
	error = prepare_userland_address_result(userAddress, _addressLength,
		addressLength, true);
	if (error != (status_t)B_OK)
		return error;
	
	// getsocknam()
	char address[MAX_SOCKET_ADDRESS_LEN];
	error = common_getsockname(socket, (sockaddr*)address, &addressLength,
		false);
	if (error != (status_t)B_OK)
		return error;

	// copy address size and address back to userland
	if (user_memcpy(_addressLength, &addressLength,
			sizeof(socklen_t)) != B_OK
		|| userAddress != NULL
			&& user_memcpy(userAddress, address, addressLength) != B_OK) {
		return B_BAD_ADDRESS;
	}

	return B_OK;
}


int
_user_sockatmark(int socket)
{
	SyscallRestartWrapper<status_t> error;
	return error = common_sockatmark(socket, false);
}


status_t
_user_socketpair(int family, int type, int protocol, int *userSocketVector)
{
	// check parameters
	if (userSocketVector == NULL)
		return B_BAD_VALUE;
	if (!IS_USER_ADDRESS(userSocketVector))
		return B_BAD_ADDRESS;

	// socketpair()
	int socketVector[2];
	SyscallRestartWrapper<status_t> error;
	error = common_socketpair(family, type, protocol, socketVector, false);
	if (error != (status_t)B_OK)
		return error;

	// copy FDs back to userland
	if (user_memcpy(userSocketVector, socketVector,
			sizeof(socketVector)) != B_OK) {
		_user_close(socketVector[0]);
		_user_close(socketVector[1]);
		return B_BAD_ADDRESS;
	}

	return B_OK;
}


status_t
_user_get_next_socket_stat(int family, uint32 *_cookie, struct net_stat *_stat)
{
	// check parameters and copy cookie from userland
	if (_cookie == NULL || _stat == NULL)
		return B_BAD_VALUE;

	uint32 cookie;
	if (!IS_USER_ADDRESS(_stat) || !IS_USER_ADDRESS(_cookie)
			|| user_memcpy(&cookie, _cookie, sizeof(cookie)) != B_OK) {
		return B_BAD_ADDRESS;
	}

	net_stat stat;
	SyscallRestartWrapper<status_t> error;
	error = common_get_next_socket_stat(family, &cookie, &stat);
	if (error != (status_t)B_OK)
		return error;

	// copy cookie and data back to userland
	if (user_memcpy(_cookie, &cookie, sizeof(cookie)) != B_OK
		|| user_memcpy(_stat, &stat, sizeof(net_stat)) != B_OK) {
		return B_BAD_ADDRESS;
	}

	return B_OK;
}
