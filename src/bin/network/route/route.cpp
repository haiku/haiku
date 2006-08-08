/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <SupportDefs.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


extern const char* __progname;
const char* kProgramName = __progname;


enum modes {
	RTM_LIST = 0,
	RTM_DELETE,
	RTM_ADD,
	
	// TODO:
	RTM_CHANGE,
	RTM_GET,
	RTM_FLUSH,
};

struct address_family {
	int			family;
	const char*	name;
	const char*	identifiers[4];
	bool		(*parse_address)(const char* string, sockaddr* _address);
	const char*	(*address_to_string)(sockaddr* address);
};

// AF_INET family
static bool inet_parse_address(const char* string, sockaddr* address);
static const char* inet_address_to_string(sockaddr* address);

static const address_family kFamilies[] = {
	{
		AF_INET,
		"inet",
		{"AF_INET", "inet", "ipv4", NULL},
		inet_parse_address,
		inet_address_to_string,
	},
	{ -1, NULL, {NULL}, NULL, NULL }
};


static bool
inet_parse_address(const char* string, sockaddr* _address)
{
	in_addr inetAddress;

	if (inet_aton(string, &inetAddress) != 1)
		return false;

	sockaddr_in& address = *(sockaddr_in *)_address;
	address.sin_family = AF_INET; 
	address.sin_len = sizeof(struct sockaddr_in);
	address.sin_port = 0;
	address.sin_addr = inetAddress;
	memset(&address.sin_zero[0], 0, sizeof(address.sin_zero));

	return true;
}


static const char *
inet_address_to_string(sockaddr* address)
{
	if (address == NULL || address->sa_family != AF_INET)
		return "-";

	return inet_ntoa(((sockaddr_in *)address)->sin_addr);
}


//	#pragma mark -


void
usage(int status)
{
	printf("usage: %s [command] [<interface>] [<address family>] <address|default> [<option/flags>...]\n"
		"Where <command> can be the one of:\n"
		"  add             - add a route for the specified interface\n"
		"  delete          - deletes the specified route\n"
		"  list            - list with filters [default]\n"
		"<option> can be the following:\n"
		"  netmask <addr>  - networking subnet mask\n"
		"  gw <addr>       - gateway address\n"
		"  mtu <bytes>     - maximal transfer unit\n"
		"And <flags> can be: reject, local, host\n\n"
		"Example:\n"
		"\t%s add /dev/net/ipro1000/0 default gw 192.168.0.254\n",
		kProgramName, kProgramName);

	exit(status);
}


bool
prepare_request(struct ifreq& request, const char* name)
{
	if (strlen(name) > IF_NAMESIZE) {
		fprintf(stderr, "%s: interface name \"%s\" is too long.\n", kProgramName, name);
		return false;
	}

	strcpy(request.ifr_name, name);
	return true;
}


bool
get_address_family(const char* argument, int32& familyIndex)
{
	for (int32 i = 0; kFamilies[i].family >= 0; i++) {
		for (int32 j = 0; kFamilies[i].identifiers[j]; j++) {
			if (!strcmp(argument, kFamilies[i].identifiers[j])) {
				// found a match
				familyIndex = i;
				return true;
			}
		}
	}

	// defaults to AF_INET
	familyIndex = 0;
	return false;
}


bool
parse_address(int32 familyIndex, const char* argument, struct sockaddr_storage& address)
{
	if (argument == NULL)
		return false;

	return kFamilies[familyIndex].parse_address(argument, (sockaddr *)&address);
}


//	#pragma mark -


void
list_routes(int socket, const char *interfaceName, route_entry &route)
{
	// get a list of all routes

	ifconf config;
	config.ifc_len = sizeof(config.ifc_value);
	if (ioctl(socket, SIOCGRTSIZE, &config, sizeof(struct ifconf)) < 0)
		return;

	uint32 size = (uint32)config.ifc_value;
	if (size == 0) {
		fprintf(stderr, "%s: There are no routes!\n", kProgramName);
		return;
	}

	void *buffer = malloc(size);
	if (buffer == NULL) {
		fprintf(stderr, "%s: Out of memory.\n", kProgramName);
		return;
	}

	config.ifc_len = size;
	config.ifc_buf = buffer;
	if (ioctl(socket, SIOCGRTTABLE, &config, sizeof(struct ifconf)) < 0)
		return;

	ifreq *interface = (ifreq *)buffer;
	ifreq *end = (ifreq *)((uint8 *)buffer + size);

	while (interface < end) {
		route_entry& route = interface->ifr_route;

		// apply filters
		if (interfaceName == NULL || !strcmp(interfaceName, interface->ifr_name)) {
			// find family
			const address_family *family = NULL;
			for (int32 i = 0; kFamilies[i].family >= 0; i++) {
				if (route.destination->sa_family == kFamilies[i].family) {
					family = &kFamilies[i];
					break;
				}
			}

			if (family != NULL) {
				printf("%15s ", family->address_to_string(route.destination));
				printf("mask %-15s ", family->address_to_string(route.mask));
	
				if (route.flags & RTF_GATEWAY)
					printf("gateway %-15s ", family->address_to_string(route.gateway));
			} else {
				printf("unknown family ");
			}

			printf("%s", interface->ifr_name);

			if (route.flags != 0) {
				const struct {
					int			value;
					const char	*name;
				} kFlags[] = {
					{RTF_DEFAULT, "default"},
					{RTF_REJECT, "reject"},
					{RTF_HOST, "host"},
					{RTF_LOCAL, "local"},
					{RTF_DYNAMIC, "dynamic"},
					{RTF_MODIFIED, "modified"},
				};
				bool first = true;

				for (uint32 i = 0; i < sizeof(kFlags) / sizeof(kFlags[0]); i++) {
					if ((route.flags & kFlags[i].value) != 0) {
						if (first) {
							printf(", ");
							first = false;
						} else
							putchar(' ');
						printf(kFlags[i].name);
					}
				}
			}

			putchar('\n');
		}

		int32 addressSize = 0;
		if (route.destination != NULL)
			addressSize += route.destination->sa_len;
		if (route.mask != NULL)
			addressSize += route.mask->sa_len;
		if (route.gateway != NULL)
			addressSize += route.gateway->sa_len;

		interface = (ifreq *)((addr_t)interface + IF_NAMESIZE + sizeof(route_entry)
			+ addressSize);
	}

	free(buffer);
}


void
delete_route(int socket, const char *interface, route_entry &route)
{
	ifreq request;
	if (!prepare_request(request, interface))
		return;

	request.ifr_route = route;

	if (ioctl(socket, SIOCDELRT, &request, sizeof(request)) < 0) {
		fprintf(stderr, "%s: Could not delete route for %s: %s\n",
			kProgramName, interface, strerror(errno));
	}
}


void
add_route(int socket, const char *interface, route_entry &route)
{
	ifreq request;
	if (!prepare_request(request, interface))
		return;

	route.flags |= RTF_STATIC;
	request.ifr_route = route;

	if (ioctl(socket, SIOCADDRT, &request, sizeof(request)) < 0) {
		fprintf(stderr, "%s: Could not add route for %s: %s\n",
			kProgramName, interface, strerror(errno));
	}
}


//	#pragma mark -


int
main(int argc, char** argv)
{
	if (argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")))
		usage(0);

	int32 mode = RTM_LIST;

	if (argc > 1) {
		if (!strcmp(argv[1], "delete")
			|| !strcmp(argv[1], "del")
			|| !strcmp(argv[1], "-d")) {
			// delete route
			if (argc < 3)
				usage(1);

			mode = RTM_DELETE;
		} else if (!strcmp(argv[1], "add")
			|| !strcmp(argv[1], "-a")) {
			// add route
			if (argc < 3)
				usage(1);

			mode = RTM_ADD;
		}
	}

	int32 i = 2;
	int32 interfaceIndex = i;
	bool familySpecified = false;
	int32 familyIndex = 0;
	const char *interface = NULL;
	sockaddr_storage destination;
	sockaddr_storage mask;
	sockaddr_storage gateway;
	bool hasDestination = false, hasGateway = false, hasMask = false;
	bool defaultRoute = false;

	route_entry route;
	memset(&route, 0, sizeof(route_entry));

	while (i < argc && i < 5) {
		// try to parse address family
		if (i <= 3 && familyIndex == -1 && get_address_family(argv[i], familyIndex)) {
			familySpecified = true;
			if (i == 2)
				interfaceIndex = -1;
		}

		if (!strcmp(argv[i], "default")) {
			defaultRoute = true;
			route.flags = RTF_DEFAULT;
			i++;
			break;
		} else if (parse_address(familyIndex, argv[i], destination)) {
			hasDestination = true;
			i++;
			break;
		}

		i++;
	}

	if (!defaultRoute && !hasDestination && mode != RTM_LIST)
		usage(1);

	if (i == 3)
		interfaceIndex = -1;
	if (interfaceIndex != -1 && interfaceIndex < argc)
		interface = argv[interfaceIndex];

	if (i < argc && parse_address(familyIndex, argv[i], mask)) {
		hasMask = true;
		i++;
	}

	// parse options and flags

	while (i < argc) {
		if (!strcmp(argv[i], "gw") || !strcmp(argv[i], "gateway")) {
			if (!parse_address(familyIndex, argv[i + 1], gateway)) {
				fprintf(stderr, "%s: Option 'gw' needs valid address parameter\n",
					kProgramName);
				exit(1);
			}
			hasGateway = true;
			i++;
		} else if (!strcmp(argv[i], "nm") || !strcmp(argv[i], "netmask")) {
			if (hasMask) {
				fprintf(stderr, "%s: Netmask is specified twice\n",
					kProgramName);
				exit(1);
			}
			if (!parse_address(familyIndex, argv[i + 1], mask)) {
				fprintf(stderr, "%s: Option 'netmask' needs valid address parameter\n",
					kProgramName);
				exit(1);
			}
			hasMask = true;
			i++;
		} else if (!strcmp(argv[i], "mtu")) {
			route.mtu = argv[i + 1] ? strtol(argv[i + 1], NULL, 0) : 0;
			if (route.mtu <= 500) {
				fprintf(stderr, "%s: Option 'mtu' exptected valid max transfer unit size\n",
					kProgramName);
				exit(1);
			}
			i++;
		} else if (!strcmp(argv[i], "host")) {
			route.flags |= RTF_HOST;
		} else if (!strcmp(argv[i], "local")) {
			route.flags |= RTF_LOCAL;
		} else if (!strcmp(argv[i], "reject")) {
			route.flags |= RTF_REJECT;
		} else
			usage(1);

		i++;
	}

	if (hasDestination)
		route.destination = (sockaddr *)&destination;
	if (hasMask)
		route.mask = (sockaddr *)&mask;
	if (hasGateway) {
		route.gateway = (sockaddr *)&gateway;
		route.flags |= RTF_GATEWAY;
	}

	// we need a socket to talk to the networking stack
	int socket = ::socket(kFamilies[familyIndex].family, SOCK_DGRAM, 0);
	if (socket < 0) {
		fprintf(stderr, "%s: The requested address family is not available.\n",
			kProgramName);
		return 1;
	}

	switch (mode) {
		case RTM_ADD:
			if (interface == NULL) {
				fprintf(stderr, "%s: You need to specify an interface when adding a route.\n",
					kProgramName);
				usage(1);
			}

			add_route(socket, interface, route);
			break;
		case RTM_DELETE:
			delete_route(socket, interface, route);
			break;

		case RTM_LIST:
			if (familySpecified)
				list_routes(socket, interface, route);
			else {
				for (int32 i = 0; kFamilies[i].family >= 0; i++) {
					int socket = ::socket(kFamilies[familyIndex].family, SOCK_DGRAM, 0);
					if (socket < 0)
						continue;

					list_routes(socket, interface, route);
					close(socket);
				}
			}
			break;
	}

	close(socket);
	return 0;
}

