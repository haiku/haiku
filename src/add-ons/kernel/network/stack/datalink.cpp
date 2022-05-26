/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Hugo Santos, hugosantos@gmail.com
 */


#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_media.h>
#include <net/route.h>
#include <new>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/sockio.h>

#include <KernelExport.h>

#include <net_datalink.h>
#include <net_device.h>
#include <NetUtilities.h>

#include "device_interfaces.h"
#include "domains.h"
#include "interfaces.h"
#include "routes.h"
#include "stack_private.h"
#include "utility.h"


//#define TRACE_DATALINK
#ifdef TRACE_DATALINK
#	define TRACE(x...) dprintf(STACK_DEBUG_PREFIX x)
#else
#	define TRACE(x...) ;
#endif


struct datalink_protocol : net_protocol {
	struct net_domain_private* domain;
};

struct interface_protocol : net_datalink_protocol {
	struct net_device_module_info* device_module;
	struct net_device* device;
};


#ifdef TRACE_DATALINK


static const char*
option_to_string(int32 option)
{
#	define CODE(x) case x: return #x;
	switch (option) {
		CODE(SIOCADDRT)			/* add route */
		CODE(SIOCDELRT)			/* delete route */
		CODE(SIOCSIFADDR)		/* set interface address */
		CODE(SIOCGIFADDR)		/* get interface address */
		CODE(SIOCSIFDSTADDR)	/* set point-to-point address */
		CODE(SIOCGIFDSTADDR)	/* get point-to-point address */
		CODE(SIOCSIFFLAGS)		/* set interface flags */
		CODE(SIOCGIFFLAGS)		/* get interface flags */
		CODE(SIOCGIFBRDADDR)	/* get broadcast address */
		CODE(SIOCSIFBRDADDR)	/* set broadcast address */
		CODE(SIOCGIFCOUNT)		/* count interfaces */
		CODE(SIOCGIFCONF)		/* get interface list */
		CODE(SIOCGIFINDEX)		/* interface name -> index */
		CODE(SIOCGIFNAME)		/* interface index -> name */
		CODE(SIOCGIFNETMASK)	/* get net address mask */
		CODE(SIOCSIFNETMASK)	/* set net address mask */
		CODE(SIOCGIFMETRIC)		/* get interface metric */
		CODE(SIOCSIFMETRIC)		/* set interface metric */
		CODE(SIOCDIFADDR)		/* delete interface address */
		CODE(SIOCAIFADDR)		/* configure interface alias */
		CODE(SIOCADDMULTI)		/* add multicast address */
		CODE(SIOCDELMULTI)		/* delete multicast address */
		CODE(SIOCGIFMTU)		/* get interface MTU */
		CODE(SIOCSIFMTU)		/* set interface MTU */
		CODE(SIOCSIFMEDIA)		/* set net media */
		CODE(SIOCGIFMEDIA)		/* get net media */

		CODE(SIOCGRTSIZE)		/* get route table size */
		CODE(SIOCGRTTABLE)		/* get route table */
		CODE(SIOCGETRT)			/* get route information for destination */

		CODE(SIOCGIFSTATS)		/* get interface stats */
		CODE(SIOCGIFTYPE)		/* get interface type */

		CODE(SIOCSPACKETCAP)	/* Start capturing packets on an interface */
		CODE(SIOCCPACKETCAP)	/* Stop capturing packets on an interface */

		CODE(SIOCSHIWAT)		/* set high watermark */
		CODE(SIOCGHIWAT)		/* get high watermark */
		CODE(SIOCSLOWAT)		/* set low watermark */
		CODE(SIOCGLOWAT)		/* get low watermark */
		CODE(SIOCATMARK)		/* at out-of-band mark? */
		CODE(SIOCSPGRP)			/* set process group */
		CODE(SIOCGPGRP)			/* get process group */

		CODE(SIOCGPRIVATE_0)	/* device private 0 */
		CODE(SIOCGPRIVATE_1)	/* device private 1 */
		CODE(SIOCSDRVSPEC)		/* set driver-specific parameters */
		CODE(SIOCGDRVSPEC)		/* get driver-specific parameters */

		CODE(SIOCSIFGENERIC)	/* generic IF set op */
		CODE(SIOCGIFGENERIC)	/* generic IF get op */

		CODE(B_SOCKET_SET_ALIAS)		/* set interface alias, ifaliasreq */
		CODE(B_SOCKET_GET_ALIAS)		/* get interface alias, ifaliasreq */
		CODE(B_SOCKET_COUNT_ALIASES)	/* count interface aliases */

		default:
			static char buffer[24];
			snprintf(buffer, sizeof(buffer), "%" B_PRId32, option);
			return buffer;
	}
#	undef CODE
}


#endif	// TRACE_DATALINK


static status_t
get_interface_name_or_index(net_domain* domain, int32 option, void* value,
	size_t* _length)
{
	ASSERT(option == SIOCGIFINDEX || option == SIOCGIFNAME);

	size_t expected = option == SIOCGIFINDEX ? IF_NAMESIZE : sizeof(ifreq);
	if (*_length > 0 && *_length < expected)
		return B_BAD_VALUE;

	ifreq request;
	memset(&request, 0, sizeof(request));

	if (user_memcpy(&request, value, expected) < B_OK)
		return B_BAD_ADDRESS;

	Interface* interface = NULL;
	if (option == SIOCGIFINDEX)
		interface = get_interface(domain, request.ifr_name);
	else
		interface = get_interface(domain, request.ifr_index);

	if (interface == NULL)
		return B_BAD_VALUE;

	if (option == SIOCGIFINDEX)
		request.ifr_index = interface->index;
	else
		strlcpy(request.ifr_name, interface->name, IF_NAMESIZE);

	*_length = sizeof(ifreq);
	interface->ReleaseReference();

	return user_memcpy(value, &request, sizeof(ifreq));
}


static void
set_interface_address(net_interface_address*& target, InterfaceAddress* address)
{
	if (target != NULL)
		static_cast<InterfaceAddress*>(target)->ReleaseReference();

	target = address;
}


static status_t
fill_address(const sockaddr* from, sockaddr* to, size_t maxLength)
{
	if (from != NULL) {
		// Copy address over
		return user_memcpy(to, from, min_c(from->sa_len, maxLength));
	}

	// Fill in empty address
	sockaddr empty;
	empty.sa_len = 2;
	empty.sa_family = AF_UNSPEC;

	return user_memcpy(to, &empty, min_c(2, maxLength));
}


//	#pragma mark - datalink module


static status_t
datalink_control(net_domain* _domain, int32 option, void* value,
	size_t* _length)
{
	TRACE("%s(domain %p, option %s, value %p, length %zu)\n", __FUNCTION__,
		_domain, option_to_string(option), value, *_length);

	net_domain_private* domain = (net_domain_private*)_domain;
	if (domain == NULL || domain->family == AF_LINK) {
		// the AF_LINK family is already handled completely in the link protocol
		return B_BAD_VALUE;
	}

	switch (option) {
		case SIOCGIFINDEX:
		case SIOCGIFNAME:
			return get_interface_name_or_index(domain, option, value, _length);

		case SIOCAIFADDR:	/* same as B_SOCKET_ADD_ALIAS */
		{
			// add new interface address
			if (*_length > 0 && *_length < sizeof(struct ifaliasreq))
				return B_BAD_VALUE;

			struct ifaliasreq request;
			if (user_memcpy(&request, value, sizeof(struct ifaliasreq)) != B_OK)
				return B_BAD_ADDRESS;

			Interface* interface = get_interface(domain, request.ifra_name);
			if (interface != NULL) {
				// A new alias is added to this interface
				status_t status = add_interface_address(interface, domain,
					request);
				notify_interface_changed(interface);
				interface->ReleaseReference();

				return status;
			}

			// A new interface needs to be added
			net_device_interface* deviceInterface
				= get_device_interface(request.ifra_name);
			if (deviceInterface == NULL)
				return B_DEVICE_NOT_FOUND;

			status_t status = add_interface(request.ifra_name, domain, request,
				deviceInterface);

			put_device_interface(deviceInterface);
			return status;
		}

		case SIOCDIFADDR:	/* same as B_SOCKET_REMOVE_ALIAS */
		{
			// remove interface (address)
			struct ifreq request;
			if (user_memcpy(&request, value, sizeof(struct ifreq)) != B_OK)
				return B_BAD_ADDRESS;

			Interface* interface = get_interface(domain, request.ifr_name);
			if (interface == NULL)
				return B_BAD_VALUE;

			status_t status = B_OK;

			if (request.ifr_addr.sa_family != AF_UNSPEC
				&& request.ifr_addr.sa_len != 0) {
				status = interface->Control(domain, SIOCDIFADDR, request,
					(ifreq*)value, *_length);
			} else
				remove_interface(interface);

			interface->ReleaseReference();

			return status;
		}

		case SIOCGIFCOUNT:
		{
			// count number of interfaces
			struct ifconf config;
			config.ifc_value = count_interfaces();

			return user_memcpy(value, &config, sizeof(struct ifconf));
		}

		case SIOCGIFCONF:
		{
			// retrieve ifreqs for all interfaces
			struct ifconf config;
			if (user_memcpy(&config, value, sizeof(struct ifconf)) < B_OK)
				return B_BAD_ADDRESS;

			size_t size = config.ifc_len;
			status_t status
				= list_interfaces(domain->family, config.ifc_buf, &size);
			if (status != B_OK)
				return status;

			config.ifc_len = (int)size;
			return user_memcpy(value, &config, sizeof(struct ifconf));
		}

		case SIOCGRTSIZE:
		{
			// determine size of buffer to hold the routing table
			struct ifconf config;
			config.ifc_value = route_table_size(domain);

			return user_memcpy(value, &config, sizeof(struct ifconf));
		}
		case SIOCGRTTABLE:
		{
			// retrieve all routes for this domain
			struct ifconf config;
			if (user_memcpy(&config, value, sizeof(struct ifconf)) < B_OK)
				return B_BAD_ADDRESS;

			return list_routes(domain, config.ifc_buf, config.ifc_len);
		}
		case SIOCGETRT:
			return get_route_information(domain, value, *_length);

		default:
		{
			// We also accept partial ifreqs as long as the name is complete.
			size_t length = sizeof(struct ifreq);
			if (*_length > 0 && *_length >= IF_NAMESIZE)
				length = min_c(length, *_length);

			// try to pass the request to an existing interface
			struct ifreq request;
			if (user_memcpy(&request, value, length) != B_OK)
				return B_BAD_ADDRESS;

			Interface* interface = get_interface(domain, request.ifr_name);
			if (interface == NULL)
				return B_BAD_VALUE;

			status_t status = interface->Control(domain, option, request,
				(ifreq*)value, *_length);

			interface->ReleaseReference();
			return status;
		}
	}
	return B_BAD_VALUE;
}


static status_t
datalink_send_routed_data(struct net_route* route, net_buffer* buffer)
{
	TRACE("%s(route %p, buffer %p)\n", __FUNCTION__, route, buffer);

	InterfaceAddress* address = (InterfaceAddress*)route->interface_address;
	Interface* interface = (Interface*)address->interface;

	//dprintf("send buffer (%ld bytes) to interface %s (route flags %lx)\n",
	//	buffer->size, interface->name, route->flags);

	if ((route->flags & RTF_REJECT) != 0) {
		TRACE("  rejected route\n");
		return ENETUNREACH;
	}

	if ((route->flags & RTF_LOCAL) != 0) {
		TRACE("  local route\n");

		// We set the interface address here, so the buffer is delivered
		// directly to the domain in interfaces.cpp:device_consumer_thread()
		address->AcquireReference();
		set_interface_address(buffer->interface_address, address);

		// this one goes back to the domain directly
		return fifo_enqueue_buffer(
			&interface->DeviceInterface()->receive_queue, buffer);
	}

	if ((route->flags & RTF_GATEWAY) != 0) {
		TRACE("  gateway route\n");

		// This route involves a gateway, we need to use the gateway address
		// instead of the destination address:
		if (route->gateway == NULL)
			return B_MISMATCHED_VALUES;
		memcpy(buffer->destination, route->gateway, route->gateway->sa_len);
	}

	// this goes out to the datalink protocols
	domain_datalink* datalink
		= interface->DomainDatalink(address->domain->family);
	return datalink->first_info->send_data(datalink->first_protocol, buffer);
}


/*!	Finds a route for the given \a buffer in the given \a domain, and calls
	net_protocol_info::send_routed_data() on either the \a protocol (if
	non-NULL), or the domain.
*/
static status_t
datalink_send_data(net_protocol* protocol, net_domain* domain,
	net_buffer* buffer)
{
	TRACE("%s(%p, domain %p, buffer %p)\n", __FUNCTION__, protocol, domain,
		buffer);

	if (protocol == NULL && domain == NULL)
		return B_BAD_VALUE;

	net_protocol_module_info* module = protocol != NULL
		? protocol->module : domain->module;

	if (domain == NULL)
		domain = protocol->module->get_domain(protocol);

	net_route* route = NULL;
	status_t status;
	if (protocol != NULL && protocol->socket != NULL
		&& protocol->socket->bound_to_device != 0) {
		status = get_device_route(domain, protocol->socket->bound_to_device,
			&route);
	} else
		status = get_buffer_route(domain, buffer, &route);

	TRACE("  route status: %s\n", strerror(status));

	if (status != B_OK)
		return status;

	status = module->send_routed_data(protocol, route, buffer);
	put_route(domain, route);
	return status;
}


/*!	Tests if \a address is a local address in the domain.

	\param _interfaceAddress will be set to the interface address belonging to
		that address if non-NULL. If the address \a _interfaceAddress points to
		is not NULL, it is assumed that it already points to an address, which
		is then released before the new address is assigned.
	\param _matchedType will be set to either zero or MSG_BCAST if non-NULL.
*/
static bool
datalink_is_local_address(net_domain* domain, const struct sockaddr* address,
	net_interface_address** _interfaceAddress, uint32* _matchedType)
{
	TRACE("%s(domain %p, address %s)\n", __FUNCTION__, domain,
		AddressString(domain, address).Data());

	if (domain == NULL || address == NULL
		|| domain->family != address->sa_family)
		return false;

	uint32 matchedType = 0;

	InterfaceAddress* interfaceAddress = get_interface_address(address);
	if (interfaceAddress == NULL) {
		// Check for matching broadcast address
		if ((domain->address_module->flags
				& NET_ADDRESS_MODULE_FLAG_BROADCAST_ADDRESS) != 0) {
			interfaceAddress
				= get_interface_address_for_destination(domain, address);
			matchedType = MSG_BCAST;
		}
		if (interfaceAddress == NULL) {
			TRACE("  no\n");
			return false;
		}
	}

	TRACE("  it is, interface address %p\n", interfaceAddress);

	if (_interfaceAddress != NULL)
		set_interface_address(*_interfaceAddress, interfaceAddress);
	else
		interfaceAddress->ReleaseReference();

	if (_matchedType != NULL)
		*_matchedType = matchedType;

	return true;
}


/*!	Tests if \a address is a local link address in the domain.

	\param unconfiguredOnly only unconfigured interfaces are taken into account.
	\param _interfaceAddress will be set to the first address of the interface
		and domain belonging to that address if non-NULL. If the address
		\a _interfaceAddress points to is not NULL, it is assumed that it
		already points to an address, which is then released before the new
		address is assigned.
*/
static bool
datalink_is_local_link_address(net_domain* domain, bool unconfiguredOnly,
	const struct sockaddr* address, net_interface_address** _interfaceAddress)
{
	if (domain == NULL || address == NULL || address->sa_family != AF_LINK)
		return false;

#ifdef TRACE_DATALINK
	uint8* data = LLADDR((sockaddr_dl*)address);
	TRACE("%s(domain %p, unconfiguredOnly %d, address %02x:%02x:%02x:%02x:%02x"
		":%02x)\n", __FUNCTION__, domain, unconfiguredOnly, data[0], data[1],
		data[2], data[3], data[4], data[5]);
#endif

	InterfaceAddress* interfaceAddress = get_interface_address_for_link(domain,
		address, unconfiguredOnly);
	if (interfaceAddress == NULL) {
		TRACE("  no\n");
		return false;
	}

	if (_interfaceAddress != NULL)
		set_interface_address(*_interfaceAddress, interfaceAddress);
	else
		interfaceAddress->ReleaseReference();

	return true;
}


static net_interface*
datalink_get_interface(net_domain* domain, uint32 index)
{
	return get_interface(domain, index);
}


static net_interface*
datalink_get_interface_with_address(const sockaddr* address)
{
	InterfaceAddress* interfaceAddress = get_interface_address(address);
	if (interfaceAddress == NULL)
		return NULL;

	Interface* interface = static_cast<Interface*>(interfaceAddress->interface);

	interface->AcquireReference();
	interfaceAddress->ReleaseReference();

	return interface;
}


static void
datalink_put_interface(net_interface* interface)
{
	if (interface == NULL)
		return;

	((Interface*)interface)->ReleaseReference();
}


static net_interface_address*
datalink_get_interface_address(const struct sockaddr* address)
{
	return get_interface_address(address);
}


/*!	Returns a reference to the next address of the given interface in
	\a _address. When you call this function the first time, \a _address must
	point to a NULL address. Upon the next call, the reference to the previous
	address is taken over again.

	If you do not traverse the list to the end, you'll have to manually release
	the reference to the address where you stopped.

	\param interface The interface whose address list should be iterated over.
	\param _address A pointer to the location where the next address should
		be stored.

	\return \c true if an address reference was returned, \c false if not.
*/
static bool
datalink_get_next_interface_address(net_interface* _interface,
	net_interface_address** _address)
{
	Interface* interface = (Interface*)_interface;

	InterfaceAddress* address = (InterfaceAddress*)*_address;
	bool gotOne = interface->GetNextAddress(&address);
	*_address = address;

	return gotOne;
}


static void
datalink_put_interface_address(net_interface_address* address)
{
	if (address == NULL)
		return;

	((InterfaceAddress*)address)->ReleaseReference();
}


static status_t
datalink_join_multicast(net_interface* _interface, net_domain* domain,
	const struct sockaddr* address)
{
	Interface* interface = (Interface*)_interface;
	domain_datalink* datalink = interface->DomainDatalink(domain->family);

	return datalink->first_info->join_multicast(datalink->first_protocol,
		address);
}


static status_t
datalink_leave_multicast(net_interface* _interface, net_domain* domain,
	const struct sockaddr* address)
{
	Interface* interface = (Interface*)_interface;
	domain_datalink* datalink = interface->DomainDatalink(domain->family);

	return datalink->first_info->leave_multicast(datalink->first_protocol,
		address);
}


static status_t
datalink_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


//	#pragma mark - net_datalink_protocol


static status_t
interface_protocol_init(net_interface* interface, net_domain* domain,
	net_datalink_protocol** _protocol)
{
	interface_protocol* protocol = new(std::nothrow) interface_protocol;
	if (protocol == NULL)
		return B_NO_MEMORY;

	TRACE("%s(%p, interface %p - %s, domain %p)\n", __FUNCTION__, protocol,
		interface, interface->name, domain);

	protocol->device_module = interface->device->module;
	protocol->device = interface->device;

	*_protocol = protocol;
	return B_OK;
}


static status_t
interface_protocol_uninit(net_datalink_protocol* protocol)
{
	TRACE("%s(%p)\n", __FUNCTION__, protocol);

	delete protocol;
	return B_OK;
}


static status_t
interface_protocol_send_data(net_datalink_protocol* _protocol,
	net_buffer* buffer)
{
	TRACE("%s(%p, buffer %p)\n", __FUNCTION__, _protocol, buffer);

	interface_protocol* protocol = (interface_protocol*)_protocol;
	Interface* interface = (Interface*)protocol->interface;

	if (atomic_get(&interface->DeviceInterface()->monitor_count) > 0)
		device_interface_monitor_receive(interface->DeviceInterface(), buffer);

	return protocol->device_module->send_data(protocol->device, buffer);
}


static status_t
interface_protocol_up(net_datalink_protocol* protocol)
{
	TRACE("%s(%p)\n", __FUNCTION__, protocol);
	return B_OK;
}


static void
interface_protocol_down(net_datalink_protocol* _protocol)
{
	TRACE("%s(%p)\n", __FUNCTION__, _protocol);

	interface_protocol* protocol = (interface_protocol*)_protocol;
	Interface* interface = (Interface*)protocol->interface;
	net_device_interface* deviceInterface = interface->DeviceInterface();

	if (deviceInterface->up_count == 0)
		return;

	deviceInterface->up_count--;

	interface->WentDown();

	if (deviceInterface->up_count > 0)
		return;

	down_device_interface(deviceInterface);
}


static status_t
interface_protocol_change_address(net_datalink_protocol* protocol,
	net_interface_address* interfaceAddress, int32 option,
	const struct sockaddr* oldAddress, const struct sockaddr* newAddress)
{
	TRACE("%s(%p, interface address %p, option %s, old %p, new %p)\n",
		__FUNCTION__, protocol, interfaceAddress, option_to_string(option),
		oldAddress, newAddress);

	switch (option) {
		case SIOCSIFADDR:
		case SIOCSIFNETMASK:
		case SIOCSIFBRDADDR:
		case SIOCSIFDSTADDR:
		case SIOCDIFADDR:
			return update_interface_address((InterfaceAddress*)interfaceAddress,
				option, oldAddress, newAddress);
	}

	return B_OK;
}


static status_t
interface_protocol_control(net_datalink_protocol* _protocol, int32 option,
	void* argument, size_t length)
{
	TRACE("%s(%p, option %s, argument %p, length %zu)\n", __FUNCTION__,
		_protocol, option_to_string(option), argument, length);

	interface_protocol* protocol = (interface_protocol*)_protocol;
	Interface* interface = (Interface*)protocol->interface;

	switch (option) {
		case SIOCGIFADDR:
		case SIOCGIFNETMASK:
		case SIOCGIFBRDADDR:
		case SIOCGIFDSTADDR:
		{
			if (length == 0)
				length = sizeof(ifreq);
			else if (length < sizeof(ifreq))
				return B_BAD_VALUE;

			ifreq request;
			if (user_memcpy(&request, argument, sizeof(struct ifreq)) != B_OK)
				return B_BAD_ADDRESS;

			InterfaceAddress* interfaceAddress
				= get_interface_address(&request.ifr_addr);
			if (interfaceAddress == NULL) {
				interfaceAddress
					= interface->FirstForFamily(protocol->domain->family);
				if (interfaceAddress == NULL)
					return B_BAD_VALUE;
			}

			size_t maxLength = length - offsetof(ifreq, ifr_addr);

			status_t status = fill_address(
				*interfaceAddress->AddressFor(option),
				&((struct ifreq*)argument)->ifr_addr, maxLength);

			interfaceAddress->ReleaseReference();
			return status;
		}

		case B_SOCKET_COUNT_ALIASES:
		{
			ifreq request;
			request.ifr_count = interface->CountAddresses();

			return user_memcpy(&((struct ifreq*)argument)->ifr_count,
				&request.ifr_count, sizeof(request.ifr_count));
		}

		case B_SOCKET_GET_ALIAS:
		{
			ifaliasreq request;
			if (user_memcpy(&request, argument, sizeof(ifaliasreq)) != B_OK)
				return B_BAD_ADDRESS;

			InterfaceAddress* address = NULL;
			if (request.ifra_index < 0) {
				if (!protocol->domain->address_module->is_empty_address(
						(const sockaddr*)&request.ifra_addr, false)) {
					// Find first address that matches the local address
					address = interface->AddressForLocal(protocol->domain,
						(const sockaddr*)&request.ifra_addr);
				} else {
					// Find first address for family
					address = interface->FirstForFamily(
						protocol->domain->family);
				}

				request.ifra_index = interface->IndexOfAddress(address);
			} else
				address = interface->AddressAt(request.ifra_index);
			if (address == NULL)
				return B_BAD_VALUE;

			// Copy index (in case none was specified)
			status_t status = user_memcpy(
				&((struct ifaliasreq*)argument)->ifra_index,
				&request.ifra_index, sizeof(request.ifra_index));

			// Copy address info
			if (status == B_OK) {
				status = fill_address(address->local,
					(sockaddr*)&((struct ifaliasreq*)argument)->ifra_addr,
					sizeof(sockaddr_storage));
			}
			if (status == B_OK) {
				status = fill_address(address->mask,
					(sockaddr*)&((struct ifaliasreq*)argument)->ifra_mask,
					sizeof(sockaddr_storage));
			}
			if (status == B_OK) {
				status = fill_address(address->destination,
					(sockaddr*)&((struct ifaliasreq*)argument)
						->ifra_destination,
					sizeof(sockaddr_storage));
			}

			address->ReleaseReference();

			return status;
		}

		case SIOCGIFFLAGS:
		{
			// get flags
			struct ifreq request;
			request.ifr_flags = interface->flags | interface->device->flags;

			return user_memcpy(&((struct ifreq*)argument)->ifr_flags,
				&request.ifr_flags, sizeof(request.ifr_flags));
		}

		case SIOCGIFSTATS:
		{
			// get stats
			return user_memcpy(&((struct ifreq*)argument)->ifr_stats,
				&interface->DeviceInterface()->device->stats,
				sizeof(struct ifreq_stats));
		}

		case SIOCGIFTYPE:
		{
			// get type
			struct ifreq request;
			request.ifr_type = interface->type;

			return user_memcpy(&((struct ifreq*)argument)->ifr_type,
				&request.ifr_type, sizeof(request.ifr_type));
		}

		case SIOCGIFMTU:
		{
			// get MTU
			struct ifreq request;
			request.ifr_mtu = interface->mtu;

			return user_memcpy(&((struct ifreq*)argument)->ifr_mtu,
				&request.ifr_mtu, sizeof(request.ifr_mtu));
		}
		case SIOCSIFMTU:
		{
			// set MTU
			struct ifreq request;
			if (user_memcpy(&request, argument, sizeof(struct ifreq)) < B_OK)
				return B_BAD_ADDRESS;

			// check for valid bounds
			if (request.ifr_mtu < 100
				|| (uint32)request.ifr_mtu > interface->device->mtu)
				return B_BAD_VALUE;

			interface->mtu = request.ifr_mtu;
			notify_interface_changed(interface);
			return B_OK;
		}

		case SIOCSIFMEDIA:
		{
			// set media
			struct ifreq request;
			if (user_memcpy(&request, argument, sizeof(struct ifreq)) != B_OK)
				return B_BAD_ADDRESS;

			status_t status
				= interface->device->module->set_media(
					interface->device, request.ifr_media);
			if (status == B_NOT_SUPPORTED) {
				// TODO: this isn't so nice, and should be solved differently
				// (for example by removing the set_media() call altogether, or
				// making it able to deal properly with FreeBSD drivers as well)
				// try driver directly
				status = interface->device->module->control(
					interface->device, SIOCSIFMEDIA, &request, sizeof(request));
			}

			return status;
		}
		case SIOCGIFMEDIA:
		{
			// get media
			if (length > 0 && length < sizeof(ifmediareq))
				return B_BAD_VALUE;

			struct ifmediareq request;
			if (user_memcpy(&request, argument, sizeof(ifmediareq)) != B_OK)
				return B_BAD_ADDRESS;

			// TODO: see above.
			if (interface->device->module->control(interface->device,
					SIOCGIFMEDIA, &request,
					sizeof(struct ifmediareq)) != B_OK) {
				memset(&request, 0, sizeof(struct ifmediareq));
				request.ifm_active = request.ifm_current
					= interface->device->media;
			}

			return user_memcpy(argument, &request, sizeof(struct ifmediareq));
		}

		case SIOCGIFMETRIC:
		{
			// get metric
			struct ifreq request;
			request.ifr_metric = interface->metric;

			return user_memcpy(&((struct ifreq*)argument)->ifr_metric,
				&request.ifr_metric, sizeof(request.ifr_metric));
		}
		case SIOCSIFMETRIC:
		{
			// set metric
			struct ifreq request;
			if (user_memcpy(&request, argument, sizeof(struct ifreq)) < B_OK)
				return B_BAD_ADDRESS;

			interface->metric = request.ifr_metric;
			notify_interface_changed(interface);
			return B_OK;
		}

		case SIOCADDRT:
		case SIOCDELRT:
			// interface related route options
			return control_routes(interface, protocol->domain, option, argument,
				length);
	}

	return protocol->device_module->control(protocol->device,
		option, argument, length);
}


static status_t
interface_protocol_join_multicast(net_datalink_protocol* _protocol,
	const sockaddr* address)
{
	interface_protocol* protocol = (interface_protocol*)_protocol;

	return protocol->device_module->add_multicast(protocol->device, address);
}


static status_t
interface_protocol_leave_multicast(net_datalink_protocol* _protocol,
	const sockaddr* address)
{
	interface_protocol* protocol = (interface_protocol*)_protocol;

	return protocol->device_module->remove_multicast(protocol->device,
		address);
}


net_datalink_module_info gNetDatalinkModule = {
	{
		NET_DATALINK_MODULE_NAME,
		0,
		datalink_std_ops
	},

	datalink_control,
	datalink_send_routed_data,
	datalink_send_data,

	datalink_is_local_address,
	datalink_is_local_link_address,

	datalink_get_interface,
	datalink_get_interface_with_address,
	datalink_put_interface,

	datalink_get_interface_address,
	datalink_get_next_interface_address,
	datalink_put_interface_address,

	datalink_join_multicast,
	datalink_leave_multicast,

	add_route,
	remove_route,
	get_route,
	get_buffer_route,
	put_route,
	register_route_info,
	unregister_route_info,
	update_route_info
};

net_datalink_protocol_module_info gDatalinkInterfaceProtocolModule = {
	{
		NULL,
		0,
		NULL
	},
	interface_protocol_init,
	interface_protocol_uninit,
	interface_protocol_send_data,
	interface_protocol_up,
	interface_protocol_down,
	interface_protocol_change_address,
	interface_protocol_control,
	interface_protocol_join_multicast,
	interface_protocol_leave_multicast,
};
