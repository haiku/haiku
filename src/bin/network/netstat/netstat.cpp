/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		James Woodcock
 */

#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <SupportDefs.h>

#include <net_stat.h>
#include <syscalls.h>


extern const char* __progname;
const char* kProgramName = __progname;

static int sResolveNames = 1;

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
		printf("%-22s", "-");
		return;
	}

	hostent* host = NULL;
	servent* service = NULL;
	if (sResolveNames) {
		host = gethostbyaddr((const char*)&address.sin_addr, sizeof(in_addr),
			AF_INET);
		service = getservbyport(address.sin_port, NULL);
	}

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

	printf("%-22s", buffer);
}


//	#pragma mark -


void
usage(int status)
{
	printf("usage: %s [-nh]\n", kProgramName);
	printf("options:\n");
	printf("	-n	don't resolve names\n");
	printf("	-h	this help\n");

	exit(status);
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
	int optionIndex = 0;
	int opt;
	static struct option longOptions[] = {
		{"help", no_argument, 0, 'h'},
		{"numeric", no_argument, 0, 'n'},
		{0, 0, 0, 0}
	};

	do {
		opt = getopt_long(argc, argv, "hn", longOptions, &optionIndex);
		switch (opt) {
			case -1:
				// end of arguments, do nothing
				break;

			case 'n':
				sResolveNames = 0;
				break;

			case 'h':
			default:
				usage(0);
				break;
		}
	} while (opt != -1);

	bool printProgram = true;
		// TODO: add some more program options... :-)

	printf("Proto  Recv-Q Send-Q Local Address         Foreign Address       "
		"State        Program\n");

	uint32 cookie = 0;
	int family = -1;
	net_stat stat;
	while (_kern_get_next_socket_stat(family, &cookie, &stat) == B_OK) {
		protoent* proto = getprotobynumber(stat.protocol);
		if (proto != NULL)
			printf("%-6s ", proto->p_name);
		else
			printf("%-6d ", stat.protocol);

		printf("%6lu ", stat.receive_queue_size);
		printf("%6lu ", stat.send_queue_size);

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

	return 0;
}

