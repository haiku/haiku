/*
 * Copyright 2013-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <NetworkRoute.h>

#include <errno.h>
#include <net/if.h>
#include <sys/sockio.h>

#include <AutoDeleter.h>


BNetworkRoute::BNetworkRoute()
{
	memset(&fRouteEntry, 0, sizeof(route_entry));
}


BNetworkRoute::~BNetworkRoute()
{
	UnsetDestination();
	UnsetMask();
	UnsetGateway();
	UnsetSource();
}


status_t
BNetworkRoute::SetTo(const BNetworkRoute& other)
{
	return SetTo(other.RouteEntry());
}


status_t
BNetworkRoute::SetTo(const route_entry& routeEntry)
{
	#define SET_ADDRESS(address, setFunction) \
		if (routeEntry.address != NULL) { \
			result = setFunction(*routeEntry.address); \
			if (result != B_OK) \
				return result; \
		}

	status_t result;
	SET_ADDRESS(destination, SetDestination)
	SET_ADDRESS(mask, SetMask)
	SET_ADDRESS(gateway, SetGateway)
	SET_ADDRESS(source, SetSource)

	SetFlags(routeEntry.flags);
	SetMTU(routeEntry.mtu);
	return B_OK;
}


void
BNetworkRoute::Adopt(BNetworkRoute& other)
{
	memcpy(&fRouteEntry, &other.fRouteEntry, sizeof(route_entry));
	memset(&other.fRouteEntry, 0, sizeof(route_entry));
}


const route_entry&
BNetworkRoute::RouteEntry() const
{
	return fRouteEntry;
}


const sockaddr*
BNetworkRoute::Destination() const
{
	return fRouteEntry.destination;
}


status_t
BNetworkRoute::SetDestination(const sockaddr& destination)
{
	return _AllocateAndSetAddress(destination, fRouteEntry.destination);
}


void
BNetworkRoute::UnsetDestination()
{
	_FreeAndUnsetAddress(fRouteEntry.destination);
}


const sockaddr*
BNetworkRoute::Mask() const
{
	return fRouteEntry.mask;
}


status_t
BNetworkRoute::SetMask(const sockaddr& mask)
{
	return _AllocateAndSetAddress(mask, fRouteEntry.mask);
}


void
BNetworkRoute::UnsetMask()
{
	_FreeAndUnsetAddress(fRouteEntry.mask);
}


const sockaddr*
BNetworkRoute::Gateway() const
{
	return fRouteEntry.gateway;
}


status_t
BNetworkRoute::SetGateway(const sockaddr& gateway)
{
	return _AllocateAndSetAddress(gateway, fRouteEntry.gateway);
}


void
BNetworkRoute::UnsetGateway()
{
	_FreeAndUnsetAddress(fRouteEntry.gateway);
}


const sockaddr*
BNetworkRoute::Source() const
{
	return fRouteEntry.source;
}


status_t
BNetworkRoute::SetSource(const sockaddr& source)
{
	return _AllocateAndSetAddress(source, fRouteEntry.source);
}


void
BNetworkRoute::UnsetSource()
{
	_FreeAndUnsetAddress(fRouteEntry.source);
}


uint32
BNetworkRoute::Flags() const
{
	return fRouteEntry.flags;
}


void
BNetworkRoute::SetFlags(uint32 flags)
{
	fRouteEntry.flags = flags;
}


uint32
BNetworkRoute::MTU() const
{
	return fRouteEntry.mtu;
}


void
BNetworkRoute::SetMTU(uint32 mtu)
{
	fRouteEntry.mtu = mtu;
}


int
BNetworkRoute::AddressFamily() const
{
	#define RETURN_FAMILY_IF_SET(address) \
		if (fRouteEntry.address != NULL \
			&& fRouteEntry.address->sa_family != AF_UNSPEC) { \
			return fRouteEntry.address->sa_family; \
		}

	RETURN_FAMILY_IF_SET(destination)
	RETURN_FAMILY_IF_SET(mask)
	RETURN_FAMILY_IF_SET(gateway)
	RETURN_FAMILY_IF_SET(source)

	return AF_UNSPEC;
}


status_t
BNetworkRoute::GetDefaultRoute(int family, const char* interfaceName,
	BNetworkRoute& route)
{
	BObjectList<BNetworkRoute> routes(1, true);
	status_t result = GetRoutes(family, interfaceName, RTF_DEFAULT, routes);
	if (result != B_OK)
		return result;

	if (routes.CountItems() == 0)
		return B_ENTRY_NOT_FOUND;

	route.Adopt(*routes.ItemAt(0));
	return B_OK;
}


status_t
BNetworkRoute::GetDefaultGateway(int family, const char* interfaceName,
	sockaddr& gateway)
{
	BNetworkRoute route;
	status_t result = GetDefaultRoute(family, interfaceName, route);
	if (result != B_OK)
		return result;

	const sockaddr* defaultGateway = route.Gateway();
	if (defaultGateway == NULL)
		return B_ENTRY_NOT_FOUND;

	memcpy(&gateway, defaultGateway, defaultGateway->sa_len);
	return B_OK;
}


status_t
BNetworkRoute::GetRoutes(int family, BObjectList<BNetworkRoute>& routes)
{
	return GetRoutes(family, NULL, 0, routes);
}


status_t
BNetworkRoute::GetRoutes(int family, const char* interfaceName,
	BObjectList<BNetworkRoute>& routes)
{
	return GetRoutes(family, interfaceName, 0, routes);
}


status_t
BNetworkRoute::GetRoutes(int family, const char* interfaceName,
	uint32 filterFlags, BObjectList<BNetworkRoute>& routes)
{
	int socket = ::socket(family, SOCK_DGRAM, 0);
	if (socket < 0)
		return errno;

	FileDescriptorCloser fdCloser(socket);

	ifconf config;
	config.ifc_len = sizeof(config.ifc_value);
	if (ioctl(socket, SIOCGRTSIZE, &config, sizeof(struct ifconf)) < 0)
		return errno;

	uint32 size = (uint32)config.ifc_value;
	if (size == 0)
		return B_OK;

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
		route_entry& routeEntry = interface->ifr_route;

		if ((interfaceName == NULL
				|| strcmp(interface->ifr_name, interfaceName) == 0)
			&& (filterFlags == 0 || (routeEntry.flags & filterFlags) != 0)) {

			BNetworkRoute* route = new(std::nothrow) BNetworkRoute;
			if (route == NULL)
				return B_NO_MEMORY;

			// Note that source is not provided in the buffer.
			routeEntry.source = NULL;

			status_t result = route->SetTo(routeEntry);
			if (result != B_OK) {
				delete route;
				return result;
			}

			if (!routes.AddItem(route)) {
				delete route;
				return B_NO_MEMORY;
			}
		}

		size_t addressSize = 0;
		if (routeEntry.destination != NULL)
			addressSize += routeEntry.destination->sa_len;
		if (routeEntry.mask != NULL)
			addressSize += routeEntry.mask->sa_len;
		if (routeEntry.gateway != NULL)
			addressSize += routeEntry.gateway->sa_len;

		interface = (ifreq *)((addr_t)interface + IF_NAMESIZE
			+ sizeof(route_entry) + addressSize);
	}

	return B_OK;
}


status_t
BNetworkRoute::_AllocateAndSetAddress(const sockaddr& from,
	sockaddr*& to)
{
	if (from.sa_len > sizeof(sockaddr_storage))
		return B_BAD_VALUE;

	if (to == NULL) {
		to = (sockaddr*)malloc(sizeof(sockaddr_storage));
		if (to == NULL)
			return B_NO_MEMORY;
	}

	memcpy(to, &from, from.sa_len);
	return B_OK;
}


void
BNetworkRoute::_FreeAndUnsetAddress(sockaddr*& address)
{
	free(address);
	address = NULL;
}
