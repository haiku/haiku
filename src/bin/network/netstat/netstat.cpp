/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <SupportDefs.h>

#include <net_stack_driver.h>
#include <net_stat.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>

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
	void		(*print_address)(sockaddr* address);
};

// AF_INET family
static void inet_print_address(sockaddr* address);

static const address_family kFamilies[] = {
	{
		AF_INET,
		"inet",
		{"AF_INET", "inet", "ipv4", NULL},
		inet_print_address
	},
	{ -1, NULL, {NULL}, NULL }
};


static void
inet_print_address(sockaddr* _address)
{
	sockaddr_in& address = *(sockaddr_in *)_address;
	
	if (address.sin_family != AF_INET || address.sin_len == 0) {
		printf("%-30s", "-");
		return;
	}

	hostent* host = gethostbyaddr((const char*)_address, sizeof(sockaddr_in), AF_INET);
	servent* service = getservbyport(ntohs(address.sin_port), NULL);

	const char *hostName;
	if (host != NULL)
		hostName = host->h_name;
	else if (address.sin_addr.s_addr == INADDR_ANY)
		hostName = "*";
	else
		hostName = inet_ntoa(address.sin_addr);

	char buffer[128];
	int length = strlcpy(buffer, hostName, sizeof(buffer));

	char port[64];
	if (service != NULL)
		strlcpy(port, service->s_name, sizeof(port));
	else if (address.sin_port == 0)
		strcpy(port, "*");
	else
		snprintf(port, sizeof(port), "%u", ntohs(address.sin_port));

	snprintf(buffer + length, sizeof(buffer) - length, ":%s", port);

	printf("%-30s", buffer);
}


//	#pragma mark -


void
usage(int status)
{
	printf("usage: %s\n", kProgramName);

	exit(status);
}


status_t
get_next_stat(int stack, uint32& cookie, int family, net_stat& stat)
{
	get_next_stat_args args;
	args.cookie = cookie;
	args.family = family;

	if (ioctl(stack, NET_STACK_GET_NEXT_STAT, &args, sizeof(args)) < 0)
		return errno;

	cookie = args.cookie;
	memcpy(&stat, &args.stat, sizeof(net_stat));
	return B_OK;
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


//	#pragma mark -


int
main(int argc, char** argv)
{
	if (argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")))
		usage(0);

	int stack = open(NET_STACK_DRIVER_PATH, O_RDWR);
	if (stack < 0) {
		fprintf(stderr, "%s: The networking stack doesn't seem to be available.\n",
			kProgramName);
		return -1;
	}

	bool printProgram = true;
		// TODO: add some program options... :-)

	printf("Proto  Local Address                 Foreign Address               State        Program\n");

	uint32 cookie = 0;
	int family = -1;
	net_stat stat;
	while (get_next_stat(stack, cookie, family, stat) == B_OK) {
		protoent* proto = getprotobynumber(stat.protocol);
		if (proto != NULL)
			printf("%-6s ", proto->p_name);
		else
			printf("%-6d ", stat.protocol);

		inet_print_address((sockaddr*)&stat.address);
		inet_print_address((sockaddr*)&stat.peer);
		printf("%-12s ", stat.state);

		team_info info;
		if (printProgram && get_team_info(stat.owner, &info) == B_OK) {
			// remove arguments
			char* name = strchr(info.args, ' ');
			if (name != NULL)
				name[0] = '\0';

			// remove path name
			name = strrchr(info.args, '/');
			if (name != NULL)
				name++;
			else
				name = info.args;

			printf("%ld/%s\n", stat.owner, name);
		} else
			printf("%ld\n", stat.owner);
	}

	close(stack);
	return 0;
}

