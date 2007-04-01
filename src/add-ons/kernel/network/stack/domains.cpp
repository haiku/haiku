/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "domains.h"
#include "interfaces.h"

#include <KernelExport.h>

#include <lock.h>
#include <util/AutoLock.h>

#include <new>
#include <string.h>


#define TRACE_DOMAINS
#ifdef TRACE_DOMAINS
#	define TRACE(x) dprintf x
#else
#	define TRACE(x) ;
#endif

static benaphore sDomainLock;
static list sDomains;


/*!
	Scans the domain list for the specified family.
	You need to hold the sDomainLock when calling this function.
*/
static net_domain_private *
lookup_domain(int family)
{
	net_domain_private *domain = NULL;
	while (true) {
		domain = (net_domain_private *)list_get_next_item(&sDomains, domain);
		if (domain == NULL)
			break;

		if (domain->family == family)
			return domain;
	}

	return NULL;
}


//	#pragma mark -


/*!
	Gets the domain of the specified family.
*/
net_domain *
get_domain(int family)
{
	BenaphoreLocker locker(sDomainLock);
	return lookup_domain(family);
}


uint32
count_domain_interfaces()
{
	BenaphoreLocker locker(sDomainLock);

	net_domain_private *domain = NULL;
	uint32 count = 0;

	while (true) {
		domain = (net_domain_private *)list_get_next_item(&sDomains, domain);
		if (domain == NULL)
			break;

		net_interface *interface = NULL;
		while (true) {
			interface = (net_interface *)list_get_next_item(&domain->interfaces,
				interface);
			if (interface == NULL)
				break;

			count++;
		}
	}

	return count;
}


/*!
	Dumps a list of all interfaces into the supplied userland buffer.
	If the interfaces don't fit into the buffer, an error (\c ENOBUFS) is
	returned.
*/
status_t
list_domain_interfaces(void *buffer, size_t *_bufferSize)
{
	BenaphoreLocker locker(sDomainLock);

	uint8 *current = (uint8 *)buffer;
	const uint8 *bufferEnd = current + (*_bufferSize);
	net_domain_private *domain = NULL;

	while (true) {
		domain = (net_domain_private *)list_get_next_item(&sDomains, domain);
		if (domain == NULL)
			break;

		net_interface *interface = NULL;
		while (true) {
			interface = (net_interface *)list_get_next_item(&domain->interfaces,
				interface);
			if (interface == NULL)
				break;

			size_t size = IF_NAMESIZE + (interface->address ? interface->address->sa_len : 2);
			if ((current + size) > bufferEnd)
				return ENOBUFS;

			ifreq request;
			strlcpy(request.ifr_name, interface->name, IF_NAMESIZE);
			if (interface->address != NULL)
				memcpy(&request.ifr_addr, interface->address, interface->address->sa_len);
			else {
				// empty address
				request.ifr_addr.sa_len = 2;
				request.ifr_addr.sa_family = AF_UNSPEC;
			}

			if (user_memcpy(current, &request, size) < B_OK)
				return B_BAD_ADDRESS;

			current += size;
		}
	}

	*_bufferSize = current - (uint8 *)buffer;
	return B_OK;
}


status_t
add_interface_to_domain(net_domain *_domain,
	struct ifreq& request)
{
	net_domain_private *domain = (net_domain_private *)_domain;

	const char *deviceName = request.ifr_parameter.device[0]
		? request.ifr_parameter.device : request.ifr_name;
	net_device_interface *deviceInterface = get_device_interface(deviceName);
	if (deviceInterface == NULL)
		return ENODEV;

	BenaphoreLocker locker(domain->lock);

	if (find_interface(domain, request.ifr_name) != NULL)
		return B_NAME_IN_USE;

	net_interface_private *interface;
	status_t status = create_interface(domain,
		request.ifr_name, request.ifr_parameter.base_name[0]
			? request.ifr_parameter.base_name : request.ifr_name,
		deviceInterface, &interface);
	if (status < B_OK) {
		put_device_interface(deviceInterface);
		return status;
	}

	list_add_item(&domain->interfaces, interface);
	return B_OK;
}


/*!
	Removes the interface from its domain, and deletes it.
	You need to hold the domain's lock when calling this function.
*/
status_t
remove_interface_from_domain(net_interface *interface)
{
	net_domain_private *domain = (net_domain_private *)interface->domain;

	list_remove_item(&domain->interfaces, interface);
	delete_interface((net_interface_private *)interface);
	return B_OK;
}


status_t
register_domain(int family, const char *name,
	struct net_protocol_module_info *module, 
	struct net_address_module_info *addressModule, 
	net_domain **_domain)
{
	TRACE(("register_domain(%d, %s)\n", family, name));
	BenaphoreLocker locker(sDomainLock);

	struct net_domain_private *domain = lookup_domain(family);
	if (domain != NULL)
		return B_NAME_IN_USE;

	domain = new (std::nothrow) net_domain_private;
	if (domain == NULL)
		return B_NO_MEMORY;

	status_t status = benaphore_init(&domain->lock, name);
	if (status < B_OK) {
		delete domain;
		return status;
	}

	domain->family = family;
	domain->name = name;
	domain->module = module;
	domain->address_module = addressModule;

	list_init(&domain->interfaces);

	list_add_item(&sDomains, domain);

	*_domain = domain;
	return B_OK;
}


status_t
unregister_domain(net_domain *_domain)
{
	TRACE(("unregister_domain(%p, %d, %s)\n", _domain, _domain->family, _domain->name));

	net_domain_private *domain = (net_domain_private *)_domain;
	BenaphoreLocker locker(sDomainLock);

	list_remove_item(&sDomains, domain);
	
	net_interface_private *interface = NULL;
	while (true) {
		interface = (net_interface_private *)list_remove_head_item(&domain->interfaces);
		if (interface == NULL)
			break;

		delete_interface(interface);
	}

	benaphore_destroy(&domain->lock);
	delete domain;
	return B_OK;
}


status_t
init_domains()
{
	if (benaphore_init(&sDomainLock, "net domains") < B_OK)
		return B_ERROR;

	list_init_etc(&sDomains, offsetof(struct net_domain_private, link));
	return B_OK;
}


status_t
uninit_domains()
{
	benaphore_destroy(&sDomainLock);
	return B_OK;
}

