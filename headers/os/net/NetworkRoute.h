/*
 * Copyright 2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NETWORK_ROUTE_H
#define _NETWORK_ROUTE_H

#include <net/route.h>

#include <ObjectList.h>


class BNetworkRoute {
public:
								BNetworkRoute();
								~BNetworkRoute();

		status_t				SetTo(const BNetworkRoute& other);
		status_t				SetTo(const route_entry& routeEntry);

		void					Adopt(BNetworkRoute& other);

		const route_entry&		RouteEntry() const;

		const sockaddr*			Destination() const;
		status_t				SetDestination(const sockaddr& destination);
		void					UnsetDestination();

		const sockaddr*			Mask() const;
		status_t				SetMask(const sockaddr& mask);
		void					UnsetMask();

		const sockaddr*			Gateway() const;
		status_t				SetGateway(const sockaddr& gateway);
		void					UnsetGateway();

		const sockaddr*			Source() const;
		status_t				SetSource(const sockaddr& source);
		void					UnsetSource();

		uint32					Flags() const;
		void					SetFlags(uint32 flags);

		uint32					MTU() const;
		void					SetMTU(uint32 mtu);

		int						AddressFamily() const;

static	status_t				GetDefaultRoute(int family,
									const char* interfaceName,
									BNetworkRoute& route);
static	status_t				GetDefaultGateway(int family,
									const char* interfaceName,
									sockaddr& gateway);

static	status_t				GetRoutes(int family,
									BObjectList<BNetworkRoute>& routes);
static	status_t				GetRoutes(int family, const char* interfaceName,
									BObjectList<BNetworkRoute>& routes);
static	status_t				GetRoutes(int family, const char* interfaceName,
									uint32 filterFlags,
									BObjectList<BNetworkRoute>& routes);

private:
								BNetworkRoute(const BNetworkRoute& other);
									// unimplemented to disallow copying

		status_t				_AllocateAndSetAddress(const sockaddr& from,
									sockaddr*& to);
		void					_FreeAndUnsetAddress(sockaddr*& address);

		route_entry				fRouteEntry;
};

#endif	// _NETWORK_ROUTE_H
