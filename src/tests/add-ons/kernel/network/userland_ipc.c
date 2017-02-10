/* userland_ipc - Communication between the network driver
**		and the userland stack.
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the MIT License.
*/


#include "userland_ipc.h"

#include "sys/socket.h"
#include "net_misc.h"
#include "core_module.h"
#include "net_module.h"
#include "sys/sockio.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <OS.h>
#include <Drivers.h>	// for B_SET_(NON)BLOCKING_IO


typedef struct commands_info {
	int op;
	const char *name;
} commands_info;

#define C2N(op) { op, #op }

commands_info g_commands_info[] = {
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
//	C2N(NET_STACK_STOP),
	C2N(NET_STACK_NOTIFY_SOCKET_EVENT),
	C2N(NET_STACK_CONTROL_NET_MODULE),

	// Userland IPC-specific opcodes
	C2N(NET_STACK_OPEN),
	C2N(NET_STACK_CLOSE),
	C2N(NET_STACK_NEW_CONNECTION),
	
	{ 0, "Unknown!" }
};

#undef C2N



extern struct core_module_info *core;

// installs a main()
//#define COMMUNICATION_TEST

#define NUM_COMMANDS 32
#define CONNECTION_BUFFER_SIZE (65536 + 4096 - CONNECTION_COMMAND_SIZE)

#define ROUND_TO_PAGE_SIZE(x) (((x) + (B_PAGE_SIZE) - 1) & ~((B_PAGE_SIZE) - 1))

struct socket; /* forward declaration */

typedef struct {
	port_id			localPort,port;
	area_id			area;
	struct socket * socket;

	uint8		*buffer;
	net_command *commands;
	sem_id		commandSemaphore;

	int32		openFlags;

	thread_id	runner;
	
	// for socket select events support
	port_id 	socket_event_port;
	void *		notify_cookie;
} connection_cookie;


port_id gStackPort = -1;
thread_id gConnectionOpener = -1;

// prototypes
static int32 connection_runner(void *_cookie);
static status_t init_connection(net_connection *connection, connection_cookie **_cookie);
static void shutdown_connection(connection_cookie *cookie);


static void
delete_cloned_areas(net_area_info *area)
{
	int32 i;
	for (i = 0;i < MAX_NET_AREAS;i++) {
		if (area[i].id == 0)
			continue;

		delete_area(area[i].id);
	}
}


static status_t
clone_command_areas(net_area_info *localArea,net_command *command)
{
	int32 i;

	memset(localArea,0,sizeof(net_area_info) * MAX_NET_AREAS);

	for (i = 0;i < MAX_NET_AREAS;i++) {
		if (command->area[i].id <= 0)
			continue;

		localArea[i].id = clone_area("net connection",(void **)&localArea[i].offset,B_ANY_ADDRESS,
				B_READ_AREA | B_WRITE_AREA,command->area[i].id);
		if (localArea[i].id < B_OK)
			return localArea[i].id;
	}
	return B_OK;
}


static uint8 *
convert_address(net_area_info *fromArea,net_area_info *toArea,uint8 *data)
{
	if (data == NULL)
		return NULL;

	if (data < fromArea->offset) {
		printf("could not translate address: %p\n",data);
		return data;
	}

	return data - fromArea->offset + toArea->offset;
}


static inline void *
convert_to_local(net_area_info *foreignArea,net_area_info *localArea,void *data)
{
	return convert_address(foreignArea,localArea,data);
}


static void *
convert_to_foreign(net_area_info *foreignArea,net_area_info *localArea,void *data)
{
	return convert_address(localArea,foreignArea,data);
}


static void
on_socket_event(void * socket, uint32 event, void * cookie)
{
	connection_cookie * cc = (connection_cookie *) cookie;
	struct socket_event_data sed;
	status_t status;

	if (!cc)
		return;

	if (cc->socket != socket) {
		printf("on_socket_event(%p, %ld, %p): socket is higly suspect! Aborting.\n", socket, event, cookie);
		return;
	}	

	printf("on_socket_event(%p, %ld, %p)\n", socket, event, cookie);

	sed.event = event;
	sed.cookie = cc->notify_cookie;

	// TODO: don't block here => write_port_etc() ?
	status = write_port(cc->socket_event_port, NET_STACK_SOCKET_EVENT_NOTIFICATION,
				&sed, sizeof(sed));
	if (status != B_OK)
		printf("write_port(NET_STACK_SOCKET_EVENT_NOTIFICATION) failure: %s\n",
			strerror(status));
	return;
}



static int32
connection_runner(void *_cookie)
{
	connection_cookie *cookie = (connection_cookie *)_cookie;
	bool run = true;
	commands_info *ci;

	while (run) {
		net_area_info area[MAX_NET_AREAS];
		net_command *command;
		status_t status = B_OK;
		struct stack_driver_args *args;
		int32 index;
		ssize_t bytes = read_port(cookie->localPort,&index,NULL,0);
		if (bytes < B_OK)
			break;

		if (index >= NUM_COMMANDS || index < 0) {
			printf("got bad command index: %lx\n",index);
			continue;
		}
		command = cookie->commands + index;
		if (clone_command_areas(area, command) < B_OK) {
			printf("could not clone command areas!\n");
			continue;
		}
		
		
		ci = g_commands_info;
		while(ci && ci->op) {
			if (ci->op == command->op)
				break;
			ci++;
		}
		
		args = convert_to_local(&command->area[0],&area[0], command->data);
		printf("command %s (0x%lx) (index = %ld), buffer = %p, length = %ld, result = %ld\n",
			ci->name, command->op, index, args, command->length, command->result);

		switch (command->op) {
			case NET_STACK_OPEN:
				cookie->openFlags = args->u.integer;
				printf("Opening socket connection, mode = %lx!\n", cookie->openFlags);
				break;

			case NET_STACK_CLOSE:
				printf("Closing socket connection...\n");
				run = false;
				break;

			case NET_STACK_SOCKET:
				printf("Creating stack socket... family = %d, type = %d, proto = %d\n",
					args->u.socket.family, args->u.socket.type, args->u.socket.proto);
				status = core->socket_init(&cookie->socket);
				if (status == 0)
					status = core->socket_create(cookie->socket, args->u.socket.family, args->u.socket.type, args->u.socket.proto);
				break;

			case NET_STACK_GETSOCKOPT:
			case NET_STACK_SETSOCKOPT:
				if (command->op == (int32) NET_STACK_GETSOCKOPT) {
					status = core->socket_getsockopt(cookie->socket, args->u.sockopt.level, args->u.sockopt.option, 
						convert_to_local(&command->area[1], &area[1], args->u.sockopt.optval),
						(size_t *) &args->u.sockopt.optlen);
				} else {
					status = core->socket_setsockopt(cookie->socket, args->u.sockopt.level, args->u.sockopt.option, 
						(const void *) convert_to_local(&command->area[1], &area[1], args->u.sockopt.optval),
						args->u.sockopt.optlen);
				}
				break;

			case NET_STACK_CONNECT:
			case NET_STACK_BIND:
			case NET_STACK_GETSOCKNAME:
			case NET_STACK_GETPEERNAME: {
				caddr_t addr = (caddr_t) convert_to_local(&command->area[1], &area[1], args->u.sockaddr.addr);

				switch (command->op) {
					case NET_STACK_CONNECT:
						status = core->socket_connect(cookie->socket, addr, args->u.sockaddr.addrlen);
						break;
					case NET_STACK_BIND:
						status = core->socket_bind(cookie->socket, addr, args->u.sockaddr.addrlen);
						break;
					case NET_STACK_GETSOCKNAME:
						status = core->socket_getsockname(cookie->socket, (struct sockaddr *) addr, &args->u.sockaddr.addrlen);
						break;
					case NET_STACK_GETPEERNAME:
						status = core->socket_getpeername(cookie->socket,(struct sockaddr *) addr, &args->u.sockaddr.addrlen);
						break;
				}
				break;
			}
			case NET_STACK_LISTEN:
				status = core->socket_listen(cookie->socket, args->u.integer);
				break;

			case NET_STACK_GET_COOKIE:
				/* this is needed by accept() call, to be able to pass back
				 * in NET_STACK_ACCEPT opcode the cookie of the filedescriptor to 
				 * use for the new accepted socket
				 */
				*((void **) args) = cookie;
				break;
				
			case NET_STACK_ACCEPT:
			{
				connection_cookie *otherCookie = (connection_cookie *) args->u.accept.cookie;
				status = core->socket_accept(cookie->socket, &otherCookie->socket,
					convert_to_local(&command->area[1], &area[1], args->u.accept.addr),
					&args->u.accept.addrlen);
			}
			case NET_STACK_SEND:
			{
				struct iovec iov;
				int flags = 0;

				iov.iov_base = convert_to_local(&command->area[1], &area[1], args->u.transfer.data);
				iov.iov_len = args->u.transfer.datalen;

				status = core->socket_writev(cookie->socket, &iov, flags);
				break;
			}
			case NET_STACK_RECV:
			{
				struct iovec iov;
				int flags = 0;

				iov.iov_base = convert_to_local(&command->area[1], &area[1], args->u.transfer.data);
				iov.iov_len = args->u.transfer.datalen;

				/* flags gets ignored here... */
				status = core->socket_readv(cookie->socket, &iov, &flags);
				break;
			}
			case NET_STACK_RECVFROM:
			{
				struct msghdr *msg = (struct msghdr *) args;
				int received;

				msg->msg_name = convert_to_local(&command->area[1],&area[1],msg->msg_name);
				msg->msg_iov = convert_to_local(&command->area[2],&area[2],msg->msg_iov);
				msg->msg_control = convert_to_local(&command->area[3],&area[3],msg->msg_control);

				status = core->socket_recv(cookie->socket, msg, (caddr_t)&msg->msg_namelen,&received);
				if (status == 0)
					status = received;

				msg->msg_name = convert_to_foreign(&command->area[1],&area[1],msg->msg_name);
				msg->msg_iov = convert_to_foreign(&command->area[2],&area[2],msg->msg_iov);
				msg->msg_control = convert_to_foreign(&command->area[3],&area[3],msg->msg_control);
				break;
			}
			case NET_STACK_SENDTO:
			{
				struct msghdr *msg = (struct msghdr *) args;
				int sent;

				msg->msg_name = convert_to_local(&command->area[1],&area[1],msg->msg_name);
				msg->msg_iov = convert_to_local(&command->area[2],&area[2],msg->msg_iov);
				msg->msg_control = convert_to_local(&command->area[3],&area[3],msg->msg_control);
	
				status = core->socket_send(cookie->socket,msg,msg->msg_flags,&sent);
				if (status == 0)
					status = sent;

				msg->msg_name = convert_to_foreign(&command->area[1],&area[1],msg->msg_name);
				msg->msg_iov = convert_to_foreign(&command->area[2],&area[2],msg->msg_iov);
				msg->msg_control = convert_to_foreign(&command->area[3],&area[3],msg->msg_control);
				break;
			}

			case NET_STACK_NOTIFY_SOCKET_EVENT:
			{
				struct notify_socket_event_args *nsea = (struct notify_socket_event_args *) args;

				cookie->socket_event_port = nsea->notify_port;
				cookie->notify_cookie = nsea->cookie;
		
				if (cookie->socket_event_port != -1)
					// start notify socket event
					status = core->socket_set_event_callback(cookie->socket, on_socket_event, cookie, 0);
				else
					// stop notify socket event
					status = core->socket_set_event_callback(cookie->socket, NULL, NULL, 0);
				break;
			}

			case NET_STACK_SYSCTL:
			{
				status = core->net_sysctl(convert_to_local(&command->area[1],&area[1], args->u.sysctl.name),
					args->u.sysctl.namelen, convert_to_local(&command->area[2],&area[2],args->u.sysctl.oldp),
					convert_to_local(&command->area[3],&area[3],args->u.sysctl.oldlenp),
					convert_to_local(&command->area[4],&area[4],args->u.sysctl.newp),
					args->u.sysctl.newlen);
				break;
			}
/*
			case NET_STACK_STOP:
				core->stop();
				break;
*/				
			case NET_STACK_CONTROL_NET_MODULE:
				// TODO!
				break;

			case B_SET_BLOCKING_IO:
				cookie->openFlags &= ~O_NONBLOCK;
				break;

			case B_SET_NONBLOCKING_IO:
				cookie->openFlags |= O_NONBLOCK;
				break;

			case OSIOCGIFCONF:
			case SIOCGIFCONF:
			{
				struct ifconf *ifc = (struct ifconf *) args;
				ifc->ifc_buf = convert_to_local(&command->area[1], &area[1], ifc->ifc_buf);

				status = core->socket_ioctl(cookie->socket, command->op, (char *) args);

				ifc->ifc_buf = convert_to_foreign(&command->area[1], &area[1], ifc->ifc_buf);
				break;
			}

			default:
				status = core->socket_ioctl(cookie->socket,command->op, (char *) args);
				break;
		}
		// mark the command as done
		command->result = status;
		command->op = 0;
		delete_cloned_areas(area);

		// notify the command pipeline that we're done with the command
		release_sem(cookie->commandSemaphore);
	}

	cookie->runner = -1;
	shutdown_connection(cookie);

	return 0;
}


static status_t
init_connection(net_connection *connection, connection_cookie **_cookie)
{
	connection_cookie *cookie;
	net_command *commands;

	cookie = (connection_cookie *) malloc(sizeof(connection_cookie));
	if (cookie == NULL) {
		fprintf(stderr, "couldn't allocate memory for cookie.\n");
		return B_NO_MEMORY;
	}

	connection->area = create_area("net connection", (void *) &commands, B_ANY_ADDRESS,
			CONNECTION_BUFFER_SIZE + CONNECTION_COMMAND_SIZE,
			B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (connection->area < B_OK) {
		fprintf(stderr, "couldn't create area: %s.\n", strerror(connection->area));
		free(cookie);
		return connection->area;
	}
	memset(commands,0,NUM_COMMANDS * sizeof(net_command));

	connection->port = create_port(CONNECTION_QUEUE_LENGTH, "net stack connection");
	if (connection->port < B_OK) {
		fprintf(stderr, "couldn't create port: %s.\n", strerror(connection->port));
		delete_area(connection->area);
		free(cookie);
		return connection->port;
	}
	
	connection->commandSemaphore = create_sem(0, "net command queue");
	if (connection->commandSemaphore < B_OK) {
		fprintf(stderr, "couldn't create semaphore: %s.\n", strerror(connection->commandSemaphore));
		delete_area(connection->area);
		delete_port(connection->port);
		free(cookie);
		return connection->commandSemaphore;
	}

	cookie->runner = spawn_thread(connection_runner, "connection runner", B_NORMAL_PRIORITY, cookie);
	if (cookie->runner < B_OK) {
		fprintf(stderr, "couldn't create thread: %s.\n", strerror(cookie->runner));
		delete_sem(connection->commandSemaphore);
		delete_area(connection->area);
		delete_port(connection->port);
		free(cookie);
		return B_ERROR;
	}

	connection->socket_thread = cookie->runner;

	connection->numCommands = NUM_COMMANDS;
	connection->bufferSize = CONNECTION_BUFFER_SIZE;

	// setup connection cookie
	cookie->area = connection->area;
	cookie->commands = commands;
	cookie->buffer = (uint8 *)commands + CONNECTION_COMMAND_SIZE;
	cookie->commandSemaphore = connection->commandSemaphore;
	cookie->localPort = connection->port;
	cookie->openFlags = 0;
	
	cookie->socket_event_port = -1;
	cookie->notify_cookie = NULL;

	resume_thread(cookie->runner);

	*_cookie = cookie;
	return B_OK;
}


static void
shutdown_connection(connection_cookie *cookie)
{
	printf("free cookie: %p\n",cookie);
	kill_thread(cookie->runner);

	delete_port(cookie->localPort);
	delete_sem(cookie->commandSemaphore);
	delete_area(cookie->area);

	free(cookie);
}


static int32
connection_opener(void *_unused)
{
	while(true) {
		port_id port;
		int32 msg;
		ssize_t bytes = read_port(gStackPort, &msg, &port, sizeof(port_id));
		if (bytes < B_OK)
			return bytes;

		if (msg == (int32) NET_STACK_NEW_CONNECTION) {
			net_connection connection;
			connection_cookie *cookie;

			printf("incoming connection...\n");
			if (init_connection(&connection, &cookie) == B_OK)
				write_port(port, NET_STACK_NEW_CONNECTION, &connection, sizeof(net_connection));
		} else
			fprintf(stderr, "connection_opener: received unknown command: %lx (expected = %lx)\n",
				msg, (int32) NET_STACK_NEW_CONNECTION);
	}
	return 0;
}


status_t
init_userland_ipc(void)
{
	gStackPort = create_port(CONNECTION_QUEUE_LENGTH, NET_STACK_PORTNAME);
	if (gStackPort < B_OK)
		return gStackPort;

	gConnectionOpener = spawn_thread(connection_opener, "connection opener", B_NORMAL_PRIORITY, NULL);
	if (resume_thread(gConnectionOpener) < B_OK) {
		delete_port(gStackPort);
		if (gConnectionOpener >= B_OK) {
			kill_thread(gConnectionOpener);
			return B_BAD_THREAD_STATE;
		}
		return gConnectionOpener;
	}

	return B_OK;
}


void
shutdown_userland_ipc(void)
{
	delete_port(gStackPort);
	kill_thread(gConnectionOpener);
}


#ifdef COMMUNICATION_TEST
int
main(void)
{
	char buffer[8];

	if (init_userland_ipc() < B_OK)
		return -1;

	puts("Userland_ipc - test is running. Press <Return> to quit.");
	fgets(buffer, sizeof(buffer), stdin);

	shutdown_userland_ipc();

	return 0;
}
#endif	/* COMMUNICATION_TEST */
