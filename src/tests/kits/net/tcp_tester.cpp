#include "argv.h"
#include "utility.h"

#include <net_buffer.h>
#include <net_datalink.h>
#include <net_protocol.h>
#include <net_socket.h>
#include <net_stack.h>

#include <KernelExport.h>
#include <module.h>

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct cmd_entry {
	char*	name;
	void	(*func)(int argc, char **argv);
	char*	help;
};


extern "C" status_t _add_builtin_module(module_info *info);

extern struct net_buffer_module_info gNetBufferModule;
	// from net_buffer.cpp
extern net_address_module_info gIPv4AddressModule;
	// from ipv4_address.cpp
extern module_info *modules[];
	// from tcp.cpp


extern struct net_protocol_module_info gDomainModule;
static struct net_protocol sDomainProtocol;
struct net_interface gInterface;
struct net_socket_module_info gNetSocketModule;
struct net_protocol_module_info *gTCPModule;
struct net_socket gServerSocket, gClientSocket;

static struct net_domain sDomain = {
	"ipv4",
	AF_INET,
	{},
	&gDomainModule,
	&gIPv4AddressModule
};


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


static net_stack_module_info gNetStackModule = {
	{
		NET_STACK_MODULE_NAME,
		0,
		std_ops
	},
	NULL, //register_domain,
	NULL, //unregister_domain,
	get_domain,

	register_domain_protocols,
	register_domain_datalink_protocols,
	register_domain_receiving_protocol,

	NULL, //get_domain_receiving_protocol,
	NULL, //put_domain_receiving_protocol,

	NULL, //register_device_deframer,
	NULL, //unregister_device_deframer,
	NULL, //register_domain_device_handler,
	NULL, //register_device_handler,
	NULL, //unregister_device_handler,
	NULL, //register_device_monitor,
	NULL, //unregister_device_monitor,
	NULL, //device_removed,

	notify_socket,

	checksum,

	init_fifo,
	uninit_fifo,
	fifo_enqueue_buffer,
	fifo_dequeue_buffer,
	clear_fifo,

	init_timer,
	set_timer,
};


//	#pragma mark - protocol/socket


net_protocol*
init_protocol(net_socket& socket)
{
	memset(&socket, 0, sizeof(net_socket));
	socket.family = AF_INET;
	socket.type = SOCK_STREAM;
	socket.protocol = IPPROTO_TCP;

	// set defaults (may be overridden by the protocols)
	socket.send.buffer_size = 65536;
	socket.send.low_water_mark = 1;
	socket.send.timeout = B_INFINITE_TIMEOUT;
	socket.receive.buffer_size = 65536;
	socket.receive.low_water_mark = 1;
	socket.receive.timeout = B_INFINITE_TIMEOUT;

	net_protocol* protocol = gTCPModule->init_protocol(&socket);
	if (protocol == NULL) {
		fprintf(stderr, "tcp_tester: cannot create protocol\n");
		return NULL;
	}

	socket.first_info = gTCPModule;
	socket.first_protocol = protocol;

	protocol->next = &sDomainProtocol;
	protocol->module = gTCPModule;
	protocol->socket = &socket;

	status_t status = gTCPModule->open(protocol);
	if (status < B_OK) {
		fprintf(stderr, "tcp_tester: cannot open client: %s\n", strerror(status));
		return NULL;
	}

	return protocol;
}


void
close_protocol(net_protocol* protocol)
{
	gTCPModule->close(protocol);
	gTCPModule->free(protocol);
	gTCPModule->uninit_protocol(protocol);
}


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
socket_listen(net_socket *socket, int backlog)
{
	status_t status = socket->first_info->listen(socket->first_protocol, backlog);
	if (status == B_OK)
		socket->options |= SO_ACCEPTCONN;

	return status;
}


//	#pragma mark - datalink


status_t
datalink_send_data(struct net_route *route, net_buffer *buffer)
{
	return sDomain.module->receive_data(buffer);
}


struct net_route *
get_route(struct net_domain *_domain, const struct sockaddr *address)
{
	static net_route route;
	route.interface = &gInterface;
	return &route;
}


net_datalink_module_info gNetDatalinkModule = {
	{
		NET_DATALINK_MODULE_NAME,
		0,
		std_ops
	},

	NULL, //datalink_control,
	datalink_send_data,

	NULL, //is_local_address,

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
domain_bind(net_protocol *protocol, struct sockaddr *address)
{
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
	puts("domain send data!");
	return B_OK;
}


status_t
domain_send_routed_data(net_protocol *protocol, struct net_route *route,
	net_buffer *buffer)
{
	printf("send routed buffer %p!\n", buffer);
	return sDomain.module->receive_data(buffer);
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
	return 1500;
}


status_t
domain_receive_data(net_buffer *buffer)
{
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
	NULL, // init
	NULL, // uninit
	domain_open,
	domain_close,
	domain_free,
	domain_connect,
	domain_accept,
	domain_control,
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
	domain_error,
	domain_error_reply,
};


//	#pragma mark - test


int32
server_thread(void *)
{
	while (true) {
		// main accept() loop
		net_socket* connectionSocket;
		sockaddr_in address;
		uint32 size = sizeof(struct sockaddr_in);
		status_t status = socket_accept(&gServerSocket, (struct sockaddr *)&address,
			&size, &connectionSocket);
		if (status < B_OK) {
			fprintf(stderr, "SERVER: accepting failed: %s\n", strerror(status));
			break;
		}

		printf("server: got connection from %ld\n", address.sin_addr.s_addr);
	}

	return 0;
}


static void do_help(int argc, char** argv);


static void
do_connect(int argc, char** argv)
{
	int port = 1024;
	if (argc > 1)
		port = atoi(argv[1]);

	sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = INADDR_ANY;

	status_t status = socket_connect(&gClientSocket, (struct sockaddr *)&address,
		sizeof(struct sockaddr));
	if (status < B_OK)
		fprintf(stderr, "tcp_tester: could not connect: %s\n", strerror(status));
}


static cmd_entry sBuiltinCommands[] = {
	{"connect", do_connect, "Connects the client"},
	{"help", do_help, "prints this help text"},
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


int
main(int argc, char** argv)
{
	status_t status = init_timers();
	if (status < B_OK) {
		fprintf(stderr, "tcp_tester: Could not initialize timers: %s\n",
			strerror(status));
		return 1;
	}

	_add_builtin_module((module_info *)&gNetStackModule);
	_add_builtin_module((module_info *)&gNetBufferModule);
	_add_builtin_module((module_info *)&gNetDatalinkModule);
	_add_builtin_module(modules[0]);

	sDomainProtocol.module = &gDomainModule;
	sockaddr_in interfaceAddress;
	interfaceAddress.sin_len = sizeof(sockaddr_in);
	interfaceAddress.sin_family = AF_INET;
	gInterface.address = (sockaddr*)&interfaceAddress;

	status = get_module("network/protocols/tcp/v1", (module_info **)&gTCPModule);
	if (status < B_OK) {
		fprintf(stderr, "tcp_tester: Could not open TCP module: %s\n",
			strerror(status));
		return 1;
	}

	net_protocol* client = init_protocol(gClientSocket);
	if (client == NULL)
		return 1;
	net_protocol* server = init_protocol(gServerSocket);
	if (server == NULL)
		return 1;

	printf("*** Server: %p, Client: %p\n", server, client);

	// setup server

	sockaddr_in address;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(1024);
	address.sin_addr.s_addr = INADDR_ANY;

	status = socket_bind(&gServerSocket, (struct sockaddr *)&address, sizeof(struct sockaddr));
	if (status < B_OK) {
		fprintf(stderr, "tcp_tester: cannot bind server: %s\n", strerror(status));
		return 1;
	}
	status = socket_listen(&gServerSocket, 40);
	if (status < B_OK) {
		fprintf(stderr, "tcp_tester: server cannot listen: %s\n", strerror(status));
		return 1;
	}

	thread_id serverThread = spawn_thread(server_thread, "server", B_NORMAL_PRIORITY, NULL);
	if (serverThread < B_OK) {
		fprintf(stderr, "tcp_tester: cannot start server: %s\n", strerror(serverThread));
		return 1;
	}

	resume_thread(serverThread);

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

		for (cmd_entry* command = sBuiltinCommands; command->name != NULL; command++) {
			if (!strncmp(command->name, argv[0], length)) {
				command->func(argc, argv);
				break;
			}
		}

		free(argv);
	}

	close_protocol(server);
	close_protocol(client);

	put_module("network/protocols/tcp/v1");
	uninit_timers();
	return 0;
}
