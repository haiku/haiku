/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>

#include <NetworkDevice.h>


extern const char* __progname;


void
usage()
{
	fprintf(stderr, "%s: <device> join|leave|list [<network> [<password>]]\n",
		__progname);
	exit(1);
}


void
show(const wireless_network& network)
{
	printf("%-32s  %s  %3g dB%s\n", network.name,
		network.address.ToString().String(), network.signal_strength / 2.0,
		(network.flags & B_NETWORK_IS_ENCRYPTED) != 0 ? " (encrypted)" : "");
}


int
main(int argc, char** argv)
{
	if (argc < 2)
		usage();

	BNetworkDevice device(argv[1]);
	if (!device.Exists()) {
		fprintf(stderr, "\"%s\" does not exit!\n", argv[1]);
		return 1;
	}
	if (!device.IsWireless()) {
		fprintf(stderr, "\"%s\" is not a WLAN device!\n", argv[1]);
		return 1;
	}

	if (argc > 2) {
		if (!strcmp(argv[2], "join")) {
			if (argc < 4)
				usage();
			const char* password = NULL;
			if (argc == 5)
				password = argv[4];

			status_t status = device.JoinNetwork(argv[3], password);
			if (status != B_OK) {
				fprintf(stderr, "joining network failed: %s\n",
					strerror(status));
				return 1;
			}
		} else if (!strcmp(argv[2], "leave")) {
			if (argc < 4)
				usage();

			status_t status = device.LeaveNetwork(argv[3]);
			if (status != B_OK) {
				fprintf(stderr, "leaving network failed: %s\n",
					strerror(status));
				return 1;
			}
		} else if (!strcmp(argv[2], "list")) {
			if (argc == 4) {
				// list the named entry
				wireless_network network;
				status_t status = device.GetNetwork(argv[3], network);
				if (status != B_OK) {
					fprintf(stderr, "getting network failed: %s\n",
						strerror(status));
					return 1;
				}
				show(network);
			} else {
				// list all
				wireless_network network;
				uint32 cookie = 0;
				while (device.GetNextNetwork(cookie, network) == B_OK)
					show(network);
			}
		} else
			usage();
	} else {
		// show associated networks
		wireless_network network;
		uint32 cookie = 0;
		while (device.GetNextAssociatedNetwork(cookie, network) == B_OK) {
			show(network);
		}
		if (cookie == 0)
			puts("no associated networks found.");
	}

	return 0;
}
