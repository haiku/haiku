/* net_server_driver - This file implements a very simple socket driver
**		that is intended to act as an interface to the networking stack when
**		it is loaded in userland, hosted by a net_server-like app.
**		The communication is slow, and could probably be much better, but it's
**		working, and that should be enough for now.
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/

#ifndef _KERNEL_MODE

#error "This module MUST be built as a kernel driver!"

#else

// Public includes
//#include <OS.h>
#include <drivers/Drivers.h>
#include <drivers/KernelExport.h>

// Posix includes
#include <malloc.h>
#include <string.h>
#include <netinet/in_var.h>

// Private includes
#include <net_stack_driver.h>
#include <userland_ipc.h>
#include <sys/sockio.h>

/* these are missing from KernelExport.h ... */
#define  B_SELECT_READ       1 
#define  B_SELECT_WRITE      2 
#define  B_SELECT_EXCEPTION  3 

extern void notify_select_event(selectsync * sync, uint32 ref);

//*****************************************************/
// Debug output
//*****************************************************/

#define DEBUG_PREFIX "net_server_driver: "
#define __out dprintf

#define DEBUG 1
#ifdef DEBUG
	#define PRINT(x) { __out(DEBUG_PREFIX); __out x; }
	#define REPORT_ERROR(status) __out(DEBUG_PREFIX "%s:%ld: %s\n",__FUNCTION__,__LINE__,strerror(status));
	#define RETURN_ERROR(err) { status_t _status = err; if (_status < B_OK) REPORT_ERROR(_status); return _status;}
	#define FATAL(x) { __out(DEBUG_PREFIX); __out x; }
	#define INFORM(x) { __out(DEBUG_PREFIX); __out x; }
	#define FUNCTION() __out(DEBUG_PREFIX "%s()\n",__FUNCTION__);
	#define FUNCTION_START(x) { __out(DEBUG_PREFIX "%s() ",__FUNCTION__); __out x; }
//	#define FUNCTION() ;
//	#define FUNCTION_START(x) ;
	#define D(x) {x;};
#else
	#define PRINT(x) ;
	#define REPORT_ERROR(status) ;
	#define RETURN_ERROR(status) return status;
	#define FATAL(x) { __out(DEBUG_PREFIX); __out x; }
	#define INFORM(x) { __out(DEBUG_PREFIX); __out x; }
	#define FUNCTION() ;
	#define FUNCTION_START(x) ;
	#define D(x) ;
#endif


//*****************************************************/
// Structure definitions
//*****************************************************/

#ifndef DRIVER_NAME
#	define DRIVER_NAME 		"net_server_driver"
#endif

/* wait one second when waiting on the stack */
#define STACK_TIMEOUT 1000000LL

typedef void (*notify_select_event_function)(selectsync * sync, uint32 ref);

// this struct will store one select() event to monitor per thread
typedef struct selecter {
	struct selecter * 	next;
	thread_id 			thread;
	uint32				event;
	selectsync * 		sync;
	uint32 				ref;
} selecter;


// the cookie we attach to each file descriptor opened on our driver entry
typedef struct {
	port_id		local_port;
	port_id		remote_port;
	area_id		area;
	thread_id	socket_thread;

	sem_id		command_sem;
	net_command *commands;
	int32		command_index;
	int32		nb_commands;

	sem_id		selecters_lock;	// protect the selecters linked-list
	selecter * 	selecters;	// the select()'ers lists (thread-aware) 
} net_server_cookie;


//*****************************************************/
// Prototypes
//*****************************************************/

/* device hooks */
static status_t net_server_open(const char *name, uint32 flags, void **cookie);
static status_t net_server_close(void *cookie);
static status_t net_server_free_cookie(void *cookie);
static status_t net_server_control(void *cookie, uint32 msg, void *data, size_t datalen);
static status_t net_server_read(void *cookie, off_t pos, void *data, size_t *datalen);
static status_t net_server_write(void *cookie, off_t pos, const void *data, size_t *datalen);
static status_t net_server_select(void *cookie, uint8 event, uint32 ref, selectsync *sync);
static status_t net_server_deselect(void *cookie, uint8 event, selectsync *sync);
// static status_t net_server_readv(void *cookie, off_t pos, const iovec *vec, size_t count, size_t *len);
// static status_t net_server_writev(void *cookie, off_t pos, const iovec *vec, size_t count, size_t *len);

/* select() support */
static int32 socket_event_listener(void *data);
static void on_socket_event(void *socket, uint32 event, void *cookie);
static void r5_notify_select_event(selectsync *sync, uint32 ref);

/* command queue */
static status_t init_connection(void **cookie);
static void shutdown_connection(net_server_cookie *nsc);
static net_command *get_command(net_server_cookie *nsc, int32 *index);
static status_t execute_command(net_server_cookie *nsc, uint32 op, void *data, uint32 length);

/*
 * Global variables
 * ----------------
 */

#define NET_SERVER_DRIVER_DEV	"net/server"
#define NET_SERVER_DRIVER_PATH	"/dev/" ## NET_SERVER_DRIVER_DEV

const char * g_device_names_list[] = {
        "net/server",
        NULL
};

device_hooks g_net_server_driver_hooks =
{
	net_server_open,		/* -> open entry point */
	net_server_close,		/* -> close entry point */
	net_server_free_cookie,	/* -> free entry point */
	net_server_control,		/* -> control entry point */
	net_server_read,		/* -> read entry point */
	net_server_write,		/* -> write entry point */
	net_server_select,		/* -> select entry point */
	net_server_deselect,	/* -> deselect entry point */
	NULL,               	/* -> readv entry pint */
	NULL                	/* -> writev entry point */
};

// by default, assert we can use kernel select() support
notify_select_event_function g_nse = notify_select_event;
thread_id g_socket_event_listener_thread = -1;
port_id g_socket_event_listener_port = -1;

port_id g_stack_port = -1;


/*
 * Driver API calls
 * ----------------
 */
 
_EXPORT int32 api_version = B_CUR_DRIVER_API_VERSION;


_EXPORT status_t
init_hardware(void)
{
	return B_OK;
}


_EXPORT status_t
init_driver (void)
{
	thread_id thread;
	port_id port;
	
	FUNCTION();

	port = create_port(32, "socket_event_listener");
	if (port < B_OK)
		return port;
	set_port_owner(port, B_SYSTEM_TEAM);

	thread = spawn_kernel_thread(socket_event_listener, "socket_event_listener",
				B_NORMAL_PRIORITY, NULL);
	if (thread < B_OK) {
		delete_port(port);
		return thread;
	};
		
	g_socket_event_listener_thread = thread;
	g_socket_event_listener_port = port;
	
	return resume_thread(thread);
}


_EXPORT void
uninit_driver (void)
{
	status_t	dummy;
	
	FUNCTION();

	delete_port(g_socket_event_listener_port);	
	wait_for_thread(g_socket_event_listener_thread, &dummy);
}


_EXPORT const char **
publish_devices()
{
	FUNCTION();
	return g_device_names_list;
}


_EXPORT device_hooks *
find_device(const char *deviceName)
{
	FUNCTION();
	return &g_net_server_driver_hooks;
}


//	#pragma mark -
//*****************************************************/
// Device hooks 
//*****************************************************/


static status_t
net_server_open(const char *name, uint32 flags, void **cookie)
{
	net_server_cookie *nsc;

	status_t status = init_connection(cookie);
	if (status < B_OK)
		return status;

	nsc = *cookie;

	status = execute_command(nsc, NET_STACK_OPEN, &flags, sizeof(uint32));
	if (status < B_OK)
		net_server_free_cookie(nsc);

	return status;
}


static status_t
net_server_close(void *cookie)
{
	net_server_cookie *nsc = cookie;
	if (nsc == NULL)
		return B_BAD_VALUE;

	// we don't care here if the stack isn't alive anymore -
	// from the kernel's point of view, the device can always
	// be closed
	execute_command(nsc, NET_STACK_CLOSE, NULL, 0);
	return B_OK;
}


static status_t
net_server_free_cookie(void *cookie)
{
	net_server_cookie *nsc = cookie;
	if (nsc == NULL)
		return B_BAD_VALUE;

	shutdown_connection(nsc);
	return B_OK;
}


static status_t
net_server_control(void *cookie, uint32 op, void *data, size_t length)
{
	net_server_cookie *nsc = cookie;
	struct stack_driver_args *args = data;

	//FUNCTION_START(("cookie = %p, op = %lx, data = %p, length = %ld\n", cookie, op, data, length));

	if (nsc == NULL)
		return B_BAD_VALUE;

	switch (op) {
		case NET_STACK_SELECT:
			// if we get this call via ioctl() we are obviously called from an
			// R5 compatible libnet: we are using the r5 kernel select() call,
			// So, we can't use the kernel notify_select_event(), but our own implementation!
			g_nse = r5_notify_select_event;
			return net_server_select(cookie, (args->u.select.ref & 0x0F), args->u.select.ref, args->u.select.sync);

		case NET_STACK_DESELECT:
			return net_server_deselect(cookie, (args->u.select.ref & 0x0F), args->u.select.sync);

		default:
			return execute_command(nsc, op, data, -1);
	};
}


static status_t
net_server_read(void *cookie, off_t pos, void *buffer, size_t *length)
{
	net_server_cookie *nsc = cookie;
	struct stack_driver_args args;
	int status;

	memset(&args, 0, sizeof(args));
	args.u.transfer.data = buffer;
	args.u.transfer.datalen = *length;

	status = execute_command(nsc, NET_STACK_RECV, &args, sizeof(args));
	if (status > 0) {
		*length = status;
		return B_OK;
	}
	return status;
}


static status_t
net_server_write(void *cookie, off_t pos, const void *buffer, size_t *length)
{
	net_server_cookie *nsc = cookie;
	struct stack_driver_args args;
	int status;
	
	memset(&args, 0, sizeof(args));
	args.u.transfer.data = (void *) buffer;
	args.u.transfer.datalen = *length;
	
	status = execute_command(nsc, NET_STACK_SEND, &args, sizeof(args));
	if (status > 0) {
		*length = status;
		return B_OK;
	}
	return status;
}


static status_t
net_server_select(void *cookie, uint8 event, uint32 ref, selectsync *sync)
{
	net_server_cookie *nsc = cookie;
	struct notify_socket_event_args args;
	selecter *s;
	status_t status;

	FUNCTION_START(("cookie = %p, event = %d,  ref = %lx, sync =%p\n", cookie, event, ref, sync));

	s = (selecter *) malloc(sizeof(selecter));
	if (!s)
		return B_NO_MEMORY;
		

	// lock the selecters list	
	status = acquire_sem(nsc->selecters_lock);
	if (status != B_OK) {
		free(s);
		return status;
	};
		
	s->thread = find_thread(NULL);	// current thread id
	s->event = event;
	s->sync = sync;
	s->ref = ref;
	
	// add it to selecters list
	s->next = nsc->selecters;	
	nsc->selecters = s;
	
	// unlock the selecters list
	release_sem(nsc->selecters_lock);

	// Tell the net_server to notify event(s) for this socket
	memset(&args, 0, sizeof(args));
	args.notify_port = g_socket_event_listener_port;
	args.cookie = cookie;;
	
	return execute_command(nsc, NET_STACK_NOTIFY_SOCKET_EVENT, &args, sizeof(args));
}


static status_t
net_server_deselect(void *cookie, uint8 event, selectsync *sync)
{
	net_server_cookie *nsc = cookie;
	selecter *previous;
	selecter *s;
	thread_id current_thread;
	status_t status;

	FUNCTION_START(("cookie = %p, event = %d,  sync =%p\n", cookie, event, sync));

	if (!nsc)
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

	status = B_OK;
	if (nsc->selecters == NULL) {
		struct notify_socket_event_args args;

		// Selecters list is empty: no need to monitor socket events anymore
		// Tell the net_server to stop notifying event(s) for this socket
		memset(&args, 0, sizeof(args));
		args.notify_port = -1;	// stop notifying
		args.cookie = cookie;	// for sanity check
		status = execute_command(nsc, NET_STACK_NOTIFY_SOCKET_EVENT, NULL, -1);
	};

	// unlock the selecters list
	release_sem(nsc->selecters_lock);

	return status;	
}


// #pragma mark -
//*****************************************************/
// select() support
//*****************************************************/

/*
	This is started as separate thread at init_driver() time to wait for
	socket event notification.
	
	One port to listen them all.
	One port to find them.
	One port to get them and in [r5_]notify_select_event() send them.
	
	Okay, I should stop watch this movie *now*.
*/

static int32 socket_event_listener(void *data)
{
	struct socket_event_data sed;
	int32 msg;
	ssize_t bytes;
	
	while(true) {
		bytes = read_port(g_socket_event_listener_port, &msg, &sed, sizeof(sed));
		if (bytes < B_OK)
			return bytes;

		if (msg == NET_STACK_SOCKET_EVENT_NOTIFICATION)
			// yep, we pass a NULL "socket" pointer here, but don't worry, we don't use anyway
			on_socket_event(NULL, sed.event, sed.cookie);
	};
	return 0;
}


static void on_socket_event(void *socket, uint32 event, void *cookie)
{
	net_server_cookie *nsc = cookie;
	selecter *s;

	if (!nsc)
		return;

	FUNCTION_START(("socket = %p, event = %ld,  cookie = %p\n", socket, event, cookie));

	// lock the selecters list
	if (acquire_sem(nsc->selecters_lock) != B_OK)
		return;
		
	s = nsc->selecters;
	while (s) {
		if (s->event == event)
			// notify this selecter (thread/event pair)
			g_nse(s->sync, s->ref);
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
static void r5_notify_select_event(selectsync *sync, uint32 ref)
{
	area_id	area;
	struct r5_selectsync *rss;
	int fd;

	FUNCTION_START(("sync = %p, ref = %lx\n", sync, ref));

	rss = NULL;
	area = clone_area("r5_selectsync_area (driver)", (void **) &rss,
		B_ANY_KERNEL_ADDRESS, B_READ_AREA | B_WRITE_AREA, (area_id) sync);
	if (area < B_OK) {
#if SHOW_INSANE_DEBUGGING
		dprintf(LOGID "r5_notify_select_event: clone_area(%d) failed -> %d!\n", (area_id) sync, area);
#endif
		return;
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
	switch (ref & 0xFF) {
	case B_SELECT_READ:
		FD_SET(fd, &rss->rbits);
		break;
	case B_SELECT_WRITE:
		FD_SET(fd, &rss->wbits);
		break;
	case B_SELECT_EXCEPTION:
		FD_SET(fd, &rss->ebits);
		break;
	};
		
	// wakeup select()
	release_sem(rss->wakeup);
		
	release_sem(rss->lock);	
	
error:
	delete_area(area);
}


//	#pragma mark -


//*****************************************************/
// The Command Queue
//*****************************************************/


static net_command *
get_command(net_server_cookie *nsc, int32 *index)
{
	int32 i, count = 0;
	net_command *command;

	while (count < nsc->nb_commands*2) {
		i = atomic_add(&nsc->command_index,1) & (nsc->nb_commands - 1);
		command = nsc->commands + i;

		if (command->op == 0) {
			// command is free to use
			*index = i;
			return command;
		}
		count++;
	}
	return NULL;
}


static status_t
get_area_from_address(net_area_info *info, void *data)
{
	area_info areaInfo;

	if (data == NULL)
		return B_OK;

	info->id = area_for(data);
	if (info->id < B_OK)
		return info->id;

	if (get_area_info(info->id,&areaInfo) != B_OK)
		return B_BAD_VALUE;

	info->offset = areaInfo.address;
	return B_OK;
}


static void
set_command_areas(net_command *command)
{
	struct stack_driver_args *args = (void *) command->data;

	if (args == NULL)
		return;

	if (get_area_from_address(&command->area[0], args) < B_OK)
		return;

	switch (command->op) {
		case NET_STACK_GETSOCKOPT:
		case NET_STACK_SETSOCKOPT:
			get_area_from_address(&command->area[1], args->u.sockopt.optval);
			break;

		case NET_STACK_CONNECT:
		case NET_STACK_BIND:
		case NET_STACK_GETSOCKNAME:
		case NET_STACK_GETPEERNAME:
			get_area_from_address(&command->area[1], args->u.sockaddr.addr);
			break;

		case NET_STACK_RECV:
		case NET_STACK_SEND:
			get_area_from_address(&command->area[1], args->u.transfer.data);
			get_area_from_address(&command->area[2], args->u.transfer.addr);
			break;

		case NET_STACK_RECVFROM:
		case NET_STACK_SENDTO: {
			struct msghdr *mh = (void *) args;
			get_area_from_address(&command->area[1],mh->msg_name);
			get_area_from_address(&command->area[2],mh->msg_iov);
			get_area_from_address(&command->area[3],mh->msg_control);
			break;
		}

		case NET_STACK_ACCEPT: 
			/* accept_args.cookie is always in the address space of the server */ 
			get_area_from_address(&command->area[1], args->u.accept.addr); 
			break; 
                
		case NET_STACK_SYSCTL:
			get_area_from_address(&command->area[1], args->u.sysctl.name);
			get_area_from_address(&command->area[2], args->u.sysctl.oldp);
			get_area_from_address(&command->area[3], args->u.sysctl.oldlenp);
			get_area_from_address(&command->area[4], args->u.sysctl.newp);
			break;

		case OSIOCGIFCONF:
		case SIOCGIFCONF: {
			struct ifconf *ifc = (void *) args;

			get_area_from_address(&command->area[1], ifc->ifc_buf);
			break;
		}
	}	
}


static status_t
execute_command(net_server_cookie *nsc, uint32 op, void *data, uint32 length)
{
	uint32 command_index;
	net_command *command = get_command(nsc, (long *) &command_index);
	int32 max_tries = 200;
	status_t status;
	ssize_t bytes;

	if (command == NULL) {
		FATAL(("execute: command queue is full\n"));
		return B_ERROR;
	}

	memset(command, 0, sizeof(net_command));

	command->op = op;
	command->data = data;
	set_command_areas(command);

	bytes = write_port(nsc->remote_port, command_index, NULL, 0);
	if (bytes < B_OK) {
		FATAL(("execute %ld: couldn't contact stack (id = %ld): %s\n",op, nsc->remote_port, strerror(bytes)));
		return bytes;
	}

	// if we're closing the connection, there is no need to
	// wait for a result
	if (op == NET_STACK_CLOSE)
		return B_OK;

	while (true) {
		// wait until we get the results back from our command
		if ((status = acquire_sem_etc(nsc->command_sem, 1, B_CAN_INTERRUPT, 0)) == B_OK) {
			if (command->op != 0) {
				if (--max_tries <= 0) {
					FATAL(("command is not freed after 200 tries!\n"));
					return B_ERROR;
				}
				release_sem(nsc->command_sem);
				continue;
			}
			return command->result;
		}
		FATAL(("command couldn't be executed: %s\n", strerror(status)));
		if (status == B_INTERRUPTED)
			// Signaling our net_server counterpart, so his socket thread awake too...
			send_signal_etc(nsc->socket_thread, SIGINT, 0);
		return status;
	}
}


static status_t
init_connection(void **cookie)
{
	net_connection connection;
	ssize_t bytes;
	uint32 msg;

	net_server_cookie *nsc = (net_server_cookie *) malloc(sizeof(net_server_cookie));
	if (nsc == NULL)
		return B_NO_MEMORY;

	// create a new port and get a connection from the stack

	nsc->local_port = create_port(CONNECTION_QUEUE_LENGTH, "net_driver connection");
	if (nsc->local_port < B_OK) {
		FATAL(("open: couldn't create port: %s\n", strerror(nsc->local_port)));
		free(cookie);
		return B_ERROR;
	}
	set_port_owner(nsc->local_port, B_SYSTEM_TEAM);

	bytes = write_port(g_stack_port, NET_STACK_NEW_CONNECTION, &nsc->local_port, sizeof(port_id));
	if (bytes == B_BAD_PORT_ID) {
		g_stack_port = find_port(NET_STACK_PORTNAME);
		PRINT(("try to get net_server's port id: %ld\n", g_stack_port));
		bytes = write_port(g_stack_port, NET_STACK_NEW_CONNECTION, &nsc->local_port, sizeof(port_id));
	}
	if (bytes < B_OK) {
		FATAL(("open: couldn't contact stack: %s\n",strerror(bytes)));
		delete_port(nsc->local_port);
		free(nsc);
		return bytes;
	}

	bytes = read_port_etc(nsc->local_port, &msg, &connection, sizeof(net_connection), B_TIMEOUT,STACK_TIMEOUT);
	if (bytes < B_OK) {
		FATAL(("open: didn't hear back from stack: %s\n",strerror(bytes)));
		delete_port(nsc->local_port);
		free(nsc);
		return bytes;
	}
	if (msg != NET_STACK_NEW_CONNECTION) {
		FATAL(("open: received wrong answer: %ld\n",msg));
		delete_port(nsc->local_port);
		free(nsc);
		return B_ERROR;
	}

	// set up communication channels
	
	// ToDo: close server connection if anything fails
	// -> could also be done by the server, if it doesn't receive NET_STACK_OPEN

	nsc->remote_port = connection.port;
	nsc->command_sem = connection.commandSemaphore;
	nsc->area = clone_area("net connection buffer", (void **) &nsc->commands,
					B_CLONE_ADDRESS, B_READ_AREA | B_WRITE_AREA, connection.area);
	if (nsc->area < B_OK) {
		status_t err;
		FATAL(("couldn't clone command queue: %s\n", strerror(nsc->area)));
		err = nsc->area;
		delete_port(nsc->local_port);
		free(nsc);
		return err;
	}
	
	nsc->socket_thread = connection.socket_thread;
	nsc->nb_commands = connection.numCommands;
	nsc->command_index = 0;
	nsc->selecters = NULL;

	nsc->selecters_lock = create_sem(1, "socket_selecters_lock");
	if (nsc->selecters_lock < B_OK) {
		status_t err;
		FATAL(("couldn't create socket_selecters_lock semaphore: %s\n", strerror(nsc->selecters_lock)));
		err = nsc->selecters_lock;
		delete_port(nsc->local_port);
		delete_area(nsc->area);
		free(nsc);
		return err;
	}

	*cookie = nsc;
	return B_OK;
}


static void
shutdown_connection(net_server_cookie *nsc)
{
	selecter *s;

	delete_area(nsc->area);
	delete_port(nsc->local_port);

	// free the selecters list
	delete_sem(nsc->selecters_lock);
	s = nsc->selecters;
	while (s) {
		selecter * tmp = s;
		s = s->next;
		free(tmp);
	};

	free(nsc);
}

#endif	// #ifndef _KERNEL_MODE

