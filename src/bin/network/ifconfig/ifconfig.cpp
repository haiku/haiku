/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Oliver Tappe, zooey@hirschkaefer.de
 *		Atis Elsts, the.kfx@gmail.com
 */


#include <Message.h>
#include <Messenger.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/if_types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/sockio.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <NetServer.h>


extern const char* __progname;
const char* kProgramName = __progname;


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
	void		(*print_address)(sockaddr* address);
};


bool initialize_address_families();

// AF_INET family
static bool inet_parse_address(const char* string, sockaddr* address);
static bool inet_prefix_length_to_mask(uint8 prefixLength, sockaddr* mask);
static uint8 inet_mask_to_prefix_length(sockaddr* mask);
static void inet_print_address(sockaddr* address);

// AF_INET6 family
static bool inet6_parse_address(const char* string, sockaddr* address);
static bool inet6_prefix_length_to_mask(uint8 prefixLength, sockaddr* mask);
static uint8 inet6_mask_to_prefix_length(sockaddr* mask);
static void inet6_print_address(sockaddr* address);

static const address_family kFamilies[] = {
	{
		AF_INET,
		"inet",
		{"AF_INET", "inet", "ipv4", NULL},
		PREFER_OUTPUT_MASK,
		inet_parse_address,
		inet_prefix_length_to_mask,
		inet_mask_to_prefix_length,
		inet_print_address
	},
	{
		AF_INET6,
		"inet6",
		{"AF_INET6", "inet6", "ipv6", NULL},
		PREFER_OUTPUT_PREFIX_LENGTH,
		inet6_parse_address,
		inet6_prefix_length_to_mask,
		inet6_mask_to_prefix_length,
		inet6_print_address
	},
	{ -1, NULL, {NULL}, PREFER_OUTPUT_MASK, NULL, NULL, NULL, NULL }
};


static int sAddressFamilySockets[sizeof(kFamilies) / sizeof(kFamilies[0])];


bool
initialize_address_families()
{
	bool ok = false;
	for (int32 i = 0; kFamilies[i].family >= 0; i++) {
		int fd = socket(kFamilies[i].family, SOCK_DGRAM, 0);
		if (fd != -1) {
			sAddressFamilySockets[i] = fd;
			ok = true;
		}
	}
	return ok;
}



static bool
inet_parse_address(const char* string, sockaddr* _address)
{
	in_addr inetAddress;

	if (inet_aton(string, &inetAddress) != 1)
		return false;

	sockaddr_in& address = *(sockaddr_in*)_address;
	address.sin_family = AF_INET;
	address.sin_len = sizeof(sockaddr_in);
	address.sin_port = 0;
	address.sin_addr = inetAddress;
	memset(&address.sin_zero[0], 0, sizeof(address.sin_zero));

	return true;
}


static bool
inet_prefix_length_to_mask(uint8 prefixLength, sockaddr* _mask)
{
	if (prefixLength > 32)
		return false;

	sockaddr_in& mask = *(sockaddr_in*)_mask;
	mask.sin_family = AF_INET;
	mask.sin_len = sizeof(sockaddr_in);
	mask.sin_port = 0;
	memset(&mask.sin_zero[0], 0, sizeof(mask.sin_zero));

	uint32 hostMask = 0;
	for (uint8 i = 32; i > 32 - prefixLength; i--)
		hostMask |= 1 << (i - 1);
	mask.sin_addr.s_addr = htonl(hostMask);

	return true;
}


static uint8
inet_mask_to_prefix_length(sockaddr* _mask)
{
	sockaddr_in& mask = *(sockaddr_in*)_mask;
	if (mask.sin_family != AF_INET)
		return (uint8)-1;

	uint8 result = 0;
	uint32 hostMask = ntohl(mask.sin_addr.s_addr);
	for (uint8 i = 32; i > 0; i--) {
		if (hostMask & (1 << (i - 1)) == 0)
			break;
		result++;
	}

	return result;
}


static void
inet_print_address(sockaddr* _address)
{
	sockaddr_in& address = *(sockaddr_in*)_address;

	if (address.sin_family != AF_INET)
		return;

	printf("%s", inet_ntoa(address.sin_addr));
}


static bool
inet6_parse_address(const char* string, sockaddr* _address)
{
	sockaddr_in6& address = *(sockaddr_in6*)_address;

	if (inet_pton(AF_INET6, string, &address.sin6_addr) != 1)
		return false;

	address.sin6_family = AF_INET6;
	address.sin6_len = sizeof(sockaddr_in6);
	address.sin6_port = 0;
	address.sin6_flowinfo = 0;
	address.sin6_scope_id = 0;

	return true;
}


static bool
inet6_prefix_length_to_mask(uint8 prefixLength, sockaddr* _mask)
{
	if (prefixLength > 128)
		return false;

	sockaddr_in6& mask = *(sockaddr_in6*)_mask;
	mask.sin6_family = AF_INET6;
	mask.sin6_len = sizeof(sockaddr_in6);
	mask.sin6_port = 0;
	mask.sin6_flowinfo = 0;
	mask.sin6_scope_id = 0;
	memset(mask.sin6_addr.s6_addr, 0, sizeof(in6_addr));

	for (uint8 i = 0; i < sizeof(in6_addr); i++, prefixLength -= 8) {
		if (prefixLength < 8) {
			const uint8 masks[] = {
				0x00, 0x80, 0xc0, 0xe0,
				0xf0, 0xf8, 0xfc, 0xfe
			};
			mask.sin6_addr.s6_addr[i] = masks[prefixLength];
			break;
		}

		mask.sin6_addr.s6_addr[i] = 0xff;
	}

	return true;
}


static uint8
inet6_mask_to_prefix_length(sockaddr* _mask)
{
	sockaddr_in6& mask = *(sockaddr_in6*)_mask;
	if (mask.sin6_family != AF_INET6)
		return (uint8)~0;

	uint8 result = 0;
	for (uint8 i = 0; i < sizeof(in6_addr); i++) {
		for (uint8 j = 0; j < 8; j++) {
			if (!(mask.sin6_addr.s6_addr[i] & (1 << j)))
				return result;
			result++;
		}
	}

	return 128;
}


static void
inet6_print_address(sockaddr* _address)
{
	sockaddr_in6& address = *(sockaddr_in6*)_address;

	if (address.sin6_family != AF_INET6)
		return;

	char buffer[INET6_ADDRSTRLEN];

	printf("%s",
		inet_ntop(AF_INET6, &address.sin6_addr, buffer, sizeof(buffer)));
}


//	#pragma mark -


struct media_type {
	int			type;
	const char*	name;
	const char* pretty;
	struct {
		int subtype;
		const char* name;
		const char* pretty;
	} subtypes [6];
	struct {
		int option;
		bool ro;
		const char* name;
		const char* pretty;
	} options [6];
};


static const media_type kMediaTypes[] = {
	{
		0, // for generic options
		"all",
		"All",
		{
			{ IFM_AUTO, "auto", "Auto-select" },
			{ -1, NULL, NULL }
		},
		{
			{ IFM_FULL_DUPLEX, true, "fullduplex", "Full Duplex" },
			{ IFM_HALF_DUPLEX, true, "halfduplex", "Half Duplex" },
			{ IFM_LOOP, true, "loop", "Loop" },
			//{ IFM_ACTIVE, false, "active", "Active" },
			{ -1, false, NULL, NULL }
		}
	},
	{
		IFM_ETHER,
		"ether",
		"Ethernet",
		{
			//{ IFM_AUTO, "auto", "Auto-select" },
			//{ IFM_AUI, "AUI", "10 MBit, AUI" },
			//{ IFM_10_2, "10base2", "10 MBit, 10BASE-2" },
			{ IFM_10_T, "10baseT", "10 MBit, 10BASE-T" },
			{ IFM_100_TX, "100baseTX", "100 MBit, 100BASE-TX" },
			{ IFM_1000_T, "1000baseT", "1 GBit, 1000BASE-T" },
			{ IFM_1000_SX, "1000baseSX", "1 GBit, 1000BASE-SX" },
			{ -1, NULL, NULL }
		},
		{
			{ -1, false, NULL, NULL }
		}
	},
	{ -1, NULL, NULL, {{ -1, NULL, NULL }}, {{ -1, false, NULL, NULL }} }
};


static bool media_parse_subtype(const char* string, int media, int* type);


static bool
media_parse_subtype(const char* string, int media, int* type)
{
	for (int32 i = 0; kMediaTypes[i].type >= 0; i++) {
		// only check for generic or correct subtypes
		if (kMediaTypes[i].type &&
			kMediaTypes[i].type != media)
			continue;
		for (int32 j = 0; kMediaTypes[i].subtypes[j].subtype >= 0; j++) {
			if (strcmp(kMediaTypes[i].subtypes[j].name, string) == 0) {
				// found a match
				*type = kMediaTypes[i].subtypes[j].subtype;
				return true;
			}
		}
	}
	return false;
}


//	#pragma mark -


void
usage(int status)
{
	printf("usage: %s [<interface> [<address family>] [<address> [<mask>] | "
			"auto-config] [<option/flags>...]]\n"
		"\t%s --delete interface [...]\n\n"
		"Where <option> can be the following:\n"
		"  netmask <addr>     - networking subnet mask\n"
		"  prefixlen <number> - subnet mask length in bits\n"
		"  broadcast <addr>   - set broadcast address\n"
		"  peer <addr>        - ppp-peer address\n"
		"  mtu <bytes>        - maximal transfer unit\n"
		"  metric <number>    - metric number to use (defaults to 0)\n"
		"  media <media>      - media type to use (defaults to auto)\n",
		kProgramName, kProgramName);
	for (int32 i = 0; kMediaTypes[i].type >= 0; i++) {
		printf("For %s <media> can be one of: ", kMediaTypes[i].pretty);
		for (int32 j = 0; kMediaTypes[i].subtypes[j].subtype >= 0; j++)
			printf("%s ", kMediaTypes[i].subtypes[j].name);
		printf("\n");
	}
	printf("And <flags> can be: up, down, [-]promisc, [-]allmulti, [-]bcast, "
			"loopback\n"
		"If you specify \"auto-config\" instead of an address, it will be "
			"configured automatically.\n\n"
		"Example:\n"
		"\t%s loop 127.0.0.1 255.0.0.0 up\n",
		kProgramName);

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

	memset(&request, 0, sizeof(ifreq));
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
parse_address(int32 familyIndex, const char* argument, struct sockaddr& address)
{
	if (argument == NULL)
		return false;

	return kFamilies[familyIndex].parse_address(argument, &address);
}


bool
prefix_length_to_mask(int32 familyIndex, const char* argument,
	struct sockaddr& mask)
{
	if (argument == NULL)
		return false;

	char *end;
	uint32 prefixLength = strtoul(argument, &end, 10);
	if (end == argument)
		return false;

	return kFamilies[familyIndex].prefix_length_to_mask(
		(uint8)prefixLength, &mask);
}


//	#pragma mark -


int
find_socket(struct ifreq& request, int addressFamily)
{
	int socket = -1;
	bool socketExists = false;
	bool ok = false;

	for (int32 i = 0; kFamilies[i].family >= 0; i++) {
		if (addressFamily != -1 && addressFamily != kFamilies[i].family)
			continue;

		socket = sAddressFamilySockets[i];
		if (socket == -1)
			continue;

		socketExists = true;

		if (ioctl(socket, SIOCGIFINDEX, &request, sizeof(request)) >= 0) {
			ok = true;
			break;
		}
	}

	if (socketExists && !ok) {
		fprintf(stderr, "%s: Interface \"%s\" does not exist.\n",
			kProgramName, request.ifr_name);
		return -1;
	}

	return socket;
}


void
list_interface_address(int socket, const address_family* family,
	uint32 flags, ifreq* request)
{
	if (ioctl(socket, SIOCGIFADDR, request, sizeof(struct ifreq)) < 0)
		return;

	printf("\t%s addr: ", family->name);
	family->print_address(&request->ifr_addr);

	if ((flags & IFF_BROADCAST) != 0
		&& ioctl(socket, SIOCGIFBRDADDR, request, sizeof(struct ifreq)) == 0
		&& request->ifr_broadaddr.sa_family == family->family) {
		printf(", Bcast: ");
		family->print_address(&request->ifr_broadaddr);
	}
	if (ioctl(socket, SIOCGIFNETMASK, request, sizeof(struct ifreq)) == 0
		&& request->ifr_mask.sa_family == family->family) {
		switch (family->preferred_format) {
			case PREFER_OUTPUT_MASK:
				printf(", Mask: ");
				family->print_address(&request->ifr_mask);
				break;
			case PREFER_OUTPUT_PREFIX_LENGTH:
				printf(", Prefix Length: %u",
					family->mask_to_prefix_length(&request->ifr_mask));
				break;
		}
	}
	putchar('\n');
}


bool
list_interface(const char* name, int addressFamily)
{
	ifreq request;
	if (!prepare_request(request, name))
		return true;

	int socket = find_socket(request, addressFamily);
	if (socket == -1)
		return false;

	printf("%s", name);
	size_t length = strlen(name);
	if (length < 8)
		putchar('\t');
	else
		printf("\n\t");

	// get link level interface for this interface

	int linkSocket = ::socket(AF_LINK, SOCK_DGRAM, 0);
	if (linkSocket < 0) {
		printf("No link level: %s\n", strerror(errno));
	} else {
		const char *type = "unknown";
		char address[256];
		strcpy(address, "none");

		if (ioctl(linkSocket, SIOCGIFADDR, &request, sizeof(struct ifreq))
				== 0) {
			sockaddr_dl &link = *(sockaddr_dl*)&request.ifr_addr;

			switch (link.sdl_type) {
				case IFT_ETHER:
				{
					type = "Ethernet";

					if (link.sdl_alen > 0) {
						uint8 *mac = (uint8*)LLADDR(&link);
						sprintf(address, "%02x:%02x:%02x:%02x:%02x:%02x",
							mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
					} else
						strcpy(address, "not available");
					break;
				}
				case IFT_LOOP:
					type = "Local Loopback";
					break;
				case IFT_MODEM:
					type = "Modem";
					break;
			}
		}

		printf("Hardware Type: %s, Address: %s\n", type, address);
		close(linkSocket);
	}

	if (ioctl(socket, SIOCGIFMEDIA, &request, sizeof(struct ifreq)) == 0
		&& (request.ifr_media & IFM_ACTIVE) != 0) {
		// dump media state in case we're linked
		const char* type = "unknown";
		bool show = false;

		for (int32 i = 0; kMediaTypes[i].type >= 0; i++) {
			// loopback don't really have a media anyway
			if (IFM_TYPE(request.ifr_media) == 0/*IFT_LOOP*/)
				break;
			// only check for generic or correct subtypes
			if (kMediaTypes[i].type &&
				kMediaTypes[i].type != IFM_TYPE(request.ifr_media))
				continue;
			for (int32 j = 0; kMediaTypes[i].subtypes[j].subtype >= 0; j++) {
				if (kMediaTypes[i].subtypes[j].subtype == IFM_SUBTYPE(request.ifr_media)) {
					// found a match
					type = kMediaTypes[i].subtypes[j].pretty;
					show = true;
					break;
				}
			}
		}

		if (show)
			printf("\tMedia Type: %s\n", type);
	}

	uint32 flags = 0;
	if (ioctl(socket, SIOCGIFFLAGS, &request, sizeof(struct ifreq)) == 0)
		flags = request.ifr_flags;

	for (int32 i = 0; kFamilies[i].family >= 0; i++) {
		int familySocket = sAddressFamilySockets[i];
		if (familySocket != -1) {
			list_interface_address(familySocket, &kFamilies[i],
				flags, &request);
		}
	}

	// Print MTU, metric, flags

	printf("\tMTU: ");
	if (ioctl(socket, SIOCGIFMTU, &request, sizeof(struct ifreq)) == 0)
		printf("%d", request.ifr_mtu);
	else
		printf("-");

	printf(", Metric: ");
	if (ioctl(socket, SIOCGIFMETRIC, &request, sizeof(struct ifreq)) == 0)
		printf("%d", request.ifr_metric);
	else
		printf("-");

	if (flags != 0) {
		const struct {
			int			value;
			const char	*name;
		} kFlags[] = {
			{IFF_UP, "up"},
			{IFF_NOARP, "noarp"},
			{IFF_BROADCAST, "broadcast"},
			{IFF_LOOPBACK, "loopback"},
			{IFF_PROMISC, "promiscuous"},
			{IFF_ALLMULTI, "allmulti"},
			{IFF_AUTOUP, "autoup"},
			{IFF_LINK, "link"},
			{IFF_AUTO_CONFIGURED, "auto-configured"},
			{IFF_CONFIGURING, "configuring"},
		};
		bool first = true;

		for (uint32 i = 0; i < sizeof(kFlags) / sizeof(kFlags[0]); i++) {
			if ((flags & kFlags[i].value) != 0) {
				if (first) {
					printf(",");
					first = false;
				}
				putchar(' ');
				printf(kFlags[i].name);
			}
		}
	}

	putchar('\n');

	// Print statistics

	if (ioctl(socket, SIOCGIFSTATS, &request, sizeof(struct ifreq)) == 0) {
		printf("\tReceive: %d packets, %d errors, %Ld bytes, %d mcasts, %d dropped\n",
			request.ifr_stats.receive.packets, request.ifr_stats.receive.errors,
			request.ifr_stats.receive.bytes, request.ifr_stats.receive.multicast_packets,
			request.ifr_stats.receive.dropped);
		printf("\tTransmit: %d packets, %d errors, %Ld bytes, %d mcasts, %d dropped\n",
			request.ifr_stats.send.packets, request.ifr_stats.send.errors,
			request.ifr_stats.send.bytes, request.ifr_stats.send.multicast_packets,
			request.ifr_stats.send.dropped);
		printf("\tCollisions: %d\n", request.ifr_stats.collisions);
	}

	putchar('\n');
	return true;
}


void
list_interfaces(const char* name)
{
	if (name != NULL) {
		list_interface(name, -1);
		return;
	}

	// get a list of all interfaces

	int socket = sAddressFamilySockets[0];

	ifconf config;
	config.ifc_len = sizeof(config.ifc_value);
	if (ioctl(socket, SIOCGIFCOUNT, &config, sizeof(struct ifconf)) < 0)
		return;

	uint32 count = (uint32)config.ifc_value;
	if (count == 0) {
		fprintf(stderr, "%s: There are no interfaces yet!\n", kProgramName);
		return;
	}

	void *buffer = malloc(count * sizeof(struct ifreq));
	if (buffer == NULL) {
		fprintf(stderr, "%s: Out of memory.\n", kProgramName);
		return;
	}

	config.ifc_len = count * sizeof(struct ifreq);
	config.ifc_buf = buffer;
	if (ioctl(socket, SIOCGIFCONF, &config, sizeof(struct ifconf)) < 0)
		return;

	ifreq* interface = (ifreq*)buffer;

	for (uint32 i = 0; i < count; i++) {
		list_interface(interface->ifr_name, interface->ifr_addr.sa_family);

		interface = (ifreq*)((uint8*)interface
			+ _SIZEOF_ADDR_IFREQ(interface[0]));
	}

	free(buffer);
}


/*!	If there are any arguments given, this will remove only the specified
	addresses from the interface named \a name.
	If there are no arguments, it will remove the complete interface with all
	of its addresses.
*/
void
delete_interface(const char* name, char* const* args, int32 argCount)
{
	ifreq request;
	if (!prepare_request(request, name))
		return;

	for (int32 i = 0; i < argCount; i++) {
		int32 familyIndex;
		if (get_address_family(args[i], familyIndex))
			i++;

		// TODO: be smart enough to detect the family by the address

		int socket = find_socket(request, kFamilies[familyIndex].family);
		if (socket == -1) {
			fprintf(stderr, "%s: Address family \"%s\" is not available.\n",
				kProgramName, kFamilies[familyIndex].name);
			exit(1);
		}

		if (!parse_address(familyIndex, args[i], request.ifr_addr)) {
			fprintf(stderr, "%s: Could not parse address \"%s\".\n",
				kProgramName, args[i]);
			exit(1);
		}

		if (ioctl(socket, SIOCDIFADDR, &request, sizeof(request)) < 0) {
			fprintf(stderr, "%s: Could not delete address %s from interface %s:"
				" %s\n", kProgramName, args[i], name, strerror(errno));
		}
	}

	if (argCount == 0) {
		// Delete interface
		int socket = find_socket(request, -1);
		if (socket < 0)
			exit(1);

		request.ifr_addr.sa_family = AF_UNSPEC;

		if (ioctl(socket, SIOCDIFADDR, &request, sizeof(request)) < 0) {
			fprintf(stderr, "%s: Could not delete interface %s: %s\n",
				kProgramName, name, strerror(errno));
		}
	}
}


void
configure_interface(const char* name, char* const* args,
	int32 argCount)
{
	ifreq request;
	if (!prepare_request(request, name))
		return;

	// try to parse address family

	int32 familyIndex;
	int32 i = 0;
	if (get_address_family(args[i], familyIndex))
		i++;

	int socket = sAddressFamilySockets[familyIndex];
	if (socket < 0) {
		fprintf(stderr, "%s: Address family \"%s\" is not available.\n",
			kProgramName, kFamilies[familyIndex].name);
		exit(1);
	}

	uint32 index = 0;
	if (ioctl(socket, SIOCGIFINDEX, &request, sizeof(request)) >= 0)
		index = request.ifr_index;

	bool hasAddress = false, hasMask = false, hasPeer = false;
	bool hasBroadcast = false, doAutoConfig = false;
	struct sockaddr address, mask, peer, broadcast;
	int mtu = -1, metric = -1, media = -1;
	int addFlags = 0, currentFlags = 0, removeFlags = 0;

	if (index == 0) {
		// the interface does not exist yet, we have to add it first
		ifaliasreq request;
		memset(&request, 0, sizeof(ifaliasreq));
		strlcpy(request.ifra_name, name, IF_NAMESIZE);

		if (ioctl(socket, SIOCAIFADDR, &request, sizeof(request)) < 0) {
			fprintf(stderr, "%s: Could not add interface: %s\n", kProgramName,
				strerror(errno));
			exit(1);
		}
	}

	// try to parse address

	if (parse_address(familyIndex, args[i], address)) {
		hasAddress = true;
		i++;

		if (parse_address(familyIndex, args[i], mask)) {
			hasMask = true;
			i++;
		}
	}

	// parse parameters and flags

	while (i < argCount) {
		if (!strcmp(args[i], "peer")) {
			if (!parse_address(familyIndex, args[i + 1], peer)) {
				fprintf(stderr, "%s: Option 'peer' needs valid address "
					"parameter\n", kProgramName);
				exit(1);
			}
			hasPeer = true;
			i++;
		} else if (!strcmp(args[i], "nm") || !strcmp(args[i], "netmask")) {
			if (hasMask) {
				fprintf(stderr, "%s: Netmask or prefix length is specified "
					"twice\n", kProgramName);
				exit(1);
			}
			if (!parse_address(familyIndex, args[i + 1], mask)) {
				fprintf(stderr, "%s: Option 'netmask' needs valid address "
					"parameter\n", kProgramName);
				exit(1);
			}
			hasMask = true;
			i++;
		} else if (!strcmp(args[i], "prefixlen") 
				|| !strcmp(args[i], "plen")) {
			if (hasMask) {
				fprintf(stderr, "%s: Netmask or prefix length is specified "
					"twice\n", kProgramName);
				exit(1);
			}
			if (!prefix_length_to_mask(familyIndex, args[i + 1], mask)) {
				fprintf(stderr, "%s: Option 'prefixlen' is invalid for this "
					"address family\n", kProgramName);
				exit(1);
			}
			hasMask = true;
			i++;
		} else if (!strcmp(args[i], "bc") || !strcmp(args[i], "broadcast")) {
			if (hasBroadcast) {
				fprintf(stderr, "%s: broadcast address is specified twice\n",
					kProgramName);
				exit(1);
			}
			if (!parse_address(familyIndex, args[i + 1], broadcast)) {
				fprintf(stderr, "%s: Option 'broadcast' needs valid address "
					"parameter\n", kProgramName);
				exit(1);
			}
			hasBroadcast = true;
			addFlags |= IFF_BROADCAST;
			i++;
		} else if (!strcmp(args[i], "mtu")) {
			mtu = args[i + 1] ? strtol(args[i + 1], NULL, 0) : 0;
			if (mtu <= 500) {
				fprintf(stderr, "%s: Option 'mtu' expected valid max transfer "
					"unit size\n", kProgramName);
				exit(1);
			}
			i++;
		} else if (!strcmp(args[i], "metric")) {
			if (i + 1 >= argCount) {
				fprintf(stderr, "%s: Option 'metric' exptected parameter\n",
					kProgramName);
				exit(1);
			}
			metric = strtol(args[i + 1], NULL, 0);
			i++;
		} else if (!strcmp(args[i], "media")) {
			if (ioctl(socket, SIOCGIFMEDIA, &request,
					sizeof(struct ifreq)) < 0) {
				fprintf(stderr, "%s: Unable to detect media type\n",
					kProgramName);
				exit(1);
			}
			if (i + 1 >= argCount) {
				fprintf(stderr, "%s: Option 'media' exptected parameter\n",
					kProgramName);
				exit(1);
			}
			if (!media_parse_subtype(args[i + 1], IFM_TYPE(request.ifr_media),
					&media)) {
				fprintf(stderr, "%s: Invalid parameter for option 'media': "
					"'%s'\n", kProgramName, args[i + 1]);
				exit(1);
			}
			i++;
		} else if (!strcmp(args[i], "up") || !strcmp(args[i], "-down")) {
			addFlags |= IFF_UP;
		} else if (!strcmp(args[i], "down") || !strcmp(args[i], "-up")) {
			removeFlags |= IFF_UP;
		} else if (!strcmp(args[i], "bcast")) {
			addFlags |= IFF_BROADCAST;
		} else if (!strcmp(args[i], "-bcast")) {
			removeFlags |= IFF_BROADCAST;
		} else if (!strcmp(args[i], "promisc")) {
			addFlags |= IFF_PROMISC;
		} else if (!strcmp(args[i], "-promisc")) {
			removeFlags |= IFF_PROMISC;
		} else if (!strcmp(args[i], "allmulti")) {
			addFlags |= IFF_ALLMULTI;
		} else if (!strcmp(args[i], "-allmulti")) {
			removeFlags |= IFF_ALLMULTI;
		} else if (!strcmp(args[i], "loopback")) {
			addFlags |= IFF_LOOPBACK;
		} else if (!strcmp(args[i], "auto-config")) {
			doAutoConfig = true;
		} else
			usage(1);

		i++;
	}

	if ((addFlags & removeFlags) != 0) {
		fprintf(stderr, "%s: Contradicting flags specified\n", kProgramName);
		exit(1);
	}

	if (doAutoConfig && (hasAddress || hasMask || hasBroadcast || hasPeer)) {
		fprintf(stderr, "%s: Contradicting changes specified\n", kProgramName);
		exit(1);
	}

	// set address/mask/broadcast/peer

	if (hasAddress) {
		memcpy(&request.ifr_addr, &address, address.sa_len);

		if (ioctl(socket, SIOCSIFADDR, &request, sizeof(struct ifreq)) < 0) {
			fprintf(stderr, "%s: Setting address failed: %s\n", kProgramName,
				strerror(errno));
			exit(1);
		}
	}

	if (ioctl(socket, SIOCGIFFLAGS, &request, sizeof(struct ifreq)) < 0) {
		fprintf(stderr, "%s: Getting flags failed: %s\n", kProgramName,
			strerror(errno));
		exit(1);
	}
	currentFlags = request.ifr_flags;

	if (hasMask) {
		memcpy(&request.ifr_mask, &mask, mask.sa_len);

		if (ioctl(socket, SIOCSIFNETMASK, &request, sizeof(struct ifreq)) < 0) {
			fprintf(stderr, "%s: Setting subnet mask failed: %s\n",
				kProgramName, strerror(errno));
			exit(1);
		}
	}

	if (hasBroadcast) {
		memcpy(&request.ifr_broadaddr, &broadcast, broadcast.sa_len);

		if (ioctl(socket, SIOCSIFBRDADDR, &request, sizeof(struct ifreq)) < 0) {
			fprintf(stderr, "%s: Setting broadcast address failed: %s\n",
				kProgramName, strerror(errno));
			exit(1);
		}
	}

	if (hasPeer) {
		memcpy(&request.ifr_dstaddr, &peer, peer.sa_len);

		if (ioctl(socket, SIOCSIFDSTADDR, &request, sizeof(struct ifreq)) < 0) {
			fprintf(stderr, "%s: Setting peer address failed: %s\n",
				kProgramName, strerror(errno));
			exit(1);
		}
	}

	// set flags

	if (hasAddress || hasMask || hasBroadcast || hasPeer)
		removeFlags = IFF_AUTO_CONFIGURED | IFF_CONFIGURING;

	if (addFlags || removeFlags) {
		request.ifr_flags = (currentFlags & ~removeFlags) | addFlags;
		if (ioctl(socket, SIOCSIFFLAGS, &request, sizeof(struct ifreq)) < 0) {
			fprintf(stderr, "%s: Setting flags failed: %s\n", kProgramName,
				strerror(errno));
		}
	}

	// set options

	if (mtu != -1) {
		request.ifr_mtu = mtu;
		if (ioctl(socket, SIOCSIFMTU, &request, sizeof(struct ifreq)) < 0) {
			fprintf(stderr, "%s: Setting MTU failed: %s\n", kProgramName,
				strerror(errno));
		}
	}

	if (metric != -1) {
		request.ifr_metric = metric;
		if (ioctl(socket, SIOCSIFMETRIC, &request, sizeof(struct ifreq)) < 0) {
			fprintf(stderr, "%s: Setting metric failed: %s\n", kProgramName,
				strerror(errno));
		}
	}

	if (media != -1) {
		request.ifr_media = media;
		if (ioctl(socket, SIOCSIFMEDIA, &request, sizeof(struct ifreq)) < 0) {
			fprintf(stderr, "%s: Setting media failed: %s\n", kProgramName,
				strerror(errno));
		}
	}

	// start auto configuration, if asked for

	if (doAutoConfig) {
		BMessage message(kMsgConfigureInterface);
		message.AddString("device", name);
		BMessage address;
		// TODO: this is not working for ipv6 yet
		address.AddString("family", "inet");
		address.AddBool("auto_config", true);
		message.AddMessage("address", &address);

		BMessenger networkServer(kNetServerSignature);
		if (networkServer.IsValid()) {
			BMessage reply;
			status_t status = networkServer.SendMessage(&message, &reply);
			if (status != B_OK) {
				fprintf(stderr, "%s: Sending auto-config message failed: %s\n",
					kProgramName, strerror(status));
			} else if (reply.FindInt32("status", &status) == B_OK
					&& status != B_OK) {
				fprintf(stderr, "%s: Auto-configuring failed: %s\n",
					kProgramName, strerror(status));
			}
		} else {
			fprintf(stderr, "%s: The net_server needs to run for the auto "
				"configuration!\n", kProgramName);
		}
	}
}


//	#pragma mark -


int
main(int argc, char** argv)
{
	if (argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")))
		usage(0);

	if (!initialize_address_families()) {
		fprintf(stderr, "%s: The networking stack doesn't seem to be "
			"available.\n", kProgramName);
		return 1;
	}

	if (argc > 1
		&& (!strcmp(argv[1], "--delete")
			|| !strcmp(argv[1], "--del")
			|| !strcmp(argv[1], "-d")
			|| !strcmp(argv[1], "del")
			|| !strcmp(argv[1], "delete"))) {
		// Delete interface (addresses)

		if (argc < 3)
			usage(1);

		const char* name = argv[2];
		delete_interface(name, argv + 3, argc - 3);
		return 0;
	}

	if (argc > 1 && !strcmp(argv[1], "-a")) {
		// Accept an optional "-a" option to list all interfaces for those
		// that are used to it from other platforms.

		if (argc > 2)
			usage(1);

		list_interfaces(NULL);
		return 0;
	}

	const char* name = argv[1];
	if (argc > 2) {
		// Add or configure an interface

		configure_interface(name, argv + 2, argc - 2);
		return 0;
	}

	// list interfaces

	list_interfaces(name);
	return 0;
}

