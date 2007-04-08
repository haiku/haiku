/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
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
list_domain_interfaces(void *_buffer, size_t *bufferSize)
{
	BenaphoreLocker locker(sDomainLock);

	UserBuffer buffer(_buffer, *bufferSize);
	net_domain_private *domain = NULL;

	while (true) {
		domain = (net_domain_private *)list_get_next_item(&sDomains, domain);
		if (domain == NULL)
			break;

		BenaphoreLocker locker(domain->lock);

		net_interface *interface = NULL;
		while (true) {
			interface = (net_interface *)list_get_next_item(&domain->interfaces,
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
add_interface_to_domain(net_domain *_domain,
	struct ifreq& request)
{
	net_domain_private *domain = (net_domain_private *)_domain;

	const char *deviceName = request.ifr_parameter.device[0]
		? request.ifr_parameter.device : request.ifr_name;
	const char *baseName = request.ifr_parameter.base_name[0]
		? request.ifr_parameter.base_name : request.ifr_name;

	net_device_interface *deviceInterface = get_device_interface(deviceName);
	if (deviceInterface == NULL)
		return ENODEV;

	BenaphoreLocker locker(domain->lock);

	net_interface_private *interface = NULL;
	status_t status;

	if (find_interface(domain, request.ifr_name) != NULL)
		status = B_NAME_IN_USE;
	else
		status = create_interface(domain, request.ifr_name,
			baseName, deviceInterface, &interface);

	put_device_interface(deviceInterface);

	if (status == B_OK)
		list_add_item(&domain->interfaces, interface);

	return status;
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
domain_interface_control(net_domain_private *domain, int32 option,
	ifreq *request)
{
	const char *name = request->ifr_name;
	status_t status = B_OK;

	net_device_interface *device = get_device_interface(name, false);
	if (device == NULL)
		return ENODEV;
	else {
		// The locking protocol dictates that if both the RX lock
		// and domain locks are required, we MUST obtain the RX
		// lock before the domain lock. This order MUST NOT ever
		// be reversed under the penalty of deadlock.
		RecursiveLocker _1(device->rx_lock);
		BenaphoreLocker _2(domain->lock);

		net_interface *interface = find_interface(domain, name);
		if (interface != NULL) {
			switch (option) {
				case SIOCDIFADDR:
					remove_interface_from_domain(interface);
					break;

				case SIOCSIFFLAGS:
				{
					uint32 requestFlags = request->ifr_flags;
					request->ifr_flags &= ~(IFF_UP | IFF_LINK | IFF_BROADCAST);

					if ((requestFlags & IFF_UP) != (interface->flags & IFF_UP)) {
						if (requestFlags & IFF_UP) {
							status = interface->first_info->interface_up(
								interface->first_protocol);
							if (status == B_OK)
								interface->flags |= IFF_UP;
						} else {
							interface_set_down(interface);
						}
					}

					if (status == B_OK)
						interface->flags |= request->ifr_flags;
					break;
				}
			}
		}
	}

	// If the SIOCDIFADDR call above removed the last interface
	// associated with the device interface, this put_() will
	// effectively remove the interface
	put_device_interface(device);

	return status;
}


void
domain_interface_went_down(net_interface *interface)
{
	// the domain should be locked here. always check
	// all callers to be sure. We get here via
	// interface_set_down().

	dprintf("domain_interface_went_down(%i, %s)\n",
		interface->domain->family, interface->name);

	// domain might have been locked by:
	//  - domain_removed_device_interface() <--- here
	//     remove_interface_from_domain()
	//      delete_interface()
	//       interface_set_down()
	//  - datalink_control() <--- here
	//     interface_set_down()
	invalidate_routes(interface->domain, interface);
}


void
domain_removed_device_interface(net_device_interface *interface)
{
	BenaphoreLocker locker(sDomainLock);

	net_domain_private *domain = NULL;
	while (true) {
		domain = (net_domain_private *)list_get_next_item(&sDomains, domain);
		if (domain == NULL)
			break;

		BenaphoreLocker locker(domain->lock);

		net_interface_private *priv = find_interface(domain, interface->name);
		if (priv == NULL)
			continue;

		remove_interface_from_domain(priv);
	}
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

