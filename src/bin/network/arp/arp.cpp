/*
 * Copyright 2006-2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <arp_control.h>

#include <generic_syscall_defs.h>
#include <syscalls.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


extern const char* __progname;
const char* kProgramName = __progname;


enum modes {
	ARP_LIST = 0,
	ARP_DELETE,
	ARP_SET,
	ARP_SET_FROM_FILE
};


static const char *
ethernet_address_to_string(uint8 *address)
{
	static char string[64];
	snprintf(string, sizeof(string), "%02x:%02x:%02x:%02x:%02x:%02x",
		address[0], address[1], address[2], address[3], address[4], address[5]);
	return string;
}


static bool
parse_ethernet_address(const char *string, uint8 *address)
{
	return sscanf(string, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &address[0], &address[1],
		&address[2], &address[3], &address[4], &address[5]) == 6;
}


static bool
parse_inet_address(const char* string, sockaddr_in& address)
{
	in_addr inetAddress;
	if (inet_aton(string, &inetAddress) != 1)
		return false;

	address.sin_family = AF_INET;
	address.sin_len = sizeof(struct sockaddr_in);
	address.sin_port = 0;
	address.sin_addr = inetAddress;
	memset(&address.sin_zero[0], 0, sizeof(address.sin_zero));

	return true;
}


static void
check_for_arp_syscall(void)
{
	uint32 version = 0;
	status_t status = _kern_generic_syscall(ARP_SYSCALLS, B_SYSCALL_INFO,
		&version, sizeof(version));
	if (status != B_OK) {
		fprintf(stderr, "\"ARP\" module not available.\n");
		exit(1);
	}
}


static void
usage(int status)
{
	printf("usage: %s [<command>] [<hostname>] [<ethernet-address>] [temp] [pub]\n"
		"       %s -f <filename>\n"
		"Where <command> can be the one of:\n"
		"  -s  - sets an entry in the ARP cache\n"
		"  -d  - deletes the specified ARP cache entry\n"
		"  -a  - list all entries [default]\n"
		"  -f  - read ARP entries from file\n"
		"  -F  - flush all temporary ARP entries\n"
		"The ethernet address is specified by six hex bytes separated by colons.\n"
		"When setting a new ARP cache entry, \"temp\" creates a temporary entry\n"
		"that will be removed after a timeout, and \"pub\" will cause ARP to\n"
		"answer to ARP requests for this entry.\n\n"
		"Example:\n"
		"\t%s -s 192.168.0.2 00:09:ab:cd:ef:12 pub\n",
		kProgramName, kProgramName, kProgramName);

	exit(status);
}


static bool
set_flags(uint32 &flags, const char *argument)
{
	if (!strcmp(argument, "temp") || !strcmp(argument, "temporary"))
		flags &= ~ARP_FLAG_PERMANENT;
	else if (!strcmp(argument, "pub") || !strcmp(argument, "publish"))
		flags |= ARP_FLAG_PUBLISH;
	else if (!strcmp(argument, "reject"))
		flags |= ARP_FLAG_REJECT;
	else
		return false;

	return true;
}


static char *
next_argument(char **_line)
{
	char *start = *_line;
	if (!start[0])
		return NULL;

	char *end = start;
	while (end[0] && !isspace(end[0])) {
		end++;
	}

	if (end[0]) {
		// skip all whitespace until next argument (or end of line)
		end[0] = '\0';
		end++;

		while (end[0] && isspace(end[0])) {
			end++;
		}
	}

	*_line = end;
	return start;
}


//	#pragma mark -


static void
list_entry(arp_control &control)
{
	in_addr address;
	address.s_addr = control.address;
	printf("%15s  %s", inet_ntoa(address),
		ethernet_address_to_string(control.ethernet_address));

	if (control.flags != 0) {
		const struct {
			int			value;
			const char	*name;
		} kFlags[] = {
			{ARP_FLAG_PERMANENT, "permanent"},
			{ARP_FLAG_LOCAL, "local"},
			{ARP_FLAG_PUBLISH, "publish"},
			{ARP_FLAG_REJECT, "reject"},
			{ARP_FLAG_VALID, "valid"},
		};
		bool first = true;

		for (uint32 i = 0; i < sizeof(kFlags) / sizeof(kFlags[0]); i++) {
			if ((control.flags & kFlags[i].value) != 0) {
				if (first) {
					printf("  ");
					first = false;
				} else
					putchar(' ');
				fputs(kFlags[i].name, stdout);
			}
		}
	}

	putchar('\n');
}


static void
list_entries(sockaddr_in *address)
{
	arp_control control;
	status_t status;

	if (address != NULL) {
		control.address = address->sin_addr.s_addr;
		status = _kern_generic_syscall(ARP_SYSCALLS, ARP_GET_ENTRY,
			&control, sizeof(arp_control));
		if (status != B_OK) {
			fprintf(stderr, "%s: ARP entry does not exist.\n", kProgramName);
			exit(1);
		}

		list_entry(control);
		return;
	}

	control.cookie = 0;
	while (_kern_generic_syscall(ARP_SYSCALLS, ARP_GET_ENTRIES,
			&control, sizeof(arp_control)) == B_OK) {
		list_entry(control);
	}
}


static void
delete_entry(sockaddr_in *address)
{
	arp_control control;
	control.address = address->sin_addr.s_addr;

	status_t status = _kern_generic_syscall(ARP_SYSCALLS, ARP_DELETE_ENTRY,
		&control, sizeof(arp_control));
	if (status != B_OK) {
		fprintf(stderr, "%s: Could not delete ARP entry: %s\n",
			kProgramName, strerror(status));
		exit(1);
	}
}


static status_t
set_entry(sockaddr_in *address, uint8 *ethernetAddress, uint32 flags)
{
	arp_control control;
	control.address = address->sin_addr.s_addr;
	memcpy(control.ethernet_address, ethernetAddress, ETHER_ADDRESS_LENGTH);
	control.flags = flags;

	return _kern_generic_syscall(ARP_SYSCALLS, ARP_SET_ENTRY,
		&control, sizeof(arp_control));
}


static int
set_entries_from_file(const char *filename)
{
	FILE *file = fopen(filename, "r");
	if (file == NULL) {
		fprintf(stderr, "%s: Could not open file: %s\n", kProgramName, strerror(errno));
		return 1;
	}

	int32 counter = 0;
	char line[4096];

	while (fgets(line, sizeof(line), file) != NULL) {
		counter++;

		// comments and empty lines are allowed
		if (line[0] == '#' || !strcmp(line, "\n"))
			continue;

		// parse hostname

		char *rest = line;
		const char *argument = next_argument(&rest);
		if (argument == NULL) {
			fprintf(stderr, "%s: Line %" B_PRId32 " is invalid (missing hostname).\n",
				kProgramName, counter);
			continue;
		}

		sockaddr_in address;
		if (!parse_inet_address(argument, address)) {
			// get host by name
			struct hostent *host = gethostbyname(argument);
			if (host == NULL) {
				fprintf(stderr, "%s: Line %" B_PRId32 ", host \"%s\" is not known in the IPv4 domain: %s\n",
					kProgramName, counter, argument, strerror(errno));
				continue;
			}
			if (host->h_addrtype != AF_INET) {
				fprintf(stderr, "%s: Line %" B_PRId32 ", host \"%s\" is not known in the IPv4 domain.\n",
					kProgramName, counter, argument);
				continue;
			}

			address.sin_family = AF_INET;
			address.sin_len = sizeof(sockaddr_in);
			address.sin_addr.s_addr = *(in_addr_t *)host->h_addr;
		}

		// parse ethernet MAC address

		argument = next_argument(&rest);
		if (argument == NULL) {
			fprintf(stderr, "%s: Line %" B_PRId32 " is invalid (missing ethernet address).\n",
				kProgramName, counter);
			continue;
		}

		uint8 ethernetAddress[6];
		if (!parse_ethernet_address(argument, ethernetAddress)) {
			fprintf(stderr, "%s: Line %" B_PRId32 ", \"%s\" is not a valid ethernet address.\n",
				kProgramName, counter, argument);
			continue;
		}

		// parse other options

		uint32 flags = ARP_FLAG_PERMANENT;
		while ((argument = next_argument(&rest)) != NULL) {
			if (!set_flags(flags, argument)) {
				fprintf(stderr, "%s: Line %" B_PRId32 ", ignoring invalid flag \"%s\".\n",
					kProgramName, counter, argument);
			}
		}

		status_t status = set_entry(&address, ethernetAddress, flags);
		if (status != B_OK) {
			fprintf(stderr, "%s: Line %" B_PRId32 ", ARP entry could not been set: %s\n",
				kProgramName, counter, strerror(status));
		}
	}

	fclose(file);
	return 0;
}


static void
flush_entries()
{
	arp_control control;

	_kern_generic_syscall(ARP_SYSCALLS, ARP_FLUSH_ENTRIES,
		&control, sizeof(arp_control));
}


//	#pragma mark -


int
main(int argc, char** argv)
{
	if (argc > 1 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")))
		usage(0);

	int32 mode = ARP_LIST;
	int32 i = 1;

	if (argc > 1 && argv[1][0] == '-') {
		if (!strcmp(argv[1], "-d")) {
			// delete entry
			if (argc != 3)
				usage(1);

			mode = ARP_DELETE;
		} else if (!strcmp(argv[1], "-s")) {
			// set entry
			if (argc < 4)
				usage(1);

			mode = ARP_SET;
		} else if (!strcmp(argv[1], "-a")) {
			// list all
			if (argc != 2)
				usage(1);

			mode = ARP_LIST;
		} else if (!strcmp(argv[1], "-F")) {
			// flush all entries
			if (argc != 2)
				usage(1);

			check_for_arp_syscall();
			flush_entries();
			return 0;
		} else if (!strcmp(argv[1], "-f")) {
			// set from file
			if (argc != 3)
				usage(1);

			check_for_arp_syscall();
			return set_entries_from_file(argv[2]);
		} else {
			fprintf(stderr, "%s: Unknown option %s\n", kProgramName, argv[1]);
			usage(1);
		}

		i++;
	}

	// parse hostname

	const char *hostname = argv[i];
	sockaddr_in address;

	if (hostname != NULL && !parse_inet_address(hostname, address)) {
		// get host by name
		struct hostent *host = gethostbyname(hostname);
		if (host == NULL) {
			fprintf(stderr, "%s: Host \"%s\" is not known in the IPv4 domain: %s\n",
				kProgramName, hostname, strerror(errno));
			return 1;
		}
		if (host->h_addrtype != AF_INET) {
			fprintf(stderr, "%s: Host \"%s\" is not known in the IPv4 domain.\n",
				kProgramName, hostname);
			return 1;
		}

		address.sin_family = AF_INET;
		address.sin_len = sizeof(sockaddr_in);
		address.sin_addr.s_addr = *(in_addr_t *)host->h_addr;
	}

	// parse other options in case of ARP_SET

	uint8 ethernetAddress[6];
	uint32 flags = ARP_FLAG_PERMANENT;

	if (mode == ARP_SET) {
		if (!parse_ethernet_address(argv[3], ethernetAddress)) {
			fprintf(stderr, "%s: \"%s\" is not a valid ethernet address.\n",
				kProgramName, argv[3]);
			return 1;
		}

		for (int32 i = 4; i < argc; i++) {
			if (!set_flags(flags, argv[i])) {
				fprintf(stderr, "%s: Flag \"%s\" is invalid.\n",
					kProgramName, argv[i]);
				return 1;
			}
		}
	}

	check_for_arp_syscall();

	switch (mode) {
		case ARP_SET:
		{
			status_t status = set_entry(&address, ethernetAddress, flags);
			if (status != B_OK) {
				fprintf(stderr, "%s: ARP entry could not been set: %s\n",
					kProgramName, strerror(status));
				return 1;
			}
			break;
		}
		case ARP_DELETE:
			delete_entry(&address);
			break;
		case ARP_LIST:
			list_entries(hostname ? &address : NULL);
			break;
	}

	return 0;
}

