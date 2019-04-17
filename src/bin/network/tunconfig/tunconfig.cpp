/*
 * Copyright 2006-2019, Haiku, Inc. All rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *   Alexander von Gluck IV <kallisti5@unixzen.com>
 */


#include <stdio.h>
#include <String.h>

#include <NetworkInterface.h>
#include <NetworkRoster.h>


static
status_t
print_help()
{
	fprintf(stderr, "tunconfig\n");
	fprintf(stderr, "With tunconfig you can create and manage tun/tap devices.\n");
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "  tunconfig show | -a\n");
	fprintf(stderr, "  tunconfig init <name>\n");
	fprintf(stderr, "  tunconfig create <name>\n");
	fprintf(stderr, "  tunconfig delete <name|interface|id>\n");
	fprintf(stderr, "  tunconfig details <name|interface|id>\n");
	fprintf(stderr, "\t<name> must be an interface description file\n");
	
	return -1;
}


static
status_t
show_interface(const char* name)
{
	printf("%s\n", name);
	return B_OK;
}


static
status_t
show_all()
{
	BNetworkRoster& roster = BNetworkRoster::Default();

	BNetworkInterface interface;
	uint32 cookie = 0;

	while (roster.GetNextInterface(&cookie, interface) == B_OK) {
		BNetworkAddress linkAddress;
		status_t status = interface.GetHardwareAddress(linkAddress);
		if (status == B_OK && linkAddress.LinkLevelType() == IFT_TUN)
			show_interface(interface.Name());
	}
	return B_OK;
}


int
main(int argc, char *argv[])
{
	if (argc == 2) {
		if (!strcmp(argv[1], "show") || !strcmp(argv[1], "-a"))
			return show_all();
		else
			return print_help();
	} else {
		return print_help();
	}

	return 0;
}
