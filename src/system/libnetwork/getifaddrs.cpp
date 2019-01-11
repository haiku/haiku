/*
 * Copyright 2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <algorithm>
#include <new>

#include <errno.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <unistd.h>

#include <AutoDeleter.h>

#include <ifaddrs.h>


static sockaddr*
copy_address(const sockaddr& address)
{
	if (address.sa_family == AF_UNSPEC)
		return NULL;

	sockaddr_storage* copy = new (std::nothrow) sockaddr_storage;
	if (copy == NULL)
		return NULL;

	size_t length = std::min(sizeof(sockaddr_storage), (size_t)address.sa_len);
	switch (address.sa_family) {
		case AF_INET:
			length = sizeof(sockaddr_in);
			break;
		case AF_INET6:
			length = sizeof(sockaddr_in6);
			break;
	}
	memcpy(copy, &address, length);
	copy->ss_len = length;
	return (sockaddr*)copy;
}


static int
_getifaddrs(int domain, char* buffer, size_t len, struct ifaddrs** previous)
{
	int socket = ::socket(domain, SOCK_DGRAM, 0);
	if (socket < 0)
		return -1;
	FileDescriptorCloser closer(socket);

	// Get interfaces configuration
	ifconf config;
	config.ifc_buf = buffer;
	config.ifc_len = len;
	if (ioctl(socket, SIOCGIFCONF, &config, sizeof(struct ifconf)) < 0)
		return -1;

	ifreq* interfaces = (ifreq*)buffer;
	ifreq* end = (ifreq*)(buffer + config.ifc_len);

	while (interfaces < end) {
		struct ifaddrs* current = new(std::nothrow) ifaddrs();
		if (current == NULL) {
			errno = B_NO_MEMORY;
			return -1;
		}

		// Chain this interface with the next one
		current->ifa_next = *previous;
		*previous = current;

		current->ifa_name = strdup(interfaces[0].ifr_name);
		current->ifa_addr = copy_address(interfaces[0].ifr_addr);
		current->ifa_netmask = NULL;
		current->ifa_dstaddr = NULL;
		current->ifa_data = NULL;

		ifreq request;
		strlcpy(request.ifr_name, interfaces[0].ifr_name, IF_NAMESIZE);

		if (ioctl(socket, SIOCGIFFLAGS, &request, sizeof(struct ifreq)) == 0)
			current->ifa_flags = request.ifr_flags;
		if (ioctl(socket, SIOCGIFNETMASK, &request, sizeof(struct ifreq))
				== 0) {
			current->ifa_netmask = copy_address(request.ifr_mask);
		}
		if (ioctl(socket, SIOCGIFDSTADDR, &request, sizeof(struct ifreq))
				== 0) {
			current->ifa_dstaddr = copy_address(request.ifr_dstaddr);
		}

		// Move on to next interface
		interfaces = (ifreq*)((uint8_t*)interfaces
			+ _SIZEOF_ADDR_IFREQ(interfaces[0]));
	}

	return 0;
}


/*! Returns a chained list of all interfaces. */
int
getifaddrs(struct ifaddrs** _ifaddrs)
{
	if (_ifaddrs == NULL) {
		errno = B_BAD_VALUE;
		return -1;
	}

	int socket = ::socket(AF_INET, SOCK_DGRAM, 0);
	if (socket < 0)
		return -1;

	FileDescriptorCloser closer(socket);

	// Get interface count
	ifconf config;
	config.ifc_len = sizeof(config.ifc_value);
	if (ioctl(socket, SIOCGIFCOUNT, &config, sizeof(struct ifconf)) < 0)
		return -1;

	socket = -1;
	closer.Unset();

	size_t count = (size_t)config.ifc_value;
	if (count == 0) {
		// No interfaces found
		*_ifaddrs = NULL;
		return 0;
	}

	// Allocate a buffer for ifreqs for all interfaces
	size_t buflen = count * sizeof(struct ifreq);
	char* buffer = (char*)malloc(buflen);
	if (buffer == NULL) {
		errno = B_NO_MEMORY;
		return -1;
	}

	MemoryDeleter deleter(buffer);

	struct ifaddrs* previous = NULL;
	int serrno = errno;
	if (_getifaddrs(AF_INET, buffer, buflen, &previous) < 0 &&
		errno != B_UNSUPPORTED) {
		freeifaddrs(previous);
		return -1;
	}
	if (_getifaddrs(AF_INET6, buffer, buflen, &previous) < 0 &&
		errno != B_UNSUPPORTED) {
		freeifaddrs(previous);
		return -1;
	}
	if (_getifaddrs(AF_LINK, buffer, buflen, &previous) < 0 &&
		errno != B_UNSUPPORTED) {
		freeifaddrs(previous);
		return -1;
	}
	if (previous == NULL)
		return -1;
	errno = serrno;

	*_ifaddrs = previous;
	return 0;
}


void
freeifaddrs(struct ifaddrs* ifa)
{
	while (ifa != NULL) {
		free((void*)ifa->ifa_name);
		delete ifa->ifa_addr;
		delete ifa->ifa_netmask;
		delete ifa->ifa_dstaddr;

		struct ifaddrs* next = ifa->ifa_next;
		delete ifa;
		ifa = next;
	}
}

