/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Oliver Tappe, zooey@hirschkaefer.de
 */


#include <SupportDefs.h>

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


extern const char* __progname;
const char* kProgramName = __progname;


struct address_family {
	int			family;
	const char*	name;
	const char*	identifiers[4];
	bool		(*parse_address)(const char* string, sockaddr* _address);
	void		(*print_address)(sockaddr* address);
};

// AF_INET family
static bool inet_parse_address(const char* string, sockaddr* address);
static void inet_print_address(sockaddr* address);

static const address_family kFamilies[] = {
	{
		AF_INET,
		"inet",
		{"AF_INET", "inet", "ipv4", NULL},
		inet_parse_address,
		inet_print_address
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


static void
inet_print_address(sockaddr* _address)
{
	sockaddr_in& address = *(sockaddr_in *)_address;
	
	if (address.sin_family != AF_INET)
		return;

	printf("%s", inet_ntoa(address.sin_addr));
}


//	#pragma mark -


void
usage(int status)
{
	printf("usage: %s [<interface> [<address family>] [<address> [<mask>]] [<option/flags>...]]\n"
		"\t%s --delete interface [...]\n\n"
		"Where <option> can be the following:\n"
		"  netmask <addr>   - networking subnet mask\n"
		"  broadcast <addr> - set broadcast address\n"
		"  peer <addr>      - ppp-peer address\n"
		"  mtu <bytes>      - maximal transfer unit\n"
		"  metric <number>  - metric number to use (defaults to 0)\n"
		"And <flags> can be: up, down, [-]promisc, [-]allmulti, [-]bcast, loopback\n\n"
		"Example:\n"
		"\t%s loop 127.0.0.1 255.0.0.0 up\n",
		kProgramName, kProgramName, kProgramName);

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
parse_address(int32 familyIndex, const char* argument, struct sockaddr& address)
{
	if (argument == NULL)
		return false;

	return kFamilies[familyIndex].parse_address(argument, &address);
}


//	#pragma mark -


void
list_interface(int socket, const char* name)
{
	ifreq request;
	if (!prepare_request(request, name))
		return;

	if (ioctl(socket, SIOCGIFINDEX, &request, sizeof(request)) < 0) {
		fprintf(stderr, "%s: Interface \"%s\" does not exist.\n", kProgramName, name);
		return;
	}

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

		if (ioctl(socket, SIOCGIFPARAM, &request, sizeof(struct ifreq)) == 0) {
			prepare_request(request, request.ifr_parameter.device);
			if (ioctl(linkSocket, SIOCGIFADDR, &request, sizeof(struct ifreq)) == 0) {
				sockaddr_dl &link = *(sockaddr_dl *)&request.ifr_addr;

				switch (link.sdl_type) {
					case IFT_ETHER:
					{
						type = "Ethernet";

						if (link.sdl_alen > 0) {
							uint8 *mac = (uint8 *)LLADDR(&link);
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
		}

		printf("Hardware Type: %s, Address: %s\n", type, address);
		close(linkSocket);
	}

	if (ioctl(socket, SIOCGIFMEDIA, &request, sizeof(struct ifreq)) == 0
		&& (request.ifr_media & IFM_ACTIVE) != 0) {
		// dump media state in case we're linked
		const char* type = "unknown";
		bool show = true;

		switch (IFM_TYPE(request.ifr_media)) {
			case IFM_ETHER:
				switch (IFM_SUBTYPE(request.ifr_media)) {
					case IFM_10_T:
						type = "10 MBit, 10BASE-T";
						break;
					case IFM_100_TX:
						type = "100 MBit, 100BASE-TX";
						break;
					case IFM_1000_T:
					case IFM_1000_SX:
						type = "1 GBit, 1000BASE-T";
						break;
				}
				break;

			default:
				show = false;
				break;
		}

		if (show)
			printf("\tMedia Type: %s\n", type);
	}

	uint32 flags = 0;
	if (ioctl(socket, SIOCGIFFLAGS, &request, sizeof(struct ifreq)) == 0)
		flags = request.ifr_flags;

	for (int32 i = 0; kFamilies[i].family >= 0; i++) {
		int familySocket = ::socket(kFamilies[i].family, SOCK_DGRAM, 0);
		if (familySocket < 0)
			continue;

		if (ioctl(familySocket, SIOCGIFADDR, &request, sizeof(struct ifreq)) == 0) {
			printf("\t%s addr: ", kFamilies[i].name);
			kFamilies[i].print_address(&request.ifr_addr);

			if ((flags & IFF_BROADCAST) != 0
				&& ioctl(familySocket, SIOCGIFBRDADDR, &request, sizeof(struct ifreq)) == 0
				&& request.ifr_broadaddr.sa_family == kFamilies[i].family) {
				printf(", Bcast: ");
				kFamilies[i].print_address(&request.ifr_broadaddr);
			}
			if (ioctl(familySocket, SIOCGIFNETMASK, &request, sizeof(struct ifreq)) == 0
				&& request.ifr_mask.sa_family == kFamilies[i].family) {
				printf(", Mask: ");
				kFamilies[i].print_address(&request.ifr_mask);
			}
			putchar('\n');
		}

		close(familySocket);
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
		printf("\tReceive: %ld packets, %ld errors, %Ld bytes, %ld mcasts, %ld dropped\n",
			request.ifr_stats.receive.packets, request.ifr_stats.receive.errors,
			request.ifr_stats.receive.bytes, request.ifr_stats.receive.multicast_packets,
			request.ifr_stats.receive.dropped);
		printf("\tTransmit: %ld packets, %ld errors, %Ld bytes, %ld mcasts, %ld dropped\n",
			request.ifr_stats.send.packets, request.ifr_stats.send.errors,
			request.ifr_stats.send.bytes, request.ifr_stats.send.multicast_packets,
			request.ifr_stats.send.dropped);
		printf("\tCollisions: %ld\n", request.ifr_stats.collisions);
	}

	putchar('\n');
}


void
list_interfaces(int socket, const char* name)
{
	if (name != NULL) {
		list_interface(socket, name);
		return;
	}

	// get a list of all interfaces

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

	ifreq *interface = (ifreq *)buffer;

	for (uint32 i = 0; i < count; i++) {
		list_interface(socket, interface->ifr_name);

		interface = (ifreq *)((addr_t)interface + IF_NAMESIZE + interface->ifr_addr.sa_len);
	}

	free(buffer);
}


void
delete_interface(int socket, const char* name)
{
	ifreq request;
	if (!prepare_request(request, name))
		return;

	if (ioctl(socket, SIOCDIFADDR, &request, sizeof(request)) < 0) {
		fprintf(stderr, "%s: Could not delete interface %s: %s\n",
			kProgramName, name, strerror(errno));
	}
}


void
configure_interface(int socket, const char* name, char* const* args,
	int32 argCount)
{
	ifreq request;
	if (!prepare_request(request, name))
		return;

	uint32 index = 0;
	if (ioctl(socket, SIOCGIFINDEX, &request, sizeof(request)) >= 0)
		index = request.ifr_index;

	bool hasAddress = false, hasMask = false, hasPeer = false, hasBroadcast = false;
	struct sockaddr address, mask, peer, broadcast;
	int mtu = -1, metric = -1, addFlags = 0, removeFlags = 0, currentFlags = 0;

	// try to parse address family

	int32 familyIndex;
	int32 i = 0;
	if (get_address_family(args[i], familyIndex))
		i++;

	if (kFamilies[familyIndex].family != AF_INET) {
		close(socket);

		// replace socket with one of the correct address family
		socket = ::socket(kFamilies[familyIndex].family, SOCK_DGRAM, 0);
		if (socket < 0) {
			fprintf(stderr, "%s: Address family \"%s\" is not available.\n",
				kProgramName, kFamilies[familyIndex].name);
			exit(1);
		}
	}

	if (index == 0) {
		// the interface does not exist yet, we have to add it first
		request.ifr_parameter.base_name[0] = '\0';
		request.ifr_parameter.device[0] = '\0';
		request.ifr_parameter.sub_type = 0;
			// the default device is okay for us

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
				fprintf(stderr, "%s: Option 'peer' needs valid address parameter\n",
					kProgramName);
				exit(1);
			}
			hasPeer = true;
			i++;
		} else if (!strcmp(args[i], "nm") || !strcmp(args[i], "netmask")) {
			if (hasMask) {
				fprintf(stderr, "%s: Netmask is specified twice\n",
					kProgramName);
				exit(1);
			}
			if (!parse_address(familyIndex, args[i + 1], mask)) {
				fprintf(stderr, "%s: Option 'netmask' needs valid address parameter\n",
					kProgramName);
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
				fprintf(stderr, "%s: Option 'broadcast' needs valid address parameter\n",
					kProgramName);
				exit(1);
			}
			hasBroadcast = true;
			addFlags |= IFF_BROADCAST;
			i++;
		} else if (!strcmp(args[i], "mtu")) {
			mtu = args[i + 1] ? strtol(args[i + 1], NULL, 0) : 0;
			if (mtu <= 500) {
				fprintf(stderr, "%s: Option 'mtu' expected valid max transfer unit size\n",
					kProgramName);
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
		} else
			usage(1);

		i++;
	}

	if ((addFlags & removeFlags) != 0) {
		fprintf(stderr, "%s: Contradicting flags specified\n", kProgramName);
		exit(1);
	}

	// set address/mask/broadcast/peer

	if (hasAddress) {
		memcpy(&request.ifr_addr, &address, address.sa_len);

		if (ioctl(socket, SIOCSIFADDR, &request, sizeof(struct ifreq)) < 0) {
			fprintf(stderr, "%s: Setting address failed: %s\n", kProgramName, strerror(errno));
			exit(1);
		}
	}

	if (ioctl(socket, SIOCGIFFLAGS, &request, sizeof(struct ifreq)) < 0) {
		fprintf(stderr, "%s: Getting flags failed: %s\n", kProgramName, strerror(errno));
		exit(1);
	}
	currentFlags = request.ifr_flags;

	if (!hasMask && hasAddress && kFamilies[familyIndex].family == AF_INET
		&& ioctl(socket, SIOCGIFNETMASK, &request, sizeof(struct ifreq)) == 0
		&& request.ifr_mask.sa_family == AF_UNSPEC) {
		// generate standard netmask if it doesn't have one yet
		sockaddr_in *netmask = (sockaddr_in *)&mask;
		netmask->sin_len = sizeof(sockaddr_in);
		netmask->sin_family = AF_INET;

		// choose default netmask depending on the class of the address
		in_addr_t net = ((sockaddr_in *)&address)->sin_addr.s_addr;
		if (IN_CLASSA(net)
			|| (ntohl(net) >> IN_CLASSA_NSHIFT) == IN_LOOPBACKNET) {
			// class A, or loopback
			netmask->sin_addr.s_addr = IN_CLASSA_NET;
		} else if (IN_CLASSB(net)) {
			// class B
			netmask->sin_addr.s_addr = IN_CLASSB_NET;
		} else {
			// class C and rest
			netmask->sin_addr.s_addr = IN_CLASSC_NET;
		}

		hasMask = true;
	}
	if (hasMask) {
		memcpy(&request.ifr_mask, &mask, mask.sa_len);

		if (ioctl(socket, SIOCSIFNETMASK, &request, sizeof(struct ifreq)) < 0) {
			fprintf(stderr, "%s: Setting subnet mask failed: %s\n", kProgramName, strerror(errno));
			exit(1);
		}
	}

	if (!hasBroadcast && hasAddress && (currentFlags & IFF_BROADCAST)
		&& kFamilies[familyIndex].family == AF_INET
		&& ioctl(socket, SIOCGIFBRDADDR, &request, sizeof(struct ifreq)) == 0
		&& request.ifr_mask.sa_family == AF_UNSPEC) {
			// generate standard broadcast address if it doesn't have one yet
		sockaddr_in *broadcastAddr = (sockaddr_in *)&broadcast;
		uint32 maskValue = ((sockaddr_in *)&mask)->sin_addr.s_addr;
		uint32 broadcastValue = ((sockaddr_in *)&address)->sin_addr.s_addr;
		broadcastValue = (broadcastValue & maskValue) | ~maskValue;
		broadcastAddr->sin_len = sizeof(sockaddr_in);
		broadcastAddr->sin_family = AF_INET;
		broadcastAddr->sin_addr.s_addr = broadcastValue;
		hasBroadcast = true;
	}
	if (hasBroadcast) {
		memcpy(&request.ifr_broadaddr, &broadcast, broadcast.sa_len);

		if (ioctl(socket, SIOCSIFBRDADDR, &request, sizeof(struct ifreq)) < 0) {
			fprintf(stderr, "%s: Setting broadcast address failed: %s\n", kProgramName, strerror(errno));
			exit(1);
		}
	}

	if (hasPeer) {
		memcpy(&request.ifr_dstaddr, &peer, peer.sa_len);

		if (ioctl(socket, SIOCSIFDSTADDR, &request, sizeof(struct ifreq)) < 0) {
			fprintf(stderr, "%s: Setting peer address failed: %s\n", kProgramName, strerror(errno));
			exit(1);
		}
	}

	// set flags

	if (addFlags || removeFlags) {
		request.ifr_flags = (currentFlags & ~removeFlags) | addFlags;
		if (ioctl(socket, SIOCSIFFLAGS, &request, sizeof(struct ifreq)) < 0)
			fprintf(stderr, "%s: Setting flags failed: %s\n", kProgramName, strerror(errno));
	}

	// set options

	if (mtu != -1) {
		request.ifr_mtu = mtu;
		if (ioctl(socket, SIOCSIFMTU, &request, sizeof(struct ifreq)) < 0)
			fprintf(stderr, "%s: Setting MTU failed: %s\n", kProgramName, strerror(errno));
	}

	if (metric != -1) {
		request.ifr_metric = metric;
		if (ioctl(socket, SIOCSIFMETRIC, &request, sizeof(struct ifreq)) < 0)
			fprintf(stderr, "%s: Setting metric failed: %s\n", kProgramName, strerror(errno));
	}
}


//	#pragma mark -


int
main(int argc, char** argv)
{
	if (argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")))
		usage(0);

	bool deleteInterfaces = false;

	if (argc > 1
		&& (!strcmp(argv[1], "--delete")
			|| !strcmp(argv[1], "--del")
			|| !strcmp(argv[1], "-d"))) {
		// delete interfaces
		if (argc < 3)
			usage(1);

		deleteInterfaces = true;
	}

	// we need a socket to talk to the networking stack
	int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (socket < 0) {
		fprintf(stderr, "%s: The networking stack doesn't seem to be available.\n",
			kProgramName);
		return 1;
	}

	if (deleteInterfaces) {
		for (int i = 2; i < argc; i++) {
			delete_interface(socket, argv[i]);
		}
		return 0;
	} else if (argc > 1 && !strcmp(argv[1], "-a")) {
		// accept -a option for those that are used to it from other platforms
		if (argc > 2)
			usage(1);

		list_interfaces(socket, NULL);
		return 0;
	}

	const char* name = argv[1];
	if (argc > 2) {
		// add or configure an interface

		configure_interface(socket, name, argv + 2, argc - 2);
		return 0;
	}

	// list interfaces

	list_interfaces(socket, name);
	return 0;
}

