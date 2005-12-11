/* net_stack_driver.c
 *
 * This file implements a very simple socket driver that is intended to
 * act as an interface to the networking stack.
 */

#ifndef _KERNEL_MODE
	#error "This module MUST be built as a kernel driver!"
#endif

// public/system includes
#include <stdlib.h>
#include <string.h>

#include <drivers/Drivers.h>
#include <drivers/KernelExport.h>
#include <drivers/driver_settings.h>
#include <drivers/module.h>		// for get_module()/put_module()

#include <netinet/in_var.h>

// private includes
#include <net_stack_driver.h>
#include <sys/protosw.h>
#include <core_module.h>


// debugging macros
#define LOGID					"net_stack_driver: "
#define ERROR(format, args...)	dprintf(LOGID "ERROR: " format, ## args)
#define WARN(format, args...)	dprintf(LOGID "WARNING: " format, ## args)
#ifdef DEBUG
#define TRACE(format, args...)	dprintf(format, ## args)
#else
#define TRACE(format, args...)
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
	struct socket *socket;	// NULL before ioctl(fd, NET_STACK_SOCKET/_ACCEPT)
	uint32 open_flags;		// the open() flags (mostly for storing O_NONBLOCK mode)
	sem_id selecters_lock;	// protect the selecters linked-list
	selecter *selecters;	// the select()'ers lists (thread-aware)
} net_stack_cookie;

/* To unload this driver execute
 * $ echo unload > /dev/net/stack
 * As soon as the last app stops using sockets the netstack will be unloaded.
 */
static const char *kUnloadCommand = "unload";


// prototypes of device hooks functions
static status_t net_stack_open(const char *name, uint32 flags, void **cookie);
static status_t net_stack_close(void *cookie);
static status_t net_stack_free_cookie(void *cookie);
static status_t net_stack_control(void *cookie, uint32 msg, void *data, size_t datalen);
static status_t net_stack_read(void *cookie, off_t pos, void *data, size_t *datalen);
static status_t net_stack_write(void *cookie, off_t pos, const void *data, size_t *datalen);
static status_t net_stack_select(void *cookie, uint8 event, uint32 ref, selectsync *sync);
static status_t net_stack_deselect(void *cookie, uint8 event, selectsync *sync);
// static status_t net_stack_readv(void *cookie, off_t pos, const iovec *vec, size_t count, size_t *len);
// static status_t net_stack_writev(void *cookie, off_t pos, const iovec *vec, size_t count, size_t *len);

// privates prototypes
static void on_socket_event(void *socket, uint32 event, void *cookie);

static status_t	keep_driver_loaded();
static status_t	unload_driver();

static const char *opcode_name(int op);

/*
 * Global variables
 * ----------------
 */

static const char *sDevices[] = { NET_STACK_DRIVER_DEV, NULL };

static device_hooks sDeviceHooks =
{
	net_stack_open,			/* -> open entry point */
	net_stack_close,		/* -> close entry point */
	net_stack_free_cookie,	/* -> free entry point */
	net_stack_control,		/* -> control entry point */
	net_stack_read,			/* -> read entry point */
	net_stack_write,		/* -> write entry point */
	net_stack_select,		/* -> select entry point */
	net_stack_deselect,		/* -> deselect entry point */
	NULL,					/* -> readv entry pint */
	NULL					/* ->writev entry point */
};

static struct core_module_info *sCore = NULL;

static int sStayLoadedFD = -1;


// internal functions
static status_t
keep_driver_loaded()
{
	if (sStayLoadedFD != -1)
		return B_OK;
	
	/* force the driver to stay loaded by opening himself */
	TRACE("keep_driver_loaded: opening " NET_STACK_DRIVER_PATH " to stay loaded...\n");
	
	sStayLoadedFD = open(NET_STACK_DRIVER_PATH, 0);
	
	if (sStayLoadedFD < 0)
		ERROR("keep_driver_loaded: couldn't open(" NET_STACK_DRIVER_PATH ")!\n");
	
	return B_OK;
}


static status_t
unload_driver()
{
	if (sStayLoadedFD >= 0) {
		int tmp_fd;
		
		/* we need to set sStayLoadedFD to < 0 if we don't want
		 * the next close enter again in this case, and so on...
		 */
		TRACE("unload_driver: unload requested.\n");
		
		tmp_fd = sStayLoadedFD;
		sStayLoadedFD = -1;
		
		close(tmp_fd);
	}
	
	return B_OK;
}


/*
 * Driver API calls
 * ----------------
 */

_EXPORT int32 api_version = B_CUR_DRIVER_API_VERSION;



/* Do we init ourselves? If we're in safe mode we'll decline so if things
 * screw up we can boot and delete the broken driver!
 * After my experiences earlier - a good idea!
 *
 * Also we'll turn on the serial debugger to aid in our debugging
 * experience.
 */
_EXPORT status_t
init_hardware(void)
{
	bool safemode = false;
	void *sfmptr;
	
	// get a pointer to the driver settings...
	sfmptr = load_driver_settings(B_SAFEMODE_DRIVER_SETTINGS);
	
	// only use the pointer if it's valid
	if (sfmptr != NULL) {
		// we got a pointer, now get settings...
		safemode = get_driver_boolean_parameter(sfmptr, B_SAFEMODE_SAFE_MODE, false, false);
		// now get rid of settings
		unload_driver_settings(sfmptr);
	}
	
	if (safemode) {
		WARN("init_hardware: declining offer to join the party.\n");
		return B_ERROR;
	}
	
	TRACE("init_hardware done.\n");
	
	return B_OK;
}


/* init_driver()
 * called every time we're loaded.
 */
_EXPORT status_t
init_driver(void)
{
	// only get the core module if we don't have it loaded already
	if (!sCore) {
		int rv = 0;
		rv = get_module(NET_CORE_MODULE_NAME, (module_info **) &sCore);
		if (rv < 0) {
			ERROR("init_driver: Can't load " NET_CORE_MODULE_NAME " module: %d\n", rv);
			return rv;
		}
		
		TRACE("init_driver: built %s %s, core = %p\n", __DATE__, __TIME__, sCore);
		
		// start the network stack
		sCore->start();
	}
	
	keep_driver_loaded();
	
	return B_OK;
}


/* uninit_driver()
 * called every time the driver is unloaded
 */
_EXPORT void
uninit_driver(void)
{
	TRACE("uninit_driver\n");
	
	if (sCore) {
		// shutdown the network stack
		sCore->stop();
		
		put_module(NET_CORE_MODULE_NAME);
		sCore = NULL;
	}
}


/* publish_devices()
 * called to publish our device.
 */
_EXPORT const char**
publish_devices()
{
	return sDevices;
}


_EXPORT device_hooks*
find_device(const char* device_name)
{
	return &sDeviceHooks;
}


/*
 * Device hooks
 * ------------
 */

// the network stack functions - mainly just pass throughs...
static status_t
net_stack_open(const char *name, uint32 flags, void **cookie)
{
	net_stack_cookie *nsc;
	
	nsc = (net_stack_cookie *) malloc(sizeof(*nsc));
	if (!nsc)
		return B_NO_MEMORY;
	
	memset(nsc, 0, sizeof(*nsc));
	nsc->socket = NULL; // the socket will be allocated in NET_STACK_SOCKET ioctl
	nsc->open_flags = flags;
	nsc->selecters_lock = create_sem(1, "socket_selecters_lock");
	nsc->selecters = NULL;
	
	// attach this new net_socket_cookie to file descriptor
	*cookie = nsc;
	
	TRACE("net_stack_open(%s, %s%s) return this cookie: %p\n",
		name,
		(((flags & O_RWMASK) == O_RDONLY) ? "O_RDONLY" :
			((flags & O_RWMASK) == O_WRONLY) ? "O_WRONLY" : "O_RDWR"),
		(flags & O_NONBLOCK) ? " O_NONBLOCK" : "",
		*cookie);
	
	return B_OK;
}


static status_t
net_stack_close(void *cookie)
{
	net_stack_cookie *nsc = cookie;
	int rv;
	
	TRACE("net_stack_close(%p)\n", nsc);
	
	rv = B_ERROR;
	if (nsc->socket) {
		// if a socket was opened on this fd, close it now.
		rv = sCore->socket_close(nsc->socket);
		nsc->socket = NULL;
	}
	
	return rv;
}


static status_t
net_stack_free_cookie(void *cookie)
{
	net_stack_cookie *nsc = cookie;
	selecter *s;
	
	TRACE("net_stack_free_cookie(%p)\n", cookie);
	
	// free the selecters list
	delete_sem(nsc->selecters_lock);
	s = nsc->selecters;
	while (s) {
		selecter *tmp = s;
		s = s->next;
		free(tmp);
	}
	
	// free the cookie
	free(cookie);
	return B_OK;
}


static status_t
net_stack_control(void *cookie, uint32 op, void *data, size_t len)
{
	net_stack_cookie *nsc = cookie;
	struct stack_driver_args *args = data;
	int err = B_BAD_VALUE;
	
	TRACE("net_stack_control(%p, %s (0x%lX), %p, %ld)\n",
		cookie, opcode_name(op), op, data, len);
	
	if (!nsc->socket) {
		switch (op) {
			case NET_STACK_SOCKET: {
				// okay, now try to open a real socket behind this fd/net_stack_cookie pair
				err = sCore->socket_init(&nsc->socket);
				if (err == 0)
					err = sCore->socket_create(nsc->socket, args->u.socket.family,
						args->u.socket.type, args->u.socket.proto);
				
				// TODO: handle some more open_flags
				if(nsc->open_flags & O_NONBLOCK) {
					int on = true;
					return sCore->socket_ioctl(nsc->socket, FIONBIO, (caddr_t) &on);
				}
				return err;
			}
			case NET_STACK_GET_COOKIE: {
				/* this is needed by accept() call, allowing libnet.so accept() to pass back
				 * in NET_STACK_ACCEPT opcode the cookie (aka the net_stack_cookie!)
				 * of the filedescriptor to use for the new accepted socket
				 */
				
				*((void **) data) = cookie;
				return B_OK;
			}
			case NET_STACK_CONTROL_NET_MODULE: {
				return sCore->control_net_module(args->u.control.name,
					args->u.control.op, args->u.control.data, args->u.control.length);
			}
			case NET_STACK_SYSCTL:
				return sCore->net_sysctl(args->u.sysctl.name, args->u.sysctl.namelen,
					args->u.sysctl.oldp, args->u.sysctl.oldlenp,
					args->u.sysctl.newp, args->u.sysctl.newlen);
		}
	} else {
		switch (op) {
			case NET_STACK_CONNECT:
				return sCore->socket_connect(nsc->socket,
					(caddr_t) args->u.sockaddr.addr, args->u.sockaddr.addrlen);
			
			case NET_STACK_SHUTDOWN:
				return sCore->socket_shutdown(nsc->socket, args->u.integer);
			
			case NET_STACK_BIND:
				return sCore->socket_bind(nsc->socket, (caddr_t) args->u.sockaddr.addr,
					args->u.sockaddr.addrlen);
			
			case NET_STACK_LISTEN:
				// backlog to set
				return sCore->socket_listen(nsc->socket, args->u.integer);
			
			case NET_STACK_ACCEPT: {
				/* args->u.accept.cookie == net_stack_cookie of the already opened fd
				 * in libnet.so accept() call to bind to newly accepted socket
				 */
				
				net_stack_cookie *ansc = args->u.accept.cookie;
				return sCore->socket_accept(nsc->socket, &ansc->socket,
					(void *)args->u.accept.addr, &args->u.accept.addrlen);
			}
			case NET_STACK_SEND:
				// TODO: flags gets ignored here...
				return net_stack_write(cookie, 0, args->u.transfer.data,
					&args->u.transfer.datalen);
			
			case NET_STACK_RECV:
				// TODO: flags gets ignored here...
				return net_stack_read(cookie, 0, args->u.transfer.data,
					&args->u.transfer.datalen);
			
			case NET_STACK_RECVFROM: {
				struct msghdr * mh = (struct msghdr *) data;
				int retsize;
				
				err = sCore->socket_recv(nsc->socket, mh, (caddr_t)&mh->msg_namelen,
					&retsize);
				if (err == 0)
					return retsize;
				return err;
			}
			case NET_STACK_SENDTO: {
				struct msghdr * mh = (struct msghdr *) data;
				int retsize;
				
				err = sCore->socket_send(nsc->socket, mh, mh->msg_flags, &retsize);
				if (err == 0)
					return retsize;
				return err;
			}
			case NET_STACK_GETSOCKOPT:
				return sCore->socket_getsockopt(nsc->socket, args->u.sockopt.level,
					args->u.sockopt.option, args->u.sockopt.optval,
					(size_t *) &args->u.sockopt.optlen);
			
			case NET_STACK_SETSOCKOPT:
				return sCore->socket_setsockopt(nsc->socket, args->u.sockopt.level,
					args->u.sockopt.option, (const void *) args->u.sockopt.optval,
					args->u.sockopt.optlen);
			
			case NET_STACK_GETSOCKNAME:
				return sCore->socket_getsockname(nsc->socket, args->u.sockaddr.addr,
					&args->u.sockaddr.addrlen);
			
			case NET_STACK_GETPEERNAME:
				return sCore->socket_getpeername(nsc->socket, args->u.sockaddr.addr,
					&args->u.sockaddr.addrlen);
			
			case NET_STACK_SOCKETPAIR: {
				net_stack_cookie *ansc = args->u.socketpair.cookie;
				return sCore->socket_socketpair(nsc->socket, &ansc->socket);
			}
			case B_SET_BLOCKING_IO: {
				int off = false;
				nsc->open_flags &= ~O_NONBLOCK;
				return sCore->socket_ioctl(nsc->socket, FIONBIO, (caddr_t) &off);
			}
			
			case B_SET_NONBLOCKING_IO: {
				int on = true;
				nsc->open_flags |= O_NONBLOCK;
				return sCore->socket_ioctl(nsc->socket, FIONBIO, (caddr_t) &on);
			}
			
			default:
				return sCore->socket_ioctl(nsc->socket, op, data);
		}
	}
	
	return err;
}


static status_t
net_stack_read(void *cookie, off_t position, void *buffer, size_t *readlen)
{
	net_stack_cookie *nsc = (net_stack_cookie *) cookie;
	struct iovec iov;
	int error;
	int flags = 0;
	
	TRACE("net_stack_read(%p, %Ld, %p, %ld)\n", cookie, position, buffer, *readlen);
	
	if (!nsc->socket)
		return B_BAD_VALUE;
	
	iov.iov_base = buffer;
	iov.iov_len = *readlen;
	
	error = sCore->socket_readv(nsc->socket, &iov, &flags);
	*readlen = error;
	return error;
}


static status_t
net_stack_write(void *cookie, off_t position, const void *buffer, size_t *writelen)
{
	net_stack_cookie *nsc = (net_stack_cookie *) cookie;
	struct iovec iov;
	int error;
	int flags = 0;
	
	TRACE("net_stack_write(%p, %Ld, %p, %ld)\n", cookie, position, buffer, *writelen);
	
	if (! nsc->socket) {
		if (*writelen >= strlen(kUnloadCommand)
				&& strncmp(buffer, kUnloadCommand, strlen(kUnloadCommand)) == 0)
			return unload_driver();
				// we are told to unload this driver
		
		return B_BAD_VALUE;
	}
	
	iov.iov_base = (void*) buffer;
	iov.iov_len = *writelen;
	
	error = sCore->socket_writev(nsc->socket, &iov, flags);
	*writelen = error;
	return error;
}


static status_t
net_stack_select(void *cookie, uint8 event, uint32 ref, selectsync *sync)
{
	net_stack_cookie *	nsc = (net_stack_cookie *) cookie;
	selecter *s;
	status_t status;
	
	TRACE("net_stack_select(%p, %d, %ld, %p)\n", cookie, event, ref, sync);
	
	if (!nsc->socket)
		return B_BAD_VALUE;
	
	s = (selecter *) malloc(sizeof(selecter));
	if (! s)
		return B_NO_MEMORY;
	
	s->thread = find_thread(NULL);	// current thread id
	s->event = event;
	s->sync = sync;
	s->ref = ref;
	
	// lock the selecters list
	status = acquire_sem(nsc->selecters_lock);
	if (status != B_OK) {
		free(s);
		return status;
	}
	
	// add it to selecters list
	s->next = nsc->selecters;
	nsc->selecters = s;
	
	// unlock the selecters list
	release_sem(nsc->selecters_lock);
	
	// start (or continue) to monitor for socket event
	return sCore->socket_set_event_callback(nsc->socket, on_socket_event, nsc, event);
}


static status_t
net_stack_deselect(void *cookie, uint8 event, selectsync *sync)
{
	net_stack_cookie *nsc = (net_stack_cookie *) cookie;
	selecter *previous;
	selecter *s;
	thread_id current_thread;
	status_t status;
	
	TRACE("net_stack_deselect(%p, %d, %p)\n", cookie, event, sync);
	
	if (!nsc || !nsc->socket)
		return B_BAD_VALUE;
	
	current_thread = find_thread(NULL);
	
	// lock the selecters list
	status = acquire_sem(nsc->selecters_lock);
	if (status != B_OK)
		return status;
	
	previous = NULL;
	s = nsc->selecters;
	while (s) {
		if (s->thread == current_thread &&
			s->event == event && s->sync == sync)
			// selecter found!
			break;
		
		previous = s;
		s = s->next;
	}
	
	if (s != NULL) {
		// remove it from selecters list
		if (previous)
			previous->next = s->next;
		else
			nsc->selecters = s->next;
		free(s);
	}
	
	if (nsc->selecters == NULL)
		// selecters list is empty: no need to monitor socket events anymore
		sCore->socket_set_event_callback(nsc->socket, NULL, NULL, event);
	
	// unlock the selecters list
	return release_sem(nsc->selecters_lock);
}


static void
on_socket_event(void * socket, uint32 event, void *cookie)
{
	net_stack_cookie *nsc = (net_stack_cookie *) cookie;
	selecter *s;
	
	if (!nsc)
		return;
	
	if (nsc->socket != socket) {
		ERROR("on_socket_event(%p, %ld, %p): socket is higly suspect! Aborting.\n",
			socket, event, cookie);
		return;
	}
	
	TRACE("on_socket_event(%p, %ld, %p)\n", socket, event, cookie);
	
	// lock the selecters list
	if (acquire_sem(nsc->selecters_lock) != B_OK)
		return;
	
	s = nsc->selecters;
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
	release_sem(nsc->selecters_lock);
	return;
}


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
