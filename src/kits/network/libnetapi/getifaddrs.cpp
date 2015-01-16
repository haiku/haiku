/*
 * Copyright 2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <AutoDeleter.h>
#include <NetworkAddress.h>
#include <NetworkInterface.h>
#include <NetworkRoster.h>

#include "compatibility/bsd/ifaddrs.h"


int getifaddrs(struct ifaddrs **ifap)
{
	if (ifap == NULL) {
		errno = B_BAD_VALUE;
		return -1;
	}

	BNetworkRoster& roster = BNetworkRoster::Default();

	uint32 cookie;

	struct ifaddrs* previous = NULL;
	struct ifaddrs* current = NULL;
	BNetworkInterface* interface = new BNetworkInterface();

	while (roster.GetNextInterface(&cookie, *interface) == B_OK) {
		BNetworkInterfaceAddress address;
		int32 i = 0;
		while (interface->GetAddressAt(i++, address) == B_OK) {
			current = new ifaddrs();

			// Chain this interface with the next one
			current->ifa_next = previous;
			previous = current;

			current->ifa_data = interface;
			current->ifa_name = interface->Name();
				// Points to the name in the BNetworkInterface instance, which
				// is added as ifa_data so freeifaddrs can release it.
			current->ifa_flags = address.Flags();
			current->ifa_addr = new sockaddr(address.Address().SockAddr());
			current->ifa_netmask = new sockaddr(address.Mask().SockAddr());
			current->ifa_dstaddr = new sockaddr(address.Destination().SockAddr());
		}

		interface = new BNetworkInterface();
	}

	delete interface;
	*ifap = current;

	return 0;
}


void freeifaddrs(struct ifaddrs *ifa)
{
	struct ifaddrs* next;
	BNetworkInterface* interface = NULL;
	while (ifa != NULL) {
		if (ifa->ifa_data != interface) {
			interface = (BNetworkInterface*)ifa->ifa_data;
			delete interface;
		}

		delete ifa->ifa_addr;
		delete ifa->ifa_netmask;
		delete ifa->ifa_dstaddr;

		next = ifa->ifa_next;
		delete ifa;
		ifa = next;
	}
}

