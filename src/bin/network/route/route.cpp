/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


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

#include <NetworkAddress.h>


extern const char* __progname;
const char* kProgramName = __progname;


enum modes {
	RTM_LIST = 0,
	RTM_DELETE,
	RTM_ADD,
	RTM_GET,

	// TODO:
	RTM_CHANGE,
	RTM_FLUSH,
};

enum preferred_output_format {
	PREFER_OUTPUT_MASK,
	PREFER_OUTPUT_PREFIX_LENGTH,
};

struct address_family {
	int			family;
	const char*	name;
	const char*	identifiers[4];
	preferred_output_format	preferred_format;
	bool		(*parse_address)(const char* string, sockaddr* _address);
	bool		(*prefix_length_to_mask)(uint8 prefixLength, sockaddr* mask);
	uint8		(*mask_to_prefix_length)(sockaddr* mask);
	const char*	(*address_to_string)(sockaddr* address);
};


static const address_family kFamilies[] = {
	{
		AF_INET,
		"inet",
		{"AF_INET", "inet", "ipv4", NULL},
		PREFER_OUTPUT_MASK,
	},
	{
		AF_INET6,
		"inet6",
		{"AF_INET6", "inet6", "ipv6", NULL},
		PREFER_OUTPUT_PREFIX_LENGTH,
	},
	{ -1, NULL, {NULL}, PREFER_OUTPUT_MASK, NULL, NULL, NULL, NULL }
};


void
usage(int status)
{
	printf("usage: %s [command] [<interface>] [<address family>] "
			"<address|default> [<option/flags>...]\n"
		"Where <command> can be the one of:\n"
		"  add                - add a route for the specified interface\n"
		"  delete             - deletes the specified route\n"
		"  list               - list with filters [default]\n"
		"<option> can be the following:\n"
		"  netmask <addr>     - networking subnet mask\n"
		"  prefixlen <number> - subnet mask length in bits\n"
		"  gw <addr>          - gateway address\n"
		"  mtu <bytes>        - maximal transfer unit\n"
		"And <flags> can be: reject, local, host\n\n"
		"Example:\n"
		"\t%s add /dev/net/ipro1000/0 default gw 192.168.0.254\n",
		kProgramName, kProgramName);

	exit(status);
}


bool
prepare_request(struct ifreq& request, const char* name)
{
	if (strlen(name) >= IF_NAMESIZE) {
		fprintf(stderr, "%s: interface name \"%s\" is too long.\n",
			kProgramName, name);
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
parse_address(int32 familyIndex, const char* argument, BNetworkAddress& address)
{
	if (argument == NULL)
		return false;

	return address.SetTo(kFamilies[familyIndex].family, argument,
		(uint16)0, B_NO_ADDRESS_RESOLUTION) == B_OK;
}


bool
prefix_length_to_mask(int32 familyIndex, const char* argument,
	BNetworkAddress& mask)
{
	if (argument == NULL)
		return false;

	char* end;
	uint32 prefixLength = strtoul(argument, &end, 10);
	if (end == argument)
		return false;

	return mask.SetToMask(kFamilies[familyIndex].family, prefixLength) == B_OK;
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
	if (size == 0)
		return;

	void *buffer = malloc(size);
	if (buffer == NULL) {
		fprintf(stderr, "%s: Out of memory.\n", kProgramName);
		return;
	}

	config.ifc_len = size;
	config.ifc_buf = buffer;
	if (ioctl(socket, SIOCGRTTABLE, &config, sizeof(struct ifconf)) < 0)
		return;

	ifreq *interface = (ifreq*)buffer;
	ifreq *end = (ifreq*)((uint8*)buffer + size);

	while (interface < end) {
		route_entry& route = interface->ifr_route;

		// apply filters
		if (interfaceName == NULL
			|| !strcmp(interfaceName, interface->ifr_name)) {
			// find family
			const address_family *family = NULL;
			for (int32 i = 0; kFamilies[i].family >= 0; i++) {
				if (route.destination->sa_family == kFamilies[i].family) {
					family = &kFamilies[i];
					break;
				}
			}

			if (family != NULL) {
				BNetworkAddress destination(*route.destination);
				BNetworkAddress mask;
				if (route.mask != NULL)
					mask.SetTo(*route.mask);

				// TODO: is the %15s format OK for IPv6?
				printf("%15s", destination.ToString().String());
				switch (family->preferred_format) {
					case PREFER_OUTPUT_MASK:
						printf(" mask %-15s ", mask.ToString().String());
						break;
					case PREFER_OUTPUT_PREFIX_LENGTH:
						printf("/%zd ", mask.PrefixLength());
						break;
				}
				if ((route.flags & RTF_GATEWAY) != 0) {
					BNetworkAddress gateway;
					if (route.gateway != NULL)
						gateway.SetTo(*route.gateway);
					printf("gateway %-15s ", gateway.ToString().String());
				}
			} else
				printf("unknown family ");

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

				for (uint32 i = 0; i < sizeof(kFlags) / sizeof(kFlags[0]);
						i++) {
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

		interface = (ifreq*)((addr_t)interface + IF_NAMESIZE
			+ sizeof(route_entry) + addressSize);
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


void
get_route(int socket, route_entry &route)
{
	union {
		route_entry request;
		uint8 buffer[512];
	};

	request = route;

	if (ioctl(socket, SIOCGETRT, buffer, sizeof(buffer)) < 0) {
		fprintf(stderr, "%s: Could not get route: %s\n",
			kProgramName, strerror(errno));
		return;
	}

	// find family
	const address_family *family = NULL;
	for (int32 i = 0; kFamilies[i].family >= 0; i++) {
		if (route.destination->sa_family == kFamilies[i].family) {
			family = &kFamilies[i];
			break;
		}
	}

	if (family != NULL) {
		printf("%s", family->address_to_string(request.destination));
		switch (family->preferred_format) {
			case PREFER_OUTPUT_MASK:
				printf(" mask %s ",
					family->address_to_string(request.mask));
				break;
			case PREFER_OUTPUT_PREFIX_LENGTH:
				printf("/%u ",
					family->mask_to_prefix_length(request.mask));
				break;
		}

		if (request.flags & RTF_GATEWAY)
			printf("gateway %s ", family->address_to_string(request.gateway));

		printf("source %s\n", family->address_to_string(request.source));
	} else {
		printf("unknown family ");
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
		} else if (!strcmp(argv[1], "get")) {
			// get route for destination
			if (argc < 3)
				usage(1);

			mode = RTM_GET;
		}
	}

	int32 i = 2;
	int32 interfaceIndex = i;
	bool familySpecified = false;
	int32 familyIndex = 0;
	const char *interface = NULL;
	BNetworkAddress destination;
	BNetworkAddress mask;
	BNetworkAddress gateway;
	bool defaultRoute = false;

	route_entry route;
	memset(&route, 0, sizeof(route_entry));

	while (i < argc && i < 5) {
		// try to parse address family
		if (i <= 3 && familySpecified == false
				&& get_address_family(argv[i], familyIndex)) {
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
			i++;
			break;
		}

		i++;
	}

	if (!defaultRoute && destination.IsEmpty() && mode != RTM_LIST)
		usage(1);

	if (i == 3)
		interfaceIndex = -1;
	if (interfaceIndex != -1 && interfaceIndex < argc)
		interface = argv[interfaceIndex];

	if (i < argc && parse_address(familyIndex, argv[i], mask))
		i++;

	// parse options and flags

	while (i < argc) {
		if (!strcmp(argv[i], "gw") || !strcmp(argv[i], "gateway")) {
			if (!parse_address(familyIndex, argv[i + 1], gateway)) {
				fprintf(stderr, "%s: Option 'gw' needs valid address "
					"parameter\n", kProgramName);
				exit(1);
			}
			i++;
		} else if (!strcmp(argv[i], "nm") || !strcmp(argv[i], "netmask")) {
			if (!mask.IsEmpty()) {
				fprintf(stderr, "%s: Netmask or prefix length is specified "
					"twice\n", kProgramName);
				exit(1);
			}
			if (!parse_address(familyIndex, argv[i + 1], mask)) {
				fprintf(stderr, "%s: Option 'netmask' needs valid address "
					"parameter\n", kProgramName);
				exit(1);
			}
			i++;
		} else if (!strcmp(argv[i], "plen") || !strcmp(argv[i], "prefixlen")
			|| !strcmp(argv[i], "prefix-length")) {
			if (!mask.IsEmpty()) {
				fprintf(stderr, "%s: Netmask or prefix length is specified "
					"twice\n", kProgramName);
				exit(1);
			}
			if (!prefix_length_to_mask(familyIndex, argv[i + 1], mask)) {
				fprintf(stderr, "%s: Option 'prefixlen' is invalid for this "
					"address family\n", kProgramName);
				exit(1);
			}
			i++;
		} else if (!strcmp(argv[i], "mtu")) {
			route.mtu = argv[i + 1] ? strtol(argv[i + 1], NULL, 0) : 0;
			if (route.mtu <= 500) {
				fprintf(stderr, "%s: Option 'mtu' exptected valid max transfer "
					"unit size\n", kProgramName);
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

	if (!destination.IsEmpty())
		route.destination = (sockaddr*)destination;
	if (!mask.IsEmpty())
		route.mask = (sockaddr*)mask;
	if (!gateway.IsEmpty()) {
		route.gateway = (sockaddr*)gateway;
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
				fprintf(stderr, "%s: You need to specify an interface when "
					"adding a route.\n", kProgramName);
				usage(1);
			}

			add_route(socket, interface, route);
			break;
		case RTM_DELETE:
			if (interface == NULL) {
				fprintf(stderr, "%s: You need to specify an interface when "
					"removing a route.\n", kProgramName);
				usage(1);
			}

			delete_route(socket, interface, route);
			break;

		case RTM_LIST:
			if (familySpecified)
				list_routes(socket, interface, route);
			else {
				for (int32 i = 0; kFamilies[i].family >= 0; i++) {
					int socket = ::socket(kFamilies[i].family, SOCK_DGRAM, 0);
					if (socket < 0)
						continue;

					list_routes(socket, interface, route);
					close(socket);
				}
			}
			break;

		case RTM_GET:
			get_route(socket, route);
			break;
	}

	close(socket);
	return 0;
}

