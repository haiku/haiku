#include "argv.h"
#include "utility.h"

#include <net_buffer.h>
#include <net_datalink.h>
#include <net_protocol.h>
#include <net_socket.h>
#include <net_stack.h>

#include <KernelExport.h>
#include <module.h>

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
struct net_socket_module_info gNetSocketModule;

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
	NULL, //get_domain,

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


//	#pragma mark - datalink


struct /*net_datalink_*/module_info gNetDatalinkModule = {
		NET_DATALINK_MODULE_NAME,
		0,
		std_ops
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
	return B_ERROR;
}


status_t
domain_unbind(net_protocol *protocol, struct sockaddr *address)
{
	return B_ERROR;
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
	puts("domain send routed data!");
	return B_OK;
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
	puts("domain receive");
	return B_OK;
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


static void do_help(int argc, char** argv);

static cmd_entry sBuiltinCommands[] = {
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

	net_protocol_module_info* tcp;
	status = get_module("network/protocols/tcp/v1", (module_info **)&tcp);
	if (status < B_OK) {
		fprintf(stderr, "tcp_tester: Could not open TCP module: %s\n",
			strerror(status));
		return 1;
	}

	net_socket clientSocket, serverSocket;
	net_protocol* client = tcp->init_protocol(&clientSocket);
	net_protocol* server = tcp->init_protocol(&serverSocket);

	printf("*** Server: %p, Client: %p\n", server, client);

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

	tcp->uninit_protocol(server);
	tcp->uninit_protocol(client);

	put_module("network/protocols/tcp/v1");
	uninit_timers();
	return 0;
}
