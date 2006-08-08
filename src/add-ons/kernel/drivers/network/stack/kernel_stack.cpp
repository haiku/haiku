/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * This file may be used under the terms of the MIT License.
 */

/*!
	This file implements a very simple socket driver that is intended to
	act as an interface to the networking stack.
*/

#include <net_socket.h>
#include <net_stack_driver.h>

#include <Drivers.h>
#include <KernelExport.h>
#include <driver_settings.h>

#include <sys/ioctl.h>
#include <stdlib.h>
#include <string.h>


// debugging macros
#define LOGID					"net_stack_driver: "
#define ERROR(format, args...)	dprintf(LOGID "ERROR: " format, ## args)
#define WARN(format, args...)	dprintf(LOGID "WARNING: " format, ## args)

#ifdef DEBUG
#	define TRACE(format, args...)	dprintf(format, ## args)
#else
#	define TRACE(format, args...)
#endif


// Local definitions

// this struct will store one select() event to monitor per thread
typedef struct selecter {
	struct selecter *next;
	thread_id thread;
	uint32 event;
	selectsync *sync;
	uint32 ref;
} selecter;

// the cookie we attach to each file descriptor opened on our driver entry
typedef struct net_stack_cookie {
	net_socket *socket;		// NULL before ioctl(fd, NET_STACK_SOCKET/_ACCEPT)
	sem_id selecters_lock;	// protect the selecters linked-list
	selecter *selecters;	// the select()'ers lists (thread-aware)
} net_stack_cookie;

// TODO: disabled
/* To unload this driver execute
 * $ echo unload > /dev/net/stack
 * As soon as the last app stops using sockets the netstack will be unloaded.
 */
//static const char *kUnloadCommand = "unload";


// prototypes of device hooks functions
static status_t net_stack_open(const char *name, uint32 flags, void **_cookie);
static status_t net_stack_close(void *cookie);
static status_t net_stack_free_cookie(void *cookie);
static status_t net_stack_control(void *cookie, uint32 op, void *data, size_t length);
static status_t net_stack_read(void *cookie, off_t pos, void *data, size_t *_length);
static status_t net_stack_write(void *cookie, off_t pos, const void *data, size_t *_length);
static status_t net_stack_select(void *cookie, uint8 event, uint32 ref, selectsync *sync);
static status_t net_stack_deselect(void *cookie, uint8 event, selectsync *sync);
static status_t net_stack_readv(void *cookie, off_t pos, const iovec *vecs, size_t count, size_t *_length);
static status_t net_stack_writev(void *cookie, off_t pos, const iovec *vecs, size_t count, size_t *_length);


int32 api_version = B_CUR_DRIVER_API_VERSION;

static int32 sOpenCount = 0;
static struct net_socket_module_info *sSocket = NULL;


//	#pragma mark - internal functions


template<typename ArgType> status_t
return_address(ArgType &args, void *data)
{
	sockaddr_storage *target;

	if (user_memcpy(&target, &((ArgType *)data)->address, sizeof(void *)) < B_OK
		|| user_memcpy(&((ArgType *)data)->address_length, &args.address_length, sizeof(socklen_t)) < B_OK
		|| user_memcpy(target, args.address, args.address_length) < B_OK)
		return B_BAD_ADDRESS;

	return B_OK;
}


template<typename ArgType> status_t
check_args_and_address(ArgType &args, sockaddr_storage &address, void *data, size_t length,
	bool copyAddress = true)
{
	if (data == NULL || length != sizeof(ArgType))
		return B_BAD_VALUE;

	if (user_memcpy(&args, data, sizeof(ArgType)) < B_OK)
		return B_BAD_ADDRESS;

	if (args.address_length > sizeof(sockaddr_storage))
		return B_BAD_VALUE;

	if (copyAddress && user_memcpy(&address, args.address, args.address_length) < B_OK)
		return B_BAD_ADDRESS;

	args.address = (sockaddr *)&address;
	return B_OK;
}


template<typename ArgType> status_t
check_args(ArgType &args, void *data, size_t length)
{
	if (data == NULL || length != sizeof(ArgType))
		return B_BAD_VALUE;

	if (user_memcpy(&args, data, sizeof(ArgType)) < B_OK)
		return B_BAD_ADDRESS;

	return B_OK;
}


#if 0
static void
on_socket_event(void * socket, uint32 event, void *_cookie)
{
	net_stack_cookie *cookie = (net_stack_cookie *) _cookie;
	selecter *s;
	
	if (!cookie)
		return;
	
	if (cookie->socket != socket) {
		ERROR("on_socket_event(%p, %ld, %p): socket is higly suspect! Aborting.\n",
			socket, event, cookie);
		return;
	}

	TRACE("on_socket_event(%p, %ld, %p)\n", socket, event, cookie);

	// lock the selecters list
	if (acquire_sem(cookie->selecters_lock) != B_OK)
		return;

	s = cookie->selecters;
	while (s) {
		if (s->event == event)
			// notify this selecter (thread/event pair)
#ifdef COMPILE_FOR_R5
			notify_select_event(s->sync, s->ref);
#else
			notify_select_event(s->sync, s->ref, event);
#endif
		
		s = s->next;
	}

	// unlock the selecters list
	release_sem(cookie->selecters_lock);
	return;
}
#endif


#ifdef DEBUG
static const char*
opcode_name(int op)
{
#define C2N(op) { op, #op }
	// op-code to name
	struct commands_info {
		int op;
		const char *name;
	} *ci, commands_info[] = {
		C2N(NET_STACK_SOCKET),
		C2N(NET_STACK_BIND),
		C2N(NET_STACK_RECVFROM),
		C2N(NET_STACK_RECV),
		C2N(NET_STACK_SENDTO),
		C2N(NET_STACK_SEND),
		C2N(NET_STACK_LISTEN),
		C2N(NET_STACK_ACCEPT),
		C2N(NET_STACK_CONNECT),
		C2N(NET_STACK_SHUTDOWN),
		C2N(NET_STACK_GETSOCKOPT),
		C2N(NET_STACK_SETSOCKOPT),
		C2N(NET_STACK_GETSOCKNAME),
		C2N(NET_STACK_GETPEERNAME),
		C2N(NET_STACK_SYSCTL),
		C2N(NET_STACK_SELECT),
		C2N(NET_STACK_DESELECT),
		C2N(NET_STACK_GET_COOKIE),
		C2N(NET_STACK_NOTIFY_SOCKET_EVENT),
		C2N(NET_STACK_CONTROL_NET_MODULE),

		// Userland IPC-specific opcodes
		// C2N(NET_STACK_OPEN),
		// C2N(NET_STACK_CLOSE),
		// C2N(NET_STACK_NEW_CONNECTION),

		// Standard BeOS opcodes
		C2N(B_SET_BLOCKING_IO),
		C2N(B_SET_NONBLOCKING_IO),

		{ 0, "Unknown!" }
	};
#undef C2N

	ci = commands_info;
	while(ci && ci->op) {
		if (ci->op == op)
			return ci->name;
		ci++;
	}

	return "???";
}
#endif	// DEBUG


//	#pragma mark - driver


/**	Do we init ourselves? If we're in safe mode we'll decline so if things
 *	screw up we can boot and delete the broken driver!
 *	After my experiences earlier - a good idea!
 */

status_t
init_hardware(void)
{
	void *settings = load_driver_settings(B_SAFEMODE_DRIVER_SETTINGS);
	if (settings != NULL) {
		// we got a pointer, now get settings...
		bool safemode = get_driver_boolean_parameter(settings, B_SAFEMODE_SAFE_MODE, false, false);
		// now get rid of settings
		unload_driver_settings(settings);

		if (safemode) {
			WARN("init_hardware: declining offer to join the party.\n");
			return B_ERROR;
		}
	}

	TRACE("init_hardware done.\n");
	return B_OK;
}


status_t
init_driver(void)
{

	return B_OK;
}


void
uninit_driver(void)
{
	TRACE("uninit_driver\n");

	put_module(NET_SOCKET_MODULE_NAME);
}


const char **
publish_devices(void)
{
	static const char *devices[] = { NET_STACK_DRIVER_DEV, NULL };

	return devices;
}


device_hooks*
find_device(const char* device_name)
{
	static device_hooks hooks = {
		net_stack_open,
		net_stack_close,
		net_stack_free_cookie,
		net_stack_control,
		net_stack_read,
		net_stack_write,
		net_stack_select,
		net_stack_deselect,
		net_stack_readv,
		net_stack_writev,
	};

	return &hooks;
}


//	#pragma mark - device


static status_t
net_stack_open(const char *name, uint32 flags, void **_cookie)
{
	if (atomic_add(&sOpenCount, 1) == 0) {
		// When we're opened for the first time, we'll try to load the
		// networking stack
		status_t status = get_module(NET_SOCKET_MODULE_NAME, (module_info **)&sSocket);
		if (status < B_OK) {
			ERROR("Can't load " NET_SOCKET_MODULE_NAME " module: %ld\n", status);
			return status;
		}
	}

	net_stack_cookie *cookie = (net_stack_cookie *)malloc(sizeof(net_stack_cookie));
	if (cookie == NULL)
		return B_NO_MEMORY;

	memset(cookie, 0, sizeof(net_stack_cookie));
	cookie->socket = NULL; // the socket will be allocated in NET_STACK_SOCKET ioctl
	cookie->selecters_lock = create_sem(1, "socket_selecters_lock");

	// attach this new net_socket_cookie to file descriptor
	*_cookie = cookie;

	TRACE("net_stack_open(%s, %s%s) return this cookie: %p\n",
		name,
		(((flags & O_RWMASK) == O_RDONLY) ? "O_RDONLY" :
			((flags & O_RWMASK) == O_WRONLY) ? "O_WRONLY" : "O_RDWR"),
		(flags & O_NONBLOCK) ? " O_NONBLOCK" : "",
		cookie);

	return B_OK;
}


static status_t
net_stack_close(void *_cookie)
{
	net_stack_cookie *cookie = (net_stack_cookie *)_cookie;

	TRACE("net_stack_close(%p)\n", cookie);

	if (cookie->socket == NULL)
		return B_OK;

	return sSocket->close(cookie->socket);
}


static status_t
net_stack_free_cookie(void *_cookie)
{
	net_stack_cookie *cookie = (net_stack_cookie *)_cookie;
	selecter *s;
	
	TRACE("net_stack_free_cookie(%p)\n", cookie);
	
	// free the selecters list
	delete_sem(cookie->selecters_lock);
	s = cookie->selecters;
	while (s) {
		selecter *tmp = s;
		s = s->next;
		free(tmp);
	}

	if (cookie->socket != NULL)
		sSocket->free(cookie->socket);

	// free the cookie
	free(cookie);

	if (atomic_add(&sOpenCount, -1) == 1) {
		// the last reference to us has been removed, unload the networking
		// stack again
		put_module(NET_SOCKET_MODULE_NAME);
	}

	return B_OK;
}


static status_t
net_stack_control(void *_cookie, uint32 op, void *data, size_t length)
{
	net_stack_cookie *cookie = (net_stack_cookie *)_cookie;
	status_t status = B_BAD_VALUE;

	TRACE("net_stack_control(%p, %s (0x%lX), %p, %ld)\n",
		_cookie, opcode_name(op), op, data, len);

	if (op < NET_STACK_IOCTL_BASE || op > NET_STACK_IOCTL_END) {
		// we only accept networking ioctl()s to get into our stack
		return B_BAD_VALUE;
	}

	if (cookie->socket == NULL) {
		switch (op) {
			case NET_STACK_SOCKET:
			{
				socket_args args;
				status = check_args(args, data, length);
				if (status < B_OK)
					return status;

				return sSocket->socket(args.family, args.type, args.protocol, &cookie->socket);
			}

			case NET_STACK_GET_COOKIE:
				// This is needed by accept() call, allowing libnet.so accept() to pass back
				// in NET_STACK_ACCEPT opcode the cookie (aka the net_stack_cookie!)
				// of the file descriptor to use for the new accepted socket
				return user_memcpy(data, cookie, sizeof(void *));
		}
	} else {
		switch (op) {
			case NET_STACK_CONNECT:
			{
				sockaddr_storage address;
				sockaddr_args args;
				status = check_args_and_address(args, address, data, length);
				if (status < B_OK)
					return status;

				return sSocket->connect(cookie->socket, args.address, args.address_length);
			}

			case NET_STACK_BIND:
			{
				sockaddr_storage address;
				sockaddr_args args;
				status = check_args_and_address(args, address, data, length);
				if (status < B_OK)
					return status;

				return sSocket->bind(cookie->socket, args.address, args.address_length);
			}

			case NET_STACK_LISTEN:
				// backlog to set
				return sSocket->listen(cookie->socket, (int)data);

			case NET_STACK_ACCEPT:
			{
				sockaddr_storage address;
				accept_args args;
				status = check_args_and_address(args, address, data, length, false);
				if (status < B_OK)
					return status;

				// args.cookie is the net_stack_cookie of the already opened socket
				// TODO: find a safer way to do this!

				net_stack_cookie *acceptCookie = (net_stack_cookie *)args.cookie;
				status = sSocket->accept(cookie->socket, args.address,
					&args.address_length, &acceptCookie->socket);
				if (status < B_OK)
					return status;

				return return_address(args, data);
			}

			case NET_STACK_SHUTDOWN:
				return sSocket->shutdown(cookie->socket, (int)data);

			case NET_STACK_SEND:
			{
				transfer_args args;
				status = check_args(args, data, length);
				if (status < B_OK)
					return status;

				return sSocket->send(cookie->socket, args.data, args.data_length, args.flags);
			}
			case NET_STACK_SENDTO:
			{
				sockaddr_storage address;
				transfer_args args;
				status = check_args_and_address(args, address, data, length);
				if (status < B_OK)
					return status;

				return sSocket->sendto(cookie->socket, args.data, args.data_length,
					args.flags, args.address, args.address_length);
			}

			case NET_STACK_RECV:
			{
				transfer_args args;
				status = check_args(args, data, length);
				if (status < B_OK)
					return status;

				return sSocket->recv(cookie->socket, args.data, args.data_length, args.flags);
			}
			case NET_STACK_RECVFROM:
			{
				sockaddr_storage address;
				transfer_args args;
				status = check_args_and_address(args, address, data, length, false);
				if (status < B_OK)
					return status;

				ssize_t bytesRead = sSocket->recvfrom(cookie->socket, args.data,
					args.data_length, args.flags, args.address, &args.address_length);
				if (bytesRead < B_OK)
					return bytesRead;

				status = return_address(args, data);
				if (status < B_OK)
					return status;

				return bytesRead;
			}

			case NET_STACK_GETSOCKOPT:
			{
				sockopt_args args;
				status = check_args(args, data, length);
				if (status < B_OK)
					return status;

				char valueBuffer[128];
				if (args.length > (int)sizeof(valueBuffer))
					args.length = (int)sizeof(valueBuffer);

				status = sSocket->getsockopt(cookie->socket, args.level, args.option,
					valueBuffer, &args.length);
				if (status < B_OK)
					return status;

				if (user_memcpy(args.value, valueBuffer, args.length) < B_OK
					|| user_memcpy(&((sockopt_args *)data)->length, &args.length, sizeof(int)) < B_OK)
					return B_BAD_ADDRESS;

				return B_OK;
			}

			case NET_STACK_SETSOCKOPT:
			{
				sockopt_args args;
				status = check_args(args, data, length);
				if (status < B_OK)
					return status;

				char valueBuffer[128];
				if (args.length > (int)sizeof(valueBuffer))
					return B_BAD_VALUE;
				if (user_memcpy(valueBuffer, args.value, args.length) < B_OK)
					return B_BAD_ADDRESS;

				return sSocket->setsockopt(cookie->socket, args.level, args.option,
					valueBuffer, args.length);
			}

			case NET_STACK_GETSOCKNAME:
			{
				sockaddr_storage address;
				sockaddr_args args;
				status = check_args_and_address(args, address, data, length, false);
				if (status < B_OK)
					return status;

				status = sSocket->getsockname(cookie->socket, args.address,
					&args.address_length);
				if (status < B_OK)
					return status;

				return return_address(args, data);
			}

			case NET_STACK_GETPEERNAME:
			{
				sockaddr_storage address;
				sockaddr_args args;
				status = check_args_and_address(args, address, data, length, false);
				if (status < B_OK)
					return status;

				status = sSocket->getpeername(cookie->socket, args.address,
					&args.address_length);
				if (status < B_OK)
					return status;

				return return_address(args, data);
			}

			case FIONBIO:
			{
				int value = (int)data;
				return sSocket->setsockopt(cookie->socket, SOL_SOCKET, SO_NONBLOCK,
					&value, sizeof(int));
			}

			case B_SET_BLOCKING_IO:
			case B_SET_NONBLOCKING_IO:
			{
				int value = op == B_SET_NONBLOCKING_IO;
				return sSocket->setsockopt(cookie->socket, SOL_SOCKET, SO_NONBLOCK,
					&value, sizeof(int));
			}

			default:
				return sSocket->control(cookie->socket, op, data, length);
		}
	}

	return status;
}


static status_t
net_stack_read(void *_cookie, off_t /*offset*/, void *buffer, size_t *_length)
{
	net_stack_cookie *cookie = (net_stack_cookie *)_cookie;

	TRACE("net_stack_read(%p, %p, %ld)\n", cookie, buffer, *_length);

	if (cookie->socket == NULL)
		return B_BAD_VALUE;

	ssize_t bytesRead = sSocket->recv(cookie->socket, buffer, *_length, 0);
	if (bytesRead < 0) {
		*_length = 0;
		return bytesRead;
	}

	*_length = bytesRead;
	return B_OK;
}


static status_t
net_stack_readv(void *_cookie, off_t /*offset*/, const struct iovec *vecs,
	size_t count, size_t *_length)
{
	net_stack_cookie *cookie = (net_stack_cookie *)_cookie;

	TRACE("net_stack_readv(%p, (%p, %lu), %ld)\n", cookie, vecs, count, *_length);

	if (cookie->socket == NULL)
		return B_BAD_VALUE;

	return sSocket->readv(cookie->socket, vecs, count, _length);
}


static status_t
net_stack_write(void *_cookie, off_t /*offset*/, const void *buffer, size_t *_length)
{
	net_stack_cookie *cookie = (net_stack_cookie *)_cookie;

	TRACE("net_stack_write(%p, %p, %ld)\n", cookie, buffer, *_length);

	if (cookie->socket == NULL)
		return B_BAD_VALUE;

	ssize_t bytesWritten = sSocket->send(cookie->socket, buffer, *_length, 0);
	if (bytesWritten < 0) {
		*_length = 0;
		return bytesWritten;
	}

	*_length = bytesWritten;
	return B_OK;
}


static status_t
net_stack_writev(void *_cookie, off_t /*offset*/, const struct iovec *vecs,
	size_t count, size_t *_length)
{
	net_stack_cookie *cookie = (net_stack_cookie *)_cookie;

	TRACE("net_stack_writev(%p, (%p, %lu), %ld)\n", cookie, vecs, count, *_length);

	if (cookie->socket == NULL)
		return B_BAD_VALUE;

	return sSocket->writev(cookie->socket, vecs, count, _length);
}


static status_t
net_stack_select(void *_cookie, uint8 event, uint32 ref, selectsync *sync)
{
	net_stack_cookie *cookie = (net_stack_cookie *)_cookie;
	status_t status;

	TRACE("net_stack_select(%p, %d, %ld, %p)\n", cookie, event, ref, sync);

	if (cookie->socket == NULL)
		return B_BAD_VALUE;

	selecter *s = (selecter *)malloc(sizeof(selecter));
	if (s == NULL)
		return B_NO_MEMORY;

	s->thread = find_thread(NULL);
	s->event = event;
	s->sync = sync;
	s->ref = ref;

	// lock the selecters list
	status = acquire_sem(cookie->selecters_lock);
	if (status != B_OK) {
		free(s);
		return status;
	}

	// add it to selecters list
	s->next = cookie->selecters;
	cookie->selecters = s;

	// unlock the selecters list
	release_sem(cookie->selecters_lock);

	// start (or continue) to monitor for socket event
	// TODO: enable select() notification
//	return sSocket->socket_set_event_callback(cookie->socket, on_socket_event, cookie, event);
	return B_OK;
}


static status_t
net_stack_deselect(void *_cookie, uint8 event, selectsync *sync)
{
	net_stack_cookie *cookie = (net_stack_cookie *)_cookie;

	TRACE("net_stack_deselect(%p, %d, %p)\n", cookie, event, sync);

	if (!cookie || !cookie->socket)
		return B_BAD_VALUE;

	// lock the selecters list
	status_t status = acquire_sem(cookie->selecters_lock);
	if (status != B_OK)
		return status;

	thread_id currentThread = find_thread(NULL);;
	selecter *previous = NULL;
	selecter *s = cookie->selecters;

	while (s) {
		if (s->thread == currentThread
			&& s->event == event && s->sync == sync) {
			// selecter found!
			break;
		}

		previous = s;
		s = s->next;
	}

	if (s != NULL) {
		// remove it from selecters list
		if (previous)
			previous->next = s->next;
		else
			cookie->selecters = s->next;
		free(s);
	}

	// TODO: enable deselect()
	if (cookie->selecters == NULL) {
		// selecters list is empty: no need to monitor socket events anymore
		//sSocket->socket_set_event_callback(cookie->socket, NULL, NULL, event);
	}

	// unlock the selecters list
	return release_sem(cookie->selecters_lock);
}
