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
#include <NetUtilities.h>

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
	ASSERT_LOCKED_MUTEX(&sDomainLock);

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

		if (!domain->routes.IsEmpty())
			kprintf("  routes:\n");
	
		RouteList::Iterator routeIterator = domain->routes.GetIterator();
		while (net_route_private* route = routeIterator.Next()) {
			kprintf("    %p: dest %s, mask %s, gw %s, flags %" B_PRIx32 ", "
				"address %p\n", route, AddressString(domain, route->destination
					? route->destination : NULL).Data(),
				AddressString(domain, route->mask ? route->mask : NULL).Data(),
				AddressString(domain, route->gateway
					? route->gateway : NULL).Data(),
				route->flags, route->interface_address);
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

