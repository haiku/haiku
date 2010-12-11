/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdio.h>
#include <stdlib.h>

#include <NetworkDevice.h>


extern const char* __progname;


void
usage()
{
	fprintf(stderr, "%s: <device> join|leave|list [<network> [<password>]]\n",
		__progname);
	exit(1);
}


const char*
get_authentication_mode(uint32 mode)
{
	switch (mode) {
		default:
		case B_NETWORK_AUTHENTICATION_NONE:
			return "-";
		case B_NETWORK_AUTHENTICATION_WEP:
			return "WEP";
		case B_NETWORK_AUTHENTICATION_WPA:
			return "WPA";
		case B_NETWORK_AUTHENTICATION_WPA2:
			return "WPA2";
	}
}


void
show(const wireless_network& network)
{
	printf("%-32s  %s  %4g dB %s C%02" B_PRIx32 " K%02" B_PRIx32
		" %s\n", network.name, network.address.ToString().String(),
		network.signal_strength / 2.0, get_authentication_mode(
			network.authentication_mode), network.cipher, network.key_mode,
		(network.flags & B_NETWORK_IS_ENCRYPTED) != 0 ? " (encrypted)" : "");
}


int
main(int argc, char** argv)
{
	if (argc < 2)
		usage();

	BNetworkDevice device(argv[1]);
	if (!device.Exists()) {
		fprintf(stderr, "\"%s\" does not exist!\n", argv[1]);
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
				BNetworkAddress link;
				status_t status = link.SetTo(AF_LINK, argv[3]);
				if (status == B_OK)
					status = device.GetNetwork(link, network);
				else
					status = device.GetNetwork(argv[3], network);
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
