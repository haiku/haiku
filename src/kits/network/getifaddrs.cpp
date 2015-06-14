/*
 * Copyright 2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <errno.h>
#include <net/if.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <unistd.h>

#include <new>

#include <AutoDeleter.h>

#include <ifaddrs.h>


int getifaddrs(struct ifaddrs **ifap)
{
	if (ifap == NULL) {
		errno = B_BAD_VALUE;
		return -1;
	}

#if 0
	uint32 cookie;
#endif

	// get a list of all interfaces

	int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (socket < 0)
		return errno;

	FileDescriptorCloser closer(socket);

	// get interface count
	ifconf config;
	config.ifc_len = sizeof(config.ifc_value);
	if (ioctl(socket, SIOCGIFCOUNT, &config, sizeof(struct ifconf)) < 0)
		return errno;

	size_t count = (size_t)config.ifc_value;
	if (count == 0)
		return B_BAD_VALUE;

	// allocate a buffer for ifreqs for all interfaces
	char* buffer = (char*)malloc(count * sizeof(struct ifreq));
	if (buffer == NULL)
		return B_NO_MEMORY;

	MemoryDeleter deleter(buffer);

	// get interfaces configuration
	config.ifc_len = count * sizeof(struct ifreq);
	config.ifc_buf = buffer;
	if (ioctl(socket, SIOCGIFCONF, &config, sizeof(struct ifconf)) < 0)
		return errno;

	ifreq* interfaces = (ifreq*)buffer;
	ifreq* end = (ifreq*)(buffer + config.ifc_len);

	struct ifaddrs* previous = NULL;
	struct ifaddrs* current = NULL;

	for (uint32_t i = 0; interfaces < end; i++) {
		current = new(std::nothrow) ifaddrs();
		if (current == NULL) {
			freeifaddrs(previous);
			errno = B_NO_MEMORY;
			return -1;
		}

		// Chain this interface with the next one
		current->ifa_next = previous;
		previous = current;

		current->ifa_name = strdup(interfaces[0].ifr_name);
		current->ifa_addr = new sockaddr(interfaces[0].ifr_addr);
		// still needs: flags, netmask, dst:broadcast addr

		ioctl(socket, SIOCGIFFLAGS, &interfaces[0], sizeof(struct ifconf));
		current->ifa_flags = interfaces[0].ifr_flags;
		ioctl(socket, SIOCGIFNETMASK, &interfaces[0], sizeof(struct ifconf));
		current->ifa_netmask = new sockaddr(interfaces[0].ifr_mask);
		ioctl(socket, SIOCGIFDSTADDR, &interfaces[0], sizeof(struct ifconf));
		current->ifa_dstaddr = new sockaddr(interfaces[0].ifr_dstaddr);

		// TODO we may also need to get other addresses for the same interface?
#if 0
		BNetworkInterfaceAddress address;
		int32 i = 0;
		while (interface->GetAddressAt(i++, address) == B_OK) {
			if (interface == NULL) {
				freeifaddrs(previous);
				errno = B_NO_MEMORY;
				return -1;
			}

			current = new(std::nothrow) ifaddrs();
			if (current == NULL) {
				freeifaddrs(previous);
				errno = B_NO_MEMORY;
				return -1;
			}

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

		interface = new(std::nothrow) BNetworkInterface();
#endif

		// move on to next interface
		interfaces = (ifreq*)((uint8_t*)interfaces
			+ _SIZEOF_ADDR_IFREQ(interfaces[0]));
	}

	*ifap = current;

	return 0;
}


void
freeifaddrs(struct ifaddrs *ifa)
{
	struct ifaddrs* next;
	while (ifa != NULL) {
		free ((void*)ifa->ifa_name);
		delete ifa->ifa_addr;
		delete ifa->ifa_netmask;
		delete ifa->ifa_dstaddr;

		next = ifa->ifa_next;
		delete ifa;
		ifa = next;
	}
}

