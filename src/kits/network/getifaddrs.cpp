/*
 * Copyright 2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <ifaddrs.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sockio.h>
#include <unistd.h>

#include <AutoDeleter.h>
#include <NetworkAddress.h>
#include <NetworkInterface.h>
#include <NetworkRoster.h>


// This structure has an ifaddrs and all the information it refers to. We can
// allocate this in a single call to the allocator instead of allocating each
// address, the name, and the ifaddr struct itself separately. This simplifies
// the error handling and the freeing of the structure, and reduces the stress
// on the memory allocator.
struct IfaddrContainer {
	ifaddrs header;

	sockaddr_storage address;
	sockaddr_storage mask;
	sockaddr_storage destination;
	char name[0];
};


int
getifaddrs(struct ifaddrs **ifap)
{
	if (ifap == NULL) {
		errno = B_BAD_VALUE;
		return -1;
	}

	// get a list of all interfaces
	int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (socket < 0)
		return -1;

	FileDescriptorCloser closer(socket);

	ifconf config;
	config.ifc_len = sizeof(config.ifc_value);
	if (ioctl(socket, SIOCGIFCOUNT, &config, sizeof(struct ifconf)) < 0)
		return -1;

	size_t count = (size_t)config.ifc_value;
	if (count == 0) {
		errno = B_BAD_VALUE;
		return -1;
	}

	char* buffer = (char*)malloc(count * sizeof(struct ifreq));
	if (buffer == NULL) {
		errno = B_NO_MEMORY;
		return -1;
	}

	MemoryDeleter deleter(buffer);

	config.ifc_len = count * sizeof(struct ifreq);
	config.ifc_buf = buffer;
	if (ioctl(socket, SIOCGIFCONF, &config, sizeof(struct ifconf)) < 0)
		return -1;

	ifreq* interfaces = (ifreq*)buffer;
	ifreq* end = (ifreq*)(buffer + config.ifc_len);

	struct ifaddrs* previous = NULL;
	struct ifaddrs* current = NULL;

	for (uint32 i = 0; interfaces < end; i++) {
		BNetworkInterfaceAddress address;
		int32 j = 0;

		while (address.SetTo(interfaces->ifr_name, j++) == B_OK) {
			IfaddrContainer* container = (IfaddrContainer*)malloc(
				sizeof(IfaddrContainer) + strlen(interfaces->ifr_name) + 1);
			if (container == NULL) {
				freeifaddrs(previous);
				errno = B_NO_MEMORY;
				return -1;
			}

			current = &container->header;

			strcpy(container->name, interfaces->ifr_name);
			current->ifa_name = container->name;
			current->ifa_flags = address.Flags();

			memcpy(&container->address, &address.Address(),
				address.Address().sa_len);
			current->ifa_addr = (sockaddr*)&container->address;

			memcpy(&container->mask, &address.Mask(), address.Mask().sa_len);
			current->ifa_netmask = (sockaddr*)&container->mask;

			memcpy(&container->destination, &address.Destination(),
				address.Destination().sa_len);
			current->ifa_dstaddr = (sockaddr*)&container->destination;

			current->ifa_data = NULL;
				// Could point to extra information, if we have something to
				// add.

			// Chain this interface with the next one
			current->ifa_next = previous;
			previous = current;
		}

		interfaces = (ifreq*)((uint8*)interfaces
			+ _SIZEOF_ADDR_IFREQ(interfaces[0]));
	}

	*ifap = current;
	return B_OK;
}


void
freeifaddrs(struct ifaddrs *ifa)
{
	// Since each item was allocated as a single chunk using the IfaddrContainer,
	// all we need to do is free that.
	struct ifaddrs* next;
	while (ifa != NULL) {
		next = ifa->ifa_next;
		free(ifa);
		ifa = next;
	}
}

