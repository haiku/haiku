/* net_stack_driver.c
 *
 * This file implements a very simple socket driver that is intended to
 * act as an interface to the networking stack.
 */

#ifndef _KERNEL_MODE

#error "This module MUST be built as a kernel driver!"

#else

// Public/system includes
#include <stdlib.h>
#include <string.h>

#include <drivers/Drivers.h>
#include <drivers/KernelExport.h>
#include <drivers/driver_settings.h>
#include <drivers/module.h>		// For get_module()/put_module()

#include <netinet/in_var.h>

// Private includes
#include <net_stack_driver.h>
#include <sys/protosw.h>
#include <core_module.h>

#define SHOW_INSANE_DEBUGGING   (1)
#define SERIAL_DEBUGGING        (0)
/* Force the driver to stay loaded in memory */
#define STAY_LOADED             (0)	

/*
 * Local definitions
 * -----------------
 */

#ifndef DRIVER_NAME
#define DRIVER_NAME 		"net_stack_driver"
#endif

#ifndef LOGID
#define LOGID DRIVER_NAME ": "
#endif

#ifndef WARN
#define WARN "Warning: "
#endif

#ifndef ERR
#define ERR "ERROR: "
#endif

#ifdef COMPILE_FOR_R5
	typedef status_t (*notify_select_event_function)(selectsync * sync, uint32 ref);
#else
	typedef status_t (*notify_select_event_function)(selectsync * sync, uint32 ref, uint8 event);
#endif

struct socket; /* forward declaration */

// this struct will store one select() event to monitor per thread
typedef struct selecter {
	struct selecter * 	next;
	thread_id 			thread;
	uint32				event;
	selectsync * 		sync;
	uint32 				ref;
	notify_select_event_function notify;
} selecter;

// the cookie we attach to each file descriptor opened on our driver entry
typedef struct {
	struct socket *	socket;			// NULL before ioctl(fd, NET_STACK_SOCKET/_ACCEPT)
	uint32			open_flags;		// the open() flags (mostly for storing O_NONBLOCK mode)
	sem_id			selecters_lock;	// protect the selecters linked-list
	selecter * 		selecters;		// the select()'ers lists (thread-aware) 
	notify_select_event_function notify;
} net_stack_cookie;

#if STAY_LOADED
	/* to unload the driver, simply write UNLOAD_CMD to him:
	 * $ echo stop > /dev/net/stack
	 * As soon as last app, via libnet.so, stop using it, it will unload,
	 * and in turn stop the stack and unload all network kernel modules...
	 */
	#define UNLOAD_CMD	"stop"
#endif

/* Prototypes of device hooks functions */
static status_t net_stack_open(const char * name, uint32 flags, void ** cookie);
static status_t net_stack_close(void * cookie);
static status_t net_stack_free_cookie(void * cookie);
static status_t net_stack_control(void * cookie, uint32 msg,void * data, size_t datalen);
static status_t net_stack_read(void * cookie, off_t pos, void * data, size_t * datalen);
static status_t net_stack_write(void * cookie, off_t pos, const void * data, size_t * datalen);
static status_t net_stack_select(void *cookie, uint8 event, uint32 ref, selectsync *sync);
static status_t net_stack_deselect(void *cookie, uint8 event, selectsync *sync);
// static status_t net_stack_readv(void * cookie, off_t pos, const iovec * vec, size_t count, size_t * len);
// static status_t net_stack_writev(void * cookie, off_t pos, const iovec * vec, size_t count, size_t * len);

/* Privates prototypes */
static void on_socket_event(void * socket, uint32 event, void * cookie);
static status_t r5_notify_select_event(selectsync * sync, uint32 ref, uint8 event);

#if STAY_LOADED
static status_t	keep_driver_loaded();
static status_t	unload_driver();
#endif

/*
 * Global variables
 * ----------------
 */

const char * g_device_names_list[] = {
        NET_STACK_DRIVER_DEV,
        NULL
};

device_hooks g_net_stack_driver_hooks =
{
	net_stack_open,			/* -> open entry point */
	net_stack_close,		/* -> close entry point */
	net_stack_free_cookie,	/* -> free entry point */
	net_stack_control,		/* -> control entry point */
	net_stack_read,			/* -> read entry point */
	net_stack_write,		/* -> write entry point */
	net_stack_select,		/* -> select entry point */
	net_stack_deselect,		/* -> deselect entry point */
	NULL,               	/* -> readv entry pint */
	NULL                	/* ->writev entry point */
};

struct core_module_info * core = NULL;

#if STAY_LOADED
int	g_stay_loaded_fd = -1;
#endif


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
_EXPORT status_t init_hardware(void)
{
	bool safemode = false;
	void *sfmptr;

#if SERIAL_DEBUGGING	
	// XXX - switch on/off at top of file...
	set_dprintf_enabled(true);
#endif

	// get a pointer to the driver settings...
	sfmptr = load_driver_settings(B_SAFEMODE_DRIVER_SETTINGS);

	// only use the pointer if it's valid
	if (sfmptr != NULL) {
		// we got a pointer, now get setting...
		safemode = get_driver_boolean_parameter(sfmptr, B_SAFEMODE_SAFE_MODE, false, false);
		// now get rid of settings
		unload_driver_settings(sfmptr);
	}
	if (safemode) {
		dprintf(LOGID WARN "init_hardware: declining offer to join the party.\n");
		return B_ERROR;
	}

#if SHOW_INSANE_DEBUGGING
	dprintf(LOGID "init_hardware done.\n");
#endif

	return B_OK;
}



/* init_driver()
 * called every time we're loaded.
 */
_EXPORT status_t init_driver(void)
{
	int rv = 0;

#if SHOW_INSANE_DEBUGGING
	dprintf(LOGID "init_driver, built %s %s\n", __DATE__, __TIME__);
#endif

	rv = get_module(NET_CORE_MODULE_NAME, (module_info **) &core);
	if (rv < 0) {
		dprintf(LOGID ERR "Argh, can't load " NET_CORE_MODULE_NAME " module: %d\n", rv);
		return rv;
	}
	
#if SHOW_INSANE_DEBUGGING
	dprintf(LOGID "init_driver: core = %p\n", core);
#endif

	// start the network stack!
	core->start();

	return B_OK;
}



/* uninit_driver()
 * called every time the driver is unloaded
 */
_EXPORT void uninit_driver(void)
{
#if SHOW_INSANE_DEBUGGING
	dprintf(LOGID "uninit_driver\n");
#endif

	if (core) {
#if STAY_LOADED
		// shutdown the network stack
		core->stop();
#endif
		put_module(NET_CORE_MODULE_NAME);
	};
}



/* publish_devices()
 * called to publish our device.
 */
_EXPORT const char ** publish_devices()
{
	return g_device_names_list;
}



_EXPORT device_hooks * find_device(const char* device_name)
{
	return &g_net_stack_driver_hooks;
}

// #pragma mark -


/*
 * Device hooks
 * ------------
 */

// the network stack functions - mainly just pass throughs...
static status_t net_stack_open(const char *name, uint32 flags, void **cookie)
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
	nsc->notify = NULL;

  	// attach this new net_socket_cookie to file descriptor
	*cookie = nsc; 

#if SHOW_INSANE_DEBUGGING
	dprintf(LOGID "net_stack_open(%s, %s%s) return this cookie: %p\n", name,
						( ((flags & O_RWMASK) == O_RDONLY) ? "O_RDONLY" :
						  ((flags & O_RWMASK) == O_WRONLY) ? "O_WRONLY" : "O_RDWR"),
						(flags & O_NONBLOCK) ? " O_NONBLOCK" : "",
						*cookie);
#endif

	return B_OK;
}



static status_t net_stack_close(void *cookie)
{
	net_stack_cookie *nsc = cookie;
	int rv;
	
#if SHOW_INSANE_DEBUGGING
	dprintf(LOGID "net_stack_close(%p)\n", nsc);
#endif

	rv = B_ERROR;
	if (nsc->socket) {
		// if a socket was opened on this fd, close it now.
		rv = core->socket_close(nsc->socket);
		nsc->socket = NULL;
	}
	
	return rv;
}



static status_t net_stack_free_cookie(void *cookie)
{
	net_stack_cookie *nsc = cookie;
	selecter *s;

#if SHOW_INSANE_DEBUGGING
	dprintf(LOGID "net_stack_free_cookie(%p)\n", cookie);
#endif

	// free the selecters list
	delete_sem(nsc->selecters_lock);
	s = nsc->selecters;
	while (s) {
		selecter *tmp = s;
		s = s->next;
		free(tmp);
	};
	
	// free the cookie
	free(cookie);
	return B_OK;
}



static status_t net_stack_control(void *cookie, uint32 op, void *data, size_t len)
{
	net_stack_cookie *nsc = cookie;
	struct stack_driver_args *args = data;
	int err = B_OK;

#if SHOW_INSANE_DEBUGGING
	dprintf(LOGID "net_stack_control(%p, 0x%lX, %p, %ld)\n", cookie, op, data, len);
#endif

#if STAY_LOADED
	keep_driver_loaded();
#endif

	if (! nsc->socket) {
		switch (op) {
			case NET_STACK_SOCKET: {

				// okay, now try to open a real socket behind this fd/net_stack_cookie pair
				err = core->socket_init(&nsc->socket);
				if (err == 0)
					err = core->socket_create(nsc->socket, args->u.socket.family, args->u.socket.type, args->u.socket.proto);
				// TODO: This is where the open flags need to be addressed
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
				return core->control_net_module(args->u.control.name, args->u.control.op,
					args->u.control.data, args->u.control.length);
			}
		};
	} else {
		switch (op) {
			case NET_STACK_CONNECT:
				return core->socket_connect(nsc->socket, (caddr_t) args->u.sockaddr.addr, args->u.sockaddr.addrlen);

			case NET_STACK_SHUTDOWN:
				return core->socket_shutdown(nsc->socket, args->u.integer);

			case NET_STACK_BIND:
				return core->socket_bind(nsc->socket, (caddr_t) args->u.sockaddr.addr, args->u.sockaddr.addrlen);
	
			case NET_STACK_LISTEN:
				// backlog to set
				return core->socket_listen(nsc->socket, args->u.integer);

			case NET_STACK_ACCEPT: {
				/* args->u.accept.cookie == net_stack_cookie of the already opened fd
				 * in libnet.so accept() call to bind to newly accepted socket
				 */
				 
				net_stack_cookie *ansc = args->u.accept.cookie;
				return core->socket_accept(nsc->socket, &ansc->socket, (void *)args->u.accept.addr, &args->u.accept.addrlen);
			}
			case NET_STACK_SEND:
				// TODO: flags gets ignored here...
				return net_stack_write(cookie, 0, args->u.transfer.data, &args->u.transfer.datalen);

			case NET_STACK_RECV:
				// TODO: flags gets ignored here...
				return net_stack_read(cookie, 0, args->u.transfer.data, &args->u.transfer.datalen);

			case NET_STACK_RECVFROM: {
				struct msghdr * mh = (struct msghdr *) data;
				int retsize;

				err = core->socket_recv(nsc->socket, mh, (caddr_t)&mh->msg_namelen, 
			                     &retsize);
				if (err == 0)
					return retsize;
				return err;
			}
			case NET_STACK_SENDTO: {
				struct msghdr * mh = (struct msghdr *) data;
				int retsize;

				err = core->socket_send(nsc->socket, mh, mh->msg_flags, 
			                     &retsize);
				if (err == 0)
					return retsize;
				return err;
			}
			case NET_STACK_SYSCTL:
				return core->net_sysctl(args->u.sysctl.name, args->u.sysctl.namelen,
			                          args->u.sysctl.oldp, args->u.sysctl.oldlenp,
			                          args->u.sysctl.newp, args->u.sysctl.newlen);
		
			case NET_STACK_GETSOCKOPT:
				return core->socket_getsockopt(nsc->socket, args->u.sockopt.level, args->u.sockopt.option,
			                      args->u.sockopt.optval, (size_t *) &args->u.sockopt.optlen);

			case NET_STACK_SETSOCKOPT:
				return core->socket_setsockopt(nsc->socket, args->u.sockopt.level, args->u.sockopt.option,
			                        (const void *) args->u.sockopt.optval, args->u.sockopt.optlen);

			case NET_STACK_GETSOCKNAME:
				return core->socket_getsockname(nsc->socket, args->u.sockaddr.addr, &args->u.sockaddr.addrlen);
	
			case NET_STACK_GETPEERNAME:
				return core->socket_getpeername(nsc->socket, args->u.sockaddr.addr, &args->u.sockaddr.addrlen);
				
			case B_SET_BLOCKING_IO: {
				int off = false;
				nsc->open_flags &= ~O_NONBLOCK;
				return core->socket_ioctl(nsc->socket, FIONBIO, (caddr_t) &off);
			}
	
			case B_SET_NONBLOCKING_IO: {
				int on = true;
				nsc->open_flags |= O_NONBLOCK;
				return core->socket_ioctl(nsc->socket, FIONBIO, (caddr_t) &on);
			}
	
			case NET_STACK_SELECT:
				/* if we get this opcode, we are using the r5 kernel select() call,
				 * so we can't use his notify_select_event(), but our own implementation! */
				nsc->notify = r5_notify_select_event;
				return net_stack_select(cookie, (args->u.select.ref & 0x0F),
										args->u.select.ref, args->u.select.sync);
			case NET_STACK_DESELECT: 
				return net_stack_deselect(cookie, (args->u.select.ref & 0x0F), args->u.select.sync);

			default:
				if (nsc->socket)
					// pass any unhandled opcode to the stack
					return core->socket_ioctl(nsc->socket, op, data);
				return B_BAD_VALUE;
		};
	};
	
	return err;
}



static status_t net_stack_read(void *cookie,
                                off_t position,
                                void *buffer,
                                size_t *readlen)
{
	net_stack_cookie *	nsc = (net_stack_cookie *) cookie;
	struct iovec iov;
	int error;
	int flags = 0;

#if SHOW_INSANE_DEBUGGING
	dprintf(LOGID "net_stack_read(%p, %Ld, %p, %ld)\n", cookie, position, buffer, *readlen);
#endif

#if STAY_LOADED
# if SHOW_INSANE_DEBUGGING
	dprintf(LOGID "Calling keep_driver_loaded()...\n");
# endif
	keep_driver_loaded();
#endif

	if (! nsc->socket)
		return B_BAD_VALUE;
	
	iov.iov_base = buffer;
	iov.iov_len = *readlen;
	
	error = core->socket_readv(nsc->socket, &iov, &flags);
	*readlen = error;
    return error;
}



static status_t net_stack_write(void *cookie,
                                 off_t position,
                                 const void *buffer,
                                 size_t *writelen)
{
	net_stack_cookie *	nsc = (net_stack_cookie *) cookie;
	struct iovec iov;
	int error;
	int flags = 0;
#if SHOW_INSANE_DEBUGGING
	dprintf(LOGID "net_stack_write(%p, %Ld, %p, %ld)\n", cookie, position, buffer, *writelen);
#endif

#if STAY_LOADED
	keep_driver_loaded();
#endif

	if (! nsc->socket) {
#if STAY_LOADED
		if (*writelen >= strlen(UNLOAD_CMD) &&
		    strncmp(buffer, UNLOAD_CMD, strlen(UNLOAD_CMD)) == 0)
			// someone write/send/tell us to unload this driver, so do it!
			return unload_driver();
#endif
		return B_BAD_VALUE;
	};

	iov.iov_base = (void*)buffer;
	iov.iov_len = *writelen;
	
	error = core->socket_writev(nsc->socket, &iov, flags);
	*writelen = error;
	return error;	
}



static status_t net_stack_select(void *cookie, uint8 event, uint32 ref, selectsync *sync)
{
	net_stack_cookie *	nsc = (net_stack_cookie *) cookie;
	selecter * s;
	status_t status;

#if SHOW_INSANE_DEBUGGING
	dprintf(LOGID "net_stack_select(%p, %d, %ld, %p)\n", cookie, event, ref, sync);
#endif
	
	if (! nsc->socket)
		return B_BAD_VALUE;
	
	// set notification function if undefined
	if(nsc->notify == NULL)
		nsc->notify = notify_select_event;
	
	s = (selecter *) malloc(sizeof(selecter));
	if (! s)
		return B_NO_MEMORY;
		
	s->thread = find_thread(NULL);	// current thread id
	s->event = event;
	s->sync = sync;
	s->ref = ref;
	s->notify = nsc->notify;
	nsc->notify = NULL;

	// lock the selecters list	
	status = acquire_sem(nsc->selecters_lock);
	if (status != B_OK) {
		free(s);
		return status;
	};
	
	// add it to selecters list
	s->next = nsc->selecters;	
	nsc->selecters = s;
	
	// unlock the selecters list
	release_sem(nsc->selecters_lock);

	// start (or continue) to monitor for socket event
	return core->socket_set_event_callback(nsc->socket, on_socket_event, nsc, event);
}



static status_t net_stack_deselect(void *cookie, uint8 event, selectsync *sync)
{
	net_stack_cookie *nsc = (net_stack_cookie *) cookie;
	selecter * previous;
	selecter * s;
	thread_id current_thread;
	status_t status;

#if SHOW_INSANE_DEBUGGING
	dprintf(LOGID "net_stack_deselect(%p, %d, %p)\n", cookie, event, sync);
#endif
	
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
	};
	
	if (s != NULL) {
		// remove it from selecters list
		if (previous)
			previous->next = s->next;
		else
			nsc->selecters = s->next;
		free(s);
	};

	if (nsc->selecters == NULL)
		// selecters list is empty: no need to monitor socket events anymore
		core->socket_set_event_callback(nsc->socket, NULL, NULL, event);

	// unlock the selecters list
	return release_sem(nsc->selecters_lock);
}


// #pragma mark -

static void on_socket_event(void * socket, uint32 event, void * cookie)
{
	net_stack_cookie * nsc = (net_stack_cookie *) cookie;
	selecter * s;

	if (!nsc)
		return;

	if (nsc->socket != socket) {
		#if SHOW_INSANE_DEBUGGING
			dprintf(LOGID "on_socket_event(%p, %ld, %p): socket is higly suspect! Aborting.\n", socket, event, cookie);
		#endif
		return;
	}	

#if SHOW_INSANE_DEBUGGING
	dprintf(LOGID "on_socket_event(%p, %ld, %p)\n", socket, event, cookie);
#endif

	// lock the selecters list
	if (acquire_sem(nsc->selecters_lock) != B_OK)
		return;
		
	s = nsc->selecters;
	while (s) {
		if (s->event == event)
			// notify this selecter (thread/event pair)
#ifdef COMPILRE_FOR_R5
			s->notify(s->sync, s->ref);
#else
			s->notify(s->sync, s->ref, event);
#endif
		s = s->next;
	};

	// unlock the selecters list
	release_sem(nsc->selecters_lock);
	return;
}

/*
	Under vanilla R5, we can't use the kernel notify_select_event(),
	as select() kernel implementation is too buggy to be usefull.
	So, here is our own notify_select_event() implementation, the driver-side pair
	of the our libnet.so select() implementation...
*/
static status_t r5_notify_select_event(selectsync * sync, uint32 ref, uint8 event)
{
	area_id	area;
	struct r5_selectsync *	rss;
	int fd;

#if SHOW_INSANE_DEBUGGING
	dprintf(LOGID "r5_notify_select_event(%p, %ld, %d)\n", sync, ref, event);
#endif

	rss = NULL;
	area = clone_area("r5_selectsync_area (driver)", (void **) &rss,
		B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA, (area_id) sync);
	if (area < B_OK) {
#if SHOW_INSANE_DEBUGGING
		dprintf(LOGID "r5_notify_select_event: clone_area(%d) failed -> %d!\n", (int) sync, (int) area);
#endif
		return area;
	};
	
#if SHOW_INSANE_DEBUGGING
	dprintf(LOGID "r5_selectsync at %p (area %ld, clone from %ld):\n"
	              "lock   %ld\n"
	              "wakeup %ld\n", rss, area, (area_id) sync, rss->lock, rss->wakeup);
#endif

	if (acquire_sem(rss->lock) != B_OK)
		// if we can't (anymore?) lock the shared r5_selectsync, select() party is done
		goto error;
	
	fd = ref >> 8;
	switch (ref & 0xFF) {	// event == ref & 0xFF
	case B_SELECT_READ:
		FD_SET(fd, &rss->rbits);
		break;
	case B_SELECT_WRITE:
		FD_SET(fd, &rss->wbits);
		break;
	case B_SELECT_ERROR:
		FD_SET(fd, &rss->ebits);
		break;
	};
		
	// wakeup select()
	release_sem(rss->wakeup);
		
	release_sem(rss->lock);
	return B_OK;
	
error:
	delete_area(area);
	return B_ERROR;
}


// #pragma mark -


#if STAY_LOADED
static status_t	keep_driver_loaded()
{
	if ( g_stay_loaded_fd != -1 )
		return B_OK;
		
	/* force the driver to stay loaded by opening himself */
#if SHOW_INSANE_DEBUGGING
	dprintf(LOGID "keep_driver_loaded: internaly opening " NET_STACK_DRIVER_PATH " to stay loaded in memory...\n");
#endif

	g_stay_loaded_fd = open(NET_STACK_DRIVER_PATH, 0);

#if SHOW_INSANE_DEBUGGING
	if (g_stay_loaded_fd < 0)
		dprintf(LOGID ERR "keep_driver_loaded: couldn't open(" NET_STACK_DRIVER_PATH ")!\n");
#endif

	return B_OK;
}

static status_t unload_driver()
{
	if ( g_stay_loaded_fd >= 0 )
		{
		int tmp_fd;
			
		/* we need to set g_stay_loaded_fd to < 0 if we don't want
		 * the next close enter again in this case, and so on...
		 */
#if SHOW_INSANE_DEBUGGING
		dprintf(LOGID "unload_driver: unload requested.\n");
#endif
		tmp_fd = g_stay_loaded_fd;
		g_stay_loaded_fd = -1;

		close(tmp_fd);
		};
			
	return B_OK;
}
#endif /* STAY_LOADED */

#endif /* _KERNEL_MODE */
