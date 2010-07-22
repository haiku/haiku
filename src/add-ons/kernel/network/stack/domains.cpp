/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "domains.h"
#include "interfaces.h"
#include "utility.h"
#include "stack_private.h"

#include <net_device.h>

#include <lock.h>
#include <util/AutoLock.h>

#include <KernelExport.h>

#include <net/if_media.h>
#include <new>
#include <string.h>
#include <sys/sockio.h>


#define TRACE_DOMAINS
#ifdef TRACE_DOMAINS
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

#define ENABLE_DEBUGGER_COMMANDS	1


typedef DoublyLinkedList<net_domain_private> DomainList;

static mutex sDomainLock;
static DomainList sDomains;


/*!	Scans the domain list for the specified family.
	You need to hold the sDomainLock when calling this function.
*/
static net_domain_private*
lookup_domain(int family)
{
	DomainList::Iterator iterator = sDomains.GetIterator();
	while (net_domain_private* domain = iterator.Next()) {
		if (domain->family == family)
			return domain;
	}

	return NULL;
}


#if	ENABLE_DEBUGGER_COMMANDS


static int
dump_domains(int argc, char** argv)
{
	DomainList::Iterator iterator = sDomains.GetIterator();
	while (net_domain_private* domain = iterator.Next()) {
		kprintf("domain: %p, %s, %d\n", domain, domain->name, domain->family);
		kprintf("  module:         %p\n", domain->module);
		kprintf("  address_module: %p\n", domain->address_module);
		
		if (!list_is_empty(&domain->interfaces))
			kprintf("  interfaces:\n");

		net_interface* interface = NULL;
		while (true) {
			interface = (net_interface*)list_get_next_item(&domain->interfaces,
				interface);
			if (interface == NULL)
				break;

			kprintf("    %p\n", interface);
		}

		if (!domain->routes.IsEmpty())
			kprintf("  routes:\n");
	
		RouteList::Iterator routeIterator = domain->routes.GetIterator();
		while (net_route* route = routeIterator.Next()) {
			kprintf("    %p\n", route);
		}

		if (!domain->route_infos.IsEmpty())
			kprintf("  route infos:\n");
	
		RouteInfoList::Iterator infoIterator = domain->route_infos.GetIterator();
		while (net_route_info* info = infoIterator.Next()) {
			kprintf("    %p\n", info);
		}
	}

	return 0;
}


#endif	// ENABLE_DEBUGGER_COMMANDS


//	#pragma mark -


/*!	Gets the domain of the specified family.
*/
net_domain*
get_domain(int family)
{
	MutexLocker locker(sDomainLock);
	return lookup_domain(family);
}


uint32
count_domain_interfaces()
{
	MutexLocker locker(sDomainLock);

	uint32 count = 0;

	DomainList::Iterator iterator = sDomains.GetIterator();
	while (net_domain_private* domain = iterator.Next()) {
		net_interface* interface = NULL;
		while (true) {
			interface = (net_interface*)list_get_next_item(&domain->interfaces,
				interface);
			if (interface == NULL)
				break;

			count++;
		}
	}

	return count;
}


/*!	Dumps a list of all interfaces into the supplied userland buffer.
	If the interfaces don't fit into the buffer, an error (\c ENOBUFS) is
	returned.
*/
status_t
list_domain_interfaces(void* _buffer, size_t* bufferSize)
{
	MutexLocker locker(sDomainLock);

	UserBuffer buffer(_buffer, *bufferSize);

	DomainList::Iterator iterator = sDomains.GetIterator();
	while (net_domain_private* domain = iterator.Next()) {
		RecursiveLocker locker(domain->lock);

		net_interface* interface = NULL;
		while (true) {
			interface = (net_interface*)list_get_next_item(&domain->interfaces,
				interface);
			if (interface == NULL)
				break;

			ifreq request;
			strlcpy(request.ifr_name, interface->name, IF_NAMESIZE);
			if (interface->address != NULL) {
				memcpy(&request.ifr_addr, interface->address,
					interface->address->sa_len);
			} else {
				// empty address
				request.ifr_addr.sa_len = 2;
				request.ifr_addr.sa_family = AF_UNSPEC;
			}

			if (buffer.Copy(&request, IF_NAMESIZE
					+ request.ifr_addr.sa_len) == NULL)
				return buffer.Status();
		}
	}

	*bufferSize = buffer.ConsumedAmount();
	return B_OK;
}


status_t
add_interface_to_domain(net_domain* _domain,
	struct ifreq& request)
{
	net_domain_private* domain = (net_domain_private*)_domain;

	const char* deviceName = request.ifr_parameter.device[0]
		? request.ifr_parameter.device : request.ifr_name;
	const char* baseName = request.ifr_parameter.base_name[0]
		? request.ifr_parameter.base_name : request.ifr_name;

	net_device_interface* deviceInterface = get_device_interface(deviceName);
	if (deviceInterface == NULL)
		return ENODEV;

	RecursiveLocker locker(domain->lock);

	net_interface_private* interface = NULL;
	status_t status;

	if (find_interface(domain, request.ifr_name) == NULL) {
		// We must not hold the domain's link when creating the interface:
		// this will call get_module() which might want to access a network
		// device when booting from network.
		locker.Unlock();
		status = create_interface(domain, request.ifr_name,
			baseName, deviceInterface, &interface);
		locker.Lock();

		if (find_interface(domain, request.ifr_name) != NULL) {
			delete_interface(interface);
			status = B_NAME_IN_USE;
		}
	} else
		status = B_NAME_IN_USE;

	put_device_interface(deviceInterface);

	if (status == B_OK) {
		list_add_item(&domain->interfaces, interface);
		notify_interface_added(interface);
	}

	return status;
}


/*!	Removes the interface from its domain, and deletes it.
	You need to hold the domain's lock when calling this function.
*/
status_t
remove_interface_from_domain(net_interface* interface)
{
	net_domain_private* domain = (net_domain_private*)interface->domain;

	invalidate_routes(domain, interface);

	list_remove_item(&domain->interfaces, interface);
	notify_interface_removed(interface);
	delete_interface((net_interface_private*)interface);
	return B_OK;
}


status_t
domain_interface_control(net_domain_private* domain, int32 option,
	ifreq* request)
{
	const char* name = request->ifr_name;
	status_t status = B_OK;

	net_device_interface* device = get_device_interface(name, false);
	if (device == NULL)
		return ENODEV;

	RecursiveLocker _(domain->lock);

	net_interface* interface = find_interface(domain, name);
	if (interface != NULL) {
		switch (option) {
			case SIOCDIFADDR:
				remove_interface_from_domain(interface);
				break;

			case SIOCSIFFLAGS:
			{
				uint32 oldFlags = interface->flags;
				uint32 requestFlags = request->ifr_flags;
				request->ifr_flags &= ~(IFF_UP | IFF_LINK | IFF_BROADCAST);

				if ((requestFlags & IFF_UP) != (interface->flags & IFF_UP)) {
					if ((requestFlags & IFF_UP) != 0) {
						status = interface->first_info->interface_up(
							interface->first_protocol);
						if (status == B_OK)
							interface->flags |= IFF_UP;
					} else {
						interface_set_down(interface);
					}
				}

				if (status == B_OK) {
					// TODO: why shouldn't we able to delete IFF_BROADCAST?
					interface->flags &= IFF_UP | IFF_LINK | IFF_BROADCAST;
					interface->flags |= request->ifr_flags;
				}
				
				if (oldFlags != interface->flags) {
					notify_interface_changed(interface, oldFlags,
						interface->flags);
				}
				break;
			}
		}
	}

	// If the SIOCDIFADDR call above removed the last interface associated with
	// the device interface, this will effectively remove the interface
	put_device_interface(device);

	return status;
}


/*!	You need to hold the domain lock when calling this function. */
void
domain_interface_went_down(net_interface* interface)
{
	ASSERT_LOCKED_RECURSIVE(&((net_domain_private*)interface->domain)->lock);

	TRACE(("domain_interface_went_down(%i, %s)\n",
		interface->domain->family, interface->name));

	invalidate_routes(interface->domain, interface);
}


void
domain_removed_device_interface(net_device_interface* deviceInterface)
{
	MutexLocker locker(sDomainLock);

	DomainList::Iterator iterator = sDomains.GetIterator();
	while (net_domain_private* domain = iterator.Next()) {
		RecursiveLocker locker(domain->lock);

		net_interface_private* interface = find_interface(domain,
			deviceInterface->device->name);
		if (interface == NULL)
			continue;

		remove_interface_from_domain(interface);
	}
}


status_t
register_domain(int family, const char* name,
	struct net_protocol_module_info* module,
	struct net_address_module_info* addressModule,
	net_domain** _domain)
{
	TRACE(("register_domain(%d, %s)\n", family, name));
	MutexLocker locker(sDomainLock);

	struct net_domain_private* domain = lookup_domain(family);
	if (domain != NULL)
		return B_NAME_IN_USE;

	domain = new(std::nothrow) net_domain_private;
	if (domain == NULL)
		return B_NO_MEMORY;

	recursive_lock_init(&domain->lock, name);

	domain->family = family;
	domain->name = name;
	domain->module = module;
	domain->address_module = addressModule;

	list_init(&domain->interfaces);

	sDomains.Add(domain);

	*_domain = domain;
	return B_OK;
}


status_t
unregister_domain(net_domain* _domain)
{
	TRACE(("unregister_domain(%p, %d, %s)\n", _domain, _domain->family,
		_domain->name));

	net_domain_private* domain = (net_domain_private*)_domain;
	MutexLocker locker(sDomainLock);

	sDomains.Remove(domain);

	net_interface_private* interface = NULL;
	while (true) {
		interface = (net_interface_private*)list_remove_head_item(
			&domain->interfaces);
		if (interface == NULL)
			break;

		delete_interface(interface);
	}

	recursive_lock_destroy(&domain->lock);
	delete domain;
	return B_OK;
}


status_t
init_domains()
{
	mutex_init(&sDomainLock, "net domains");

	new (&sDomains) DomainList;
		// static C++ objects are not initialized in the module startup

#if ENABLE_DEBUGGER_COMMANDS
	add_debugger_command("net_domains", &dump_domains,
		"Dump network domains");
#endif
	return B_OK;
}


status_t
uninit_domains()
{
#if ENABLE_DEBUGGER_COMMANDS
	remove_debugger_command("net_domains", &dump_domains);
#endif

	mutex_destroy(&sDomainLock);
	return B_OK;
}

