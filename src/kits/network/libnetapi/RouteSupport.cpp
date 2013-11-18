/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <errno.h>
#include <net/if.h>
#include <sys/sockio.h>

#include <AutoDeleter.h>
#include <ObjectList.h>
#include <RouteSupport.h>


namespace BPrivate {


status_t
get_routes(const char* interfaceName, int family, BObjectList<route_entry>& routes)
{
	int socket = ::socket(family, SOCK_DGRAM, 0);
	if (socket < 0)
		return errno;

	FileDescriptorCloser fdCloser(socket);

	// Obtain gateway
	ifconf config;
	config.ifc_len = sizeof(config.ifc_value);
	if (ioctl(socket, SIOCGRTSIZE, &config, sizeof(struct ifconf)) < 0)
		return errno;

	uint32 size = (uint32)config.ifc_value;
	if (size == 0)
		return B_ERROR;

	void* buffer = malloc(size);
	if (buffer == NULL)
		return B_NO_MEMORY;

	MemoryDeleter bufferDeleter(buffer);
	config.ifc_len = size;
	config.ifc_buf = buffer;

	if (ioctl(socket, SIOCGRTTABLE, &config, sizeof(struct ifconf)) < 0)
		return errno;

	ifreq* interface = (ifreq*)buffer;
	ifreq* end = (ifreq*)((uint8*)buffer + size);

	while (interface < end) {
		route_entry& route = interface->ifr_route;
		if (interfaceName == NULL
			|| !strcmp(interface->ifr_name, interfaceName)) {
			route_entry* newRoute = new (std::nothrow) route_entry;
			if (newRoute == NULL)
				return B_NO_MEMORY;
			memcpy(newRoute, &interface->ifr_route, sizeof(route_entry));
			if (!routes.AddItem(newRoute)) {
				delete newRoute;
				return B_NO_MEMORY;
			}
		}

		int32 addressSize = 0;
		if (route.destination != NULL)
			addressSize += route.destination->sa_len;
		if (route.mask != NULL)
			addressSize += route.mask->sa_len;
		if (route.gateway != NULL)
			addressSize += route.gateway->sa_len;

		interface = (ifreq *)((addr_t)interface + IF_NAMESIZE
			+ sizeof(route_entry) + addressSize);
	}

	return B_OK;
}


}
