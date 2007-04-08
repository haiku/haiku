/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "domains.h"
#include "routes.h"
#include "stack_private.h"
#include "utility.h"

#include <net_device.h>
#include <NetUtilities.h>

#include <lock.h>
#include <util/AutoLock.h>

#include <KernelExport.h>

#include <net/if_dl.h>
#include <net/route.h>
#include <new>
#include <stdlib.h>
#include <string.h>
#include <sys/sockio.h>


//#define TRACE_ROUTES
#ifdef TRACE_ROUTES
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif


net_route_private::net_route_private()
{
	destination = mask = gateway = NULL;
}


net_route_private::~net_route_private()
{
	free(destination);
	free(mask);
	free(gateway);
}


//	#pragma mark -


static status_t
user_copy_address(const sockaddr *from, sockaddr **to)
{
	if (from == NULL) {
		*to = NULL;
		return B_OK;
	}

	sockaddr address;
	if (user_memcpy(&address, from, sizeof(struct sockaddr)) < B_OK)
		return B_BAD_ADDRESS;

	*to = (sockaddr *)malloc(address.sa_len);
	if (*to == NULL)
		return B_NO_MEMORY;

	if (address.sa_len > sizeof(struct sockaddr)) {
		if (user_memcpy(*to, from, address.sa_len) < B_OK)
			return B_BAD_ADDRESS;
	} else
		memcpy(*to, &address, address.sa_len);

	return B_OK;
}


static status_t
user_copy_address(const sockaddr *from, sockaddr_storage *to)
{
	if (from == NULL)
		return B_BAD_ADDRESS;

	if (user_memcpy(to, from, sizeof(sockaddr)) < B_OK)
		return B_BAD_ADDRESS;

	if (to->ss_len > sizeof(sockaddr)) {
		if (to->ss_len > sizeof(sockaddr_storage))
			return B_BAD_VALUE;
		if (user_memcpy(to, from, to->ss_len) < B_OK)
			return B_BAD_ADDRESS;
	}

	return B_OK;
}

static net_route_private *
find_route(struct net_domain *_domain, const net_route *description)
{
	struct net_domain_private *domain = (net_domain_private *)_domain;
	RouteList::Iterator iterator = domain->routes.GetIterator();

	while (iterator.HasNext()) {
		net_route_private *route = iterator.Next();

		if ((route->flags & RTF_DEFAULT) != 0
			&& (description->flags & RTF_DEFAULT) != 0) {
			// there can only be one default route
			return route;
		}

		if ((route->flags & (RTF_GATEWAY | RTF_HOST | RTF_LOCAL)) ==
				(description->flags & (RTF_GATEWAY | RTF_HOST | RTF_LOCAL))
			&& domain->address_module->equal_masked_addresses(route->destination, 
				description->destination, description->mask)
			&& domain->address_module->equal_addresses(route->mask, 
				description->mask)
			&& domain->address_module->equal_addresses(route->gateway, 
				description->gateway))
			return route;
	}

	return NULL;
}


static net_route_private *
find_route(struct net_domain *_domain, const struct sockaddr *address)
{
	struct net_domain_private *domain = (net_domain_private *)_domain;

	// TODO: the following only works for IPv4 routes!
	if (domain->family != AF_INET)
		panic("you should have known better...");

	// find last matching route

	RouteList::Iterator iterator = domain->routes.GetIterator();
	TRACE(("test address %s for routes...\n", AddressString(domain, address).Data()));

	while (iterator.HasNext()) {
		net_route_private *route = iterator.Next();

		bool found;
 		if (route->mask != NULL) {
			sockaddr maskedAddress;
			domain->address_module->mask_address(address, route->mask, 
				&maskedAddress);
			found = domain->address_module->equal_addresses(&maskedAddress, 
				route->destination);
		} else {
			found = domain->address_module->equal_addresses(address, 
				route->destination);
 		}
 
		if (found) {
			TRACE(("  found route: %s, flags %lx\n",
				AddressString(domain, route->destination).Data(), route->flags));
			return route;
		}
	}

	return NULL;
}


static void
put_route_internal(struct net_domain_private *domain, net_route *_route)
{
	net_route_private *route = (net_route_private *)_route;
	if (route == NULL || atomic_add(&route->ref_count, -1) != 1)
		return;

	// remove route

	domain->routes.Remove(route);
	delete route;
}


struct net_route *
get_route_internal(struct net_domain_private *domain, const struct sockaddr *address)
{
	net_route_private *route = find_route(domain, address);
	if (route != NULL && atomic_add(&route->ref_count, 1) == 0) {
		// route has been deleted already
		route = NULL;
	}

	return route;
}


void
update_route_infos(struct net_domain_private *domain)
{
	RouteInfoList::Iterator iterator = domain->route_infos.GetIterator();

	while (iterator.HasNext()) {
		net_route_info *info = iterator.Next();
		
		put_route_internal(domain, info->route);
		info->route = get_route_internal(domain, &info->address);
	}
}


//	#pragma mark -


/*!
	Determines the size of a buffer large enough to contain the whole
	routing table.
*/
uint32
route_table_size(net_domain_private *domain)
{
	BenaphoreLocker locker(domain->lock);
	uint32 size = 0;

	RouteList::Iterator iterator = domain->routes.GetIterator();
	while (iterator.HasNext()) {
		net_route_private *route = iterator.Next();
		size += IF_NAMESIZE + sizeof(route_entry);

		if (route->destination)
			size += route->destination->sa_len;
		if (route->mask)
			size += route->mask->sa_len;
		if (route->gateway)
			size += route->gateway->sa_len;
	}

	return size;
}


/*!
	Dumps a list of all routes into the supplied userland buffer.
	If the routes don't fit into the buffer, an error (\c ENOBUFS) is
	returned.
*/
status_t
list_routes(net_domain_private *domain, void *buffer, size_t size)
{
	RouteList::Iterator iterator = domain->routes.GetIterator();
	size_t spaceLeft = size;

	sockaddr zeros;
	memset(&zeros, 0, sizeof(sockaddr));
	zeros.sa_family = domain->family;
	zeros.sa_len = sizeof(sockaddr);

	while (iterator.HasNext()) {
		net_route *route = iterator.Next();

		size = IF_NAMESIZE + sizeof(route_entry);

		sockaddr *destination = NULL;
		sockaddr *mask = NULL;
		sockaddr *gateway = NULL;
		uint8 *next = (uint8 *)buffer + size;

		if (route->destination != NULL) {
			destination = (sockaddr *)next;
			next += route->destination->sa_len;
			size += route->destination->sa_len;
		}
		if (route->mask != NULL) {
			mask = (sockaddr *)next;
			next += route->mask->sa_len;
			size += route->mask->sa_len;
		}
		if (route->gateway != NULL) {
			gateway = (sockaddr *)next;
			next += route->gateway->sa_len;
			size += route->gateway->sa_len;
		}

		if (spaceLeft < size)
			return ENOBUFS;

		ifreq request;
		strlcpy(request.ifr_name, route->interface->name, IF_NAMESIZE);
		request.ifr_route.destination = destination;
		request.ifr_route.mask = mask;
		request.ifr_route.gateway = gateway;
		request.ifr_route.mtu = route->mtu;
		request.ifr_route.flags = route->flags;

		if (user_memcpy(buffer, &request, size) < B_OK
			|| (route->destination != NULL && user_memcpy(request.ifr_route.destination, route->destination, route->destination->sa_len) < B_OK)
			|| (route->mask != NULL && user_memcpy(request.ifr_route.mask, route->mask, route->mask->sa_len) < B_OK)
			|| (route->gateway != NULL && user_memcpy(request.ifr_route.gateway, route->gateway, route->gateway->sa_len) < B_OK))
			return B_BAD_ADDRESS;

		buffer = (void *)next;
		spaceLeft -= size;
	}

	return B_OK;
}


status_t
control_routes(struct net_interface *interface, int32 option, void *argument, size_t length)
{
	net_domain_private *domain = (net_domain_private *)interface->domain;

	switch (option) {
		case SIOCADDRT:
		case SIOCDELRT:
		{
			// add or remove a route
			if (length != sizeof(struct ifreq))
				return B_BAD_VALUE;

			route_entry entry;
			if (user_memcpy(&entry, &((ifreq *)argument)->ifr_route, sizeof(route_entry)) != B_OK)
				return B_BAD_ADDRESS;

			net_route_private route;
			status_t status;
			if ((status = user_copy_address(entry.destination, &route.destination)) != B_OK
				|| (status = user_copy_address(entry.mask, &route.mask)) != B_OK
				|| (status = user_copy_address(entry.gateway, &route.gateway)) != B_OK)
				return status;

			route.mtu = entry.mtu;
			route.flags = entry.flags;
			route.interface = interface;

			if (option == SIOCADDRT)
				return add_route(domain, &route);

			return remove_route(domain, &route);
		}
	}
	return B_BAD_VALUE;
}


status_t
add_route(struct net_domain *_domain, const struct net_route *newRoute)
{
	struct net_domain_private *domain = (net_domain_private *)_domain;

	TRACE(("add route to domain %s: dest %s, mask %s, gw %s, flags %lx\n",
		domain->name,
		AddressString(domain, newRoute->destination ? newRoute->destination : NULL).Data(),
		AddressString(domain, newRoute->mask ? newRoute->mask : NULL).Data(),
		AddressString(domain, newRoute->gateway ? newRoute->gateway : NULL).Data(),
		newRoute->flags));

	if (domain == NULL || newRoute == NULL || newRoute->interface == NULL
		|| ((newRoute->flags & RTF_HOST) != 0 && newRoute->mask != NULL)
		|| ((newRoute->flags & RTF_DEFAULT) == 0 && newRoute->destination == NULL)
		|| ((newRoute->flags & RTF_GATEWAY) != 0 && newRoute->gateway == NULL)
		|| !domain->address_module->check_mask(newRoute->mask))
		return B_BAD_VALUE;

	net_route_private *route = find_route(domain, newRoute);
	if (route != NULL)
		return B_FILE_EXISTS;

	route = new (std::nothrow) net_route_private;
	if (route == NULL)
		return B_NO_MEMORY;

	if (domain->address_module->copy_address(newRoute->destination, 
		&route->destination, (newRoute->flags & RTF_DEFAULT) != 0, 
		newRoute->mask) != B_OK
		|| domain->address_module->copy_address(newRoute->mask, &route->mask,
				(newRoute->flags & RTF_DEFAULT) != 0, NULL) != B_OK
		|| domain->address_module->copy_address(newRoute->gateway, 
			&route->gateway, false, NULL) != B_OK) {
		delete route;
		return B_NO_MEMORY;
	}

	route->flags = newRoute->flags;
	route->interface = newRoute->interface;
	route->mtu = 0;
	route->ref_count = 1;

	// TODO: for now...
	//BenaphoreLocker locker(domain->lock);

	// Insert the route sorted by completeness of its mask

	RouteList::Iterator iterator = domain->routes.GetIterator();
	net_route_private *before = NULL;

	while ((before = iterator.Next()) != NULL) {
		// if the before mask is less specific than the one of the route, 
		// we can insert it before that route.
		if (domain->address_module->first_mask_bit(before->mask) 
			> domain->address_module->first_mask_bit(route->mask))
			break;
	}

	domain->routes.Insert(before, route);
	update_route_infos(domain);

	return B_OK;
}


status_t
remove_route(struct net_domain *_domain, const struct net_route *removeRoute)
{
	struct net_domain_private *domain = (net_domain_private *)_domain;

	TRACE(("remove route from domain %s: dest %s, mask %s, gw %s, flags %lx\n",
		domain->name,
		AddressString(domain, removeRoute->destination ? removeRoute->destination : NULL).Data(),
		AddressString(domain, removeRoute->mask ? removeRoute->mask : NULL).Data(),
		AddressString(domain, removeRoute->gateway ? removeRoute->gateway : NULL).Data(),
		removeRoute->flags));

	// TODO: for now...
	//BenaphoreLocker locker(domain->lock);

	net_route_private *route = find_route(domain, removeRoute);
	if (route == NULL)
		return B_ENTRY_NOT_FOUND;

	put_route_internal(domain, route);
	update_route_infos(domain);
	return B_OK;
}


static sockaddr *
copy_address(UserBuffer &buffer, sockaddr *address)
{
	if (address == NULL)
		return NULL;

	return (sockaddr *)buffer.Copy(address, address->sa_len);
}

static status_t
fill_route_entry(route_entry *target, void *_buffer, size_t bufferSize,
		 net_route *route)
{
	UserBuffer buffer(((uint8 *)_buffer) + sizeof(route_entry),
		bufferSize - sizeof(route_entry));

	target->destination = copy_address(buffer, route->destination);
	target->mask = copy_address(buffer, route->mask);
	target->gateway = copy_address(buffer, route->gateway);
	target->source = copy_address(buffer, route->interface->address);
	target->flags = route->flags;
	target->mtu = route->mtu;

	return buffer.Status();
}


status_t
get_route_information(struct net_domain *_domain, void *value, size_t length)
{
	struct net_domain_private *domain = (net_domain_private *)_domain;

	if (length < sizeof(route_entry))
		return B_BAD_VALUE;

	route_entry entry;
	if (user_memcpy(&entry, value, sizeof(route_entry)) < B_OK)
		return B_BAD_ADDRESS;

	sockaddr_storage destination;
	status_t status = user_copy_address(entry.destination, &destination);
	if (status != B_OK)
		return status;

	BenaphoreLocker locker(domain->lock);

	net_route_private *route = find_route(domain, (sockaddr *)&destination);
	if (route == NULL)
		return B_ENTRY_NOT_FOUND;

	status = fill_route_entry(&entry, value, length, route);
	if (status != B_OK)
		return status;

	return user_memcpy(value, &entry, sizeof(route_entry));
}


void
invalidate_routes(net_domain *_domain, net_interface *interface)
{
	// this function is called with the domain locked
	// (see domain_interface_went_down)
	net_domain_private *domain = (net_domain_private *)_domain;

	dprintf("invalidate_routes(%i, %s)\n", domain->family, interface->name);

	RouteList::Iterator iterator = domain->routes.GetIterator();
	while (iterator.HasNext()) {
		net_route *route = iterator.Next();

		// TODO If we are removing the interface this will bork.
		//      Consider the following case:
		//      [thread 1] ipv4_send_data()
		//      [thread 1]  get_route() [domain locked, unlocked] <- route
		//      [thread 2] ... [domain locked]
		//      [thread 2]  invalidate_routes()
		//      [thread 2]   remove_route() <- route
		//      [thread 1] ... ipv4_send_data() accesses `route'. Bork bork.
		//
		//      We could either add per-route locks (expensive) or
		//      lock the domain throughout the send_data() routine.
		//      These are the easy solutions, need to think about this. -hugo

		if (route->interface == interface)
			remove_route(domain, route);
	}
}


struct net_route *
get_route(struct net_domain *_domain, const struct sockaddr *address)
{
	struct net_domain_private *domain = (net_domain_private *)_domain;
	BenaphoreLocker locker(domain->lock);

	return get_route_internal(domain, address);
}


void
put_route(struct net_domain *_domain, net_route *route)
{
	struct net_domain_private *domain = (net_domain_private *)_domain;
	BenaphoreLocker locker(domain->lock);

	put_route_internal(domain, (net_route *)route);
}


status_t
register_route_info(struct net_domain *_domain,
	struct net_route_info *info)
{
	struct net_domain_private *domain = (net_domain_private *)_domain;
	BenaphoreLocker locker(domain->lock);

	domain->route_infos.Add(info);
	info->route = get_route_internal(domain, &info->address);

	return B_OK;
}


status_t
unregister_route_info(struct net_domain *_domain,
	struct net_route_info *info)
{
	struct net_domain_private *domain = (net_domain_private *)_domain;
	BenaphoreLocker locker(domain->lock);

	domain->route_infos.Remove(info);
	if (info->route != NULL)
		put_route_internal(domain, info->route);

	return B_OK;
}


status_t
update_route_info(struct net_domain *domain,
	struct net_route_info *info)
{
	return B_ERROR;
}

