/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "interfaces.h"

#include <net/if_dl.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sockio.h>

#include <KernelExport.h>

#include <net_device.h>
#include <NetUtilities.h>

#include "device_interfaces.h"
#include "domains.h"
#include "stack_private.h"
#include "utility.h"


//#define TRACE_INTERFACES
#ifdef TRACE_INTERFACES
#	define TRACE(x...) dprintf(STACK_DEBUG_PREFIX x)
#else
#	define TRACE(x...) ;
#endif


struct AddressHashDefinition {
	typedef const sockaddr* KeyType;
	typedef InterfaceAddress ValueType;

	AddressHashDefinition()
	{
	}

	size_t HashKey(const KeyType& key) const
	{
		net_domain* domain = get_domain(key->sa_family);
		if (domain == NULL)
			return 0;

		return domain->address_module->hash_address(key, false);
	}

	size_t Hash(InterfaceAddress* address) const
	{
		return address->domain->address_module->hash_address(address->local,
			false);
	}

	bool Compare(const KeyType& key, InterfaceAddress* address) const
	{
		if (address->local == NULL)
			return key->sa_family == AF_UNSPEC;

		if (address->local->sa_family != key->sa_family)
			return false;

		return address->domain->address_module->equal_addresses(key,
			address->local);
	}

	InterfaceAddress*& GetLink(InterfaceAddress* address) const
	{
		return address->HashTableLink();
	}
};

typedef BOpenHashTable<AddressHashDefinition, true, false> AddressTable;


static recursive_lock sLock;
static InterfaceList sInterfaces;
static mutex sHashLock;
static AddressTable sAddressTable;
static uint32 sInterfaceIndex;


#if 0
//! For debugging purposes only
void
dump_interface_refs(void)
{
	RecursiveLocker locker(sLock);

	InterfaceList::Iterator iterator = sInterfaces.GetIterator();
	while (Interface* interface = iterator.Next()) {
		dprintf("%p: %s, %ld\n", interface, interface->name,
			interface->CountReferences());
	}
}
#endif


#if ENABLE_DEBUGGER_COMMANDS


static int
dump_interface(int argc, char** argv)
{
	if (argc != 2) {
		kprintf("usage: %s [name|address]\n", argv[0]);
		return 0;
	}

	Interface* interface = NULL;

	InterfaceList::Iterator iterator = sInterfaces.GetIterator();
	while ((interface = iterator.Next()) != NULL) {
		if (!strcmp(argv[1], interface->name))
			break;
	}

	if (interface == NULL)
		interface = (Interface*)parse_expression(argv[1]);

	interface->Dump();

	return 0;
}


static int
dump_interfaces(int argc, char** argv)
{
	InterfaceList::Iterator iterator = sInterfaces.GetIterator();
	while (Interface* interface = iterator.Next()) {
		kprintf("%p  %s\n", interface, interface->name);
	}
	return 0;
}


static int
dump_local(int argc, char** argv)
{
	AddressTable::Iterator iterator = sAddressTable.GetIterator();
	size_t i = 0;
	while (InterfaceAddress* address = iterator.Next()) {
		address->Dump(++i);
		dprintf("    hash:          %" B_PRIu32 "\n",
			address->domain->address_module->hash_address(address->local,
				false));
	}
	return 0;
}


static int
dump_route(int argc, char** argv)
{
	if (argc != 2) {
		kprintf("usage: %s [address]\n", argv[0]);
		return 0;
	}

	net_route* route = (net_route*)parse_expression(argv[1]);
	kprintf("destination:       %p\n", route->destination);
	kprintf("mask:              %p\n", route->mask);
	kprintf("gateway:           %p\n", route->gateway);
	kprintf("flags:             %" B_PRIx32 "\n", route->flags);
	kprintf("mtu:               %" B_PRIu32 "\n", route->mtu);
	kprintf("interface address: %p\n", route->interface_address);

	if (route->interface_address != NULL) {
		((InterfaceAddress*)route->interface_address)->Dump();
	}

	return 0;
}


#endif	// ENABLE_DEBUGGER_COMMANDS


InterfaceAddress::InterfaceAddress()
{
	_Init(NULL, NULL);
}


InterfaceAddress::InterfaceAddress(net_interface* netInterface,
	net_domain* netDomain)
{
	_Init(netInterface, netDomain);
}


InterfaceAddress::~InterfaceAddress()
{
	TRACE("InterfaceAddress %p: destructor\n", this);

	if (interface != NULL && (flags & IFAF_DIRECT_ADDRESS) == 0)
		((Interface*)interface)->ReleaseReference();
}


status_t
InterfaceAddress::SetTo(const ifaliasreq& request)
{
	status_t status = SetLocal((const sockaddr*)&request.ifra_addr);
	if (status == B_OK)
		status = SetDestination((const sockaddr*)&request.ifra_broadaddr);
	if (status == B_OK)
		status = SetMask((const sockaddr*)&request.ifra_mask);

	return status;
}


status_t
InterfaceAddress::SetLocal(const sockaddr* to)
{
	return Set(&local, to);
}


status_t
InterfaceAddress::SetDestination(const sockaddr* to)
{
	return Set(&destination, to);
}


status_t
InterfaceAddress::SetMask(const sockaddr* to)
{
	return Set(&mask, to);
}


sockaddr**
InterfaceAddress::AddressFor(int32 option)
{
	switch (option) {
		case SIOCSIFADDR:
		case SIOCGIFADDR:
		case SIOCDIFADDR:
			return &local;

		case SIOCSIFNETMASK:
		case SIOCGIFNETMASK:
			return &mask;

		case SIOCSIFBRDADDR:
		case SIOCSIFDSTADDR:
		case SIOCGIFBRDADDR:
		case SIOCGIFDSTADDR:
			return &destination;

		default:
			return NULL;
	}
}


/*!	Adds the default routes that every interface address needs, ie. the local
	host route, and one for the subnet (if set).
*/
void
InterfaceAddress::AddDefaultRoutes(int32 option)
{
	net_route route;
	route.destination = local;
	route.gateway = NULL;
	route.interface_address = this;

	if (mask != NULL && (option == SIOCSIFNETMASK || option == SIOCSIFADDR)) {
		route.mask = mask;
		route.flags = 0;
		add_route(domain, &route);
	}

	if (option == SIOCSIFADDR) {
		route.mask = NULL;
		route.flags = RTF_LOCAL | RTF_HOST;
		add_route(domain, &route);
	}
}


/*!	Removes the default routes as set by AddDefaultRoutes() again. */
void
InterfaceAddress::RemoveDefaultRoutes(int32 option)
{
	net_route route;
	route.destination = local;
	route.gateway = NULL;
	route.interface_address = this;

	if (mask != NULL && (option == SIOCSIFNETMASK || option == SIOCSIFADDR)) {
		route.mask = mask;
		route.flags = 0;
		remove_route(domain, &route);
	}

	if (option == SIOCSIFADDR) {
		route.mask = NULL;
		route.flags = RTF_LOCAL | RTF_HOST;
		remove_route(domain, &route);
	}
}


bool
InterfaceAddress::LocalIsDefined() const
{
	return local != NULL && local->sa_family != AF_UNSPEC;
}


#if ENABLE_DEBUGGER_COMMANDS


void
InterfaceAddress::Dump(size_t index, bool hideInterface)
{
	if (index)
		kprintf("%2zu. ", index);
	else
		kprintf("    ");

	if (!hideInterface) {
		kprintf("interface:   %p (%s)\n    ", interface,
			interface != NULL ? interface->name : "-");
	}

	kprintf("domain:      %p (family %u)\n", domain,
		domain != NULL ? domain->family : AF_UNSPEC);

	char buffer[64];
	if (local != NULL && domain != NULL) {
		domain->address_module->print_address_buffer(local, buffer,
			sizeof(buffer), false);
	} else
		strcpy(buffer, "-");
	kprintf("    local:       %s\n", buffer);

	if (mask != NULL && domain != NULL) {
		domain->address_module->print_address_buffer(mask, buffer,
			sizeof(buffer), false);
	} else
		strcpy(buffer, "-");
	kprintf("    mask:        %s\n", buffer);

	if (destination != NULL && domain != NULL) {
		domain->address_module->print_address_buffer(destination, buffer,
			sizeof(buffer), false);
	} else
		strcpy(buffer, "-");
	kprintf("    destination: %s\n", buffer);

	kprintf("    ref count:   %" B_PRId32 "\n", CountReferences());
}


#endif	// ENABLE_DEBUGGER_COMMANDS


/*static*/ status_t
InterfaceAddress::Set(sockaddr** _address, const sockaddr* to)
{
	sockaddr* address = *_address;

	if (to == NULL || to->sa_family == AF_UNSPEC) {
		// Clear address
		free(address);
		*_address = NULL;
		return B_OK;
	}

	// Set address

	size_t size = max_c(to->sa_len, sizeof(sockaddr));
	if (size > sizeof(sockaddr_storage))
		size = sizeof(sockaddr_storage);

	address = Prepare(_address, size);
	if (address == NULL)
		return B_NO_MEMORY;

	memcpy(address, to, size);
	address->sa_len = size;

	return B_OK;
}


/*!	Makes sure that the sockaddr object pointed to by \a _address is large
	enough to hold \a size bytes.
	\a _address may point to NULL when calling this method.
*/
/*static*/ sockaddr*
InterfaceAddress::Prepare(sockaddr** _address, size_t size)
{
	size = max_c(size, sizeof(sockaddr));
	if (size > sizeof(sockaddr_storage))
		size = sizeof(sockaddr_storage);

	sockaddr* address = *_address;

	if (address == NULL || size > address->sa_len) {
		sockaddr* resized = (sockaddr*)realloc(address, size);
		if (resized == NULL) {
			free(address);
			return NULL;
		}

		address = resized;
	}

	address->sa_len = size;

	*_address = address;
	return address;
}


void
InterfaceAddress::_Init(net_interface* netInterface, net_domain* netDomain)
{
	TRACE("InterfaceAddress %p: init interface %p, domain %p\n", this,
		netInterface, netDomain);

	interface = netInterface;
	domain = netDomain;
	local = NULL;
	destination = NULL;
	mask = NULL;
	flags = 0;

	if (interface != NULL)
		((Interface*)interface)->AcquireReference();
}


// #pragma mark -


Interface::Interface(const char* interfaceName,
	net_device_interface* deviceInterface)
	:
	fBusy(false)
{
	TRACE("Interface %p: new \"%s\", device interface %p\n", this,
		interfaceName, deviceInterface);

	int written = strlcpy(name, interfaceName, IF_NAMESIZE);
	memset(name + written, 0, IF_NAMESIZE - written);
		// Clear remaining space

	device = deviceInterface->device;

	index = ++sInterfaceIndex;
	flags = 0;
	type = 0;
	mtu = deviceInterface->device->mtu;
	metric = 0;

	fDeviceInterface = acquire_device_interface(deviceInterface);

	recursive_lock_init(&fLock, name);

	// Grab a reference to the networking stack, to make sure it won't be
	// unloaded as long as an interface exists
	module_info* module;
	get_module(gNetStackInterfaceModule.info.name, &module);
}


Interface::~Interface()
{
	TRACE("Interface %p: destructor\n", this);

	// Uninitialize the domain datalink protocols

	DatalinkTable::Iterator iterator = fDatalinkTable.GetIterator();
	while (domain_datalink* datalink = iterator.Next()) {
		put_domain_datalink_protocols(this, datalink->domain);
	}

	// Free domain datalink objects

	domain_datalink* next = fDatalinkTable.Clear(true);
	while (next != NULL) {
		domain_datalink* datalink = next;
		next = next->hash_link;

		delete datalink;
	}

	put_device_interface(fDeviceInterface);

	recursive_lock_destroy(&fLock);

	// Release reference of the stack - at this point, our stack may be unloaded
	// if no other interfaces or sockets are left
	put_module(gNetStackInterfaceModule.info.name);
}


/*!	Returns a reference to the first InterfaceAddress that is from the same
	as the specified \a family.
*/
InterfaceAddress*
Interface::FirstForFamily(int family)
{
	RecursiveLocker locker(fLock);

	InterfaceAddress* address = _FirstForFamily(family);
	if (address != NULL) {
		address->AcquireReference();
		return address;
	}

	return NULL;
}


/*!	Returns a reference to the first unconfigured address of this interface
	for the specified \a family.
*/
InterfaceAddress*
Interface::FirstUnconfiguredForFamily(int family)
{
	RecursiveLocker locker(fLock);

	AddressList::Iterator iterator = fAddresses.GetIterator();
	while (InterfaceAddress* address = iterator.Next()) {
		if (address->domain->family == family
			&& (address->local == NULL
				// TODO: this has to be solved differently!!
				|| (flags & IFF_CONFIGURING) != 0)) {
			address->AcquireReference();
			return address;
		}
	}

	return NULL;
}


/*!	Returns a reference to the InterfaceAddress that has the specified
	\a destination address.
*/
InterfaceAddress*
Interface::AddressForDestination(net_domain* domain,
	const sockaddr* destination)
{
	RecursiveLocker locker(fLock);

	if ((device->flags & IFF_BROADCAST) == 0) {
		// The device does not support broadcasting
		return NULL;
	}

	AddressList::Iterator iterator = fAddresses.GetIterator();
	while (InterfaceAddress* address = iterator.Next()) {
		if (address->domain == domain
			&& address->destination != NULL
			&& domain->address_module->equal_addresses(address->destination,
					destination)) {
			address->AcquireReference();
			return address;
		}
	}

	return NULL;
}


/*!	Returns a reference to the InterfaceAddress that has the specified
	\a local address.
*/
InterfaceAddress*
Interface::AddressForLocal(net_domain* domain, const sockaddr* local)
{
	RecursiveLocker locker(fLock);

	AddressList::Iterator iterator = fAddresses.GetIterator();
	while (InterfaceAddress* address = iterator.Next()) {
		if (address->domain == domain
			&& address->local != NULL
			&& domain->address_module->equal_addresses(address->local, local)) {
			address->AcquireReference();
			return address;
		}
	}

	return NULL;
}


status_t
Interface::AddAddress(InterfaceAddress* address)
{
	net_domain* domain = address->domain;
	if (domain == NULL)
		return B_BAD_VALUE;

	RecursiveLocker locker(fLock);
	fAddresses.Add(address);
	locker.Unlock();

	if (address->LocalIsDefined()) {
		MutexLocker hashLocker(sHashLock);
		sAddressTable.Insert(address);
	}
	return B_OK;
}


void
Interface::RemoveAddress(InterfaceAddress* address)
{
	net_domain* domain = address->domain;
	if (domain == NULL)
		return;

	RecursiveLocker locker(fLock);

	fAddresses.Remove(address);
	address->GetDoublyLinkedListLink()->next = NULL;

	locker.Unlock();

	if (address->LocalIsDefined()) {
		MutexLocker hashLocker(sHashLock);
		sAddressTable.Remove(address);
	}
}


bool
Interface::GetNextAddress(InterfaceAddress** _address)
{
	RecursiveLocker locker(fLock);

	InterfaceAddress* address = *_address;
	if (address == NULL) {
		// get first address
		address = fAddresses.First();
	} else {
		// get next, if possible
		InterfaceAddress* next = fAddresses.GetNext(address);
		address->ReleaseReference();
		address = next;
	}

	*_address = address;

	if (address == NULL)
		return false;

	address->AcquireReference();
	return true;
}


InterfaceAddress*
Interface::AddressAt(size_t index)
{
	RecursiveLocker locker(fLock);

	AddressList::Iterator iterator = fAddresses.GetIterator();
	size_t i = 0;

	while (InterfaceAddress* address = iterator.Next()) {
		if (i++ == index) {
			address->AcquireReference();
			return address;
		}
	}

	return NULL;
}


int32
Interface::IndexOfAddress(InterfaceAddress* address)
{
	if (address == NULL)
		return -1;

	RecursiveLocker locker(fLock);

	AddressList::Iterator iterator = fAddresses.GetIterator();
	int32 index = 0;

	while (iterator.HasNext()) {
		if (address == iterator.Next())
			return index;

		index++;
	}

	return -1;
}


size_t
Interface::CountAddresses()
{
	RecursiveLocker locker(fLock);
	return fAddresses.Count();
}


void
Interface::RemoveAddresses()
{
	RecursiveLocker locker(fLock);

	while (InterfaceAddress* address = fAddresses.RemoveHead()) {
		locker.Unlock();

		if (address->LocalIsDefined()) {
			MutexLocker hashLocker(sHashLock);
			sAddressTable.Remove(address);
		}
		address->ReleaseReference();

		locker.Lock();
	}
}


/*!	This is called in order to call the correct methods of the datalink
	protocols, ie. it will translate address changes to
	net_datalink_protocol::change_address(), and IFF_UP changes to
	net_datalink_protocol::interface_up(), and interface_down().

	Everything else is passed unchanged to net_datalink_protocol::control().
*/
status_t
Interface::Control(net_domain* domain, int32 option, ifreq& request,
	ifreq* userRequest, size_t length)
{
	switch (option) {
		case SIOCSIFFLAGS:
		{
			if (length != sizeof(ifreq))
				return B_BAD_VALUE;

			uint32 requestFlags = request.ifr_flags;
			uint32 oldFlags = flags;
			status_t status = B_OK;

			request.ifr_flags &= ~(IFF_UP | IFF_LINK | IFF_BROADCAST);

			if ((requestFlags & IFF_UP) != (flags & IFF_UP)) {
				if ((requestFlags & IFF_UP) != 0)
					status = _SetUp();
				else
					SetDown();
			}

			if (status == B_OK) {
				// TODO: maybe allow deleting IFF_BROADCAST on the interface
				// level?
				flags &= IFF_UP | IFF_LINK | IFF_BROADCAST;
				flags |= request.ifr_flags;
			}

			if (oldFlags != flags) {
				TRACE("Interface %p: flags changed from %" B_PRIx32 " to %"
					B_PRIx32 "\n", this, oldFlags, flags);
				notify_interface_changed(this, oldFlags, flags);
			}

			return status;
		}

		case B_SOCKET_SET_ALIAS:
		{
			if (length != sizeof(ifaliasreq))
				return B_BAD_VALUE;

			RecursiveLocker locker(fLock);

			ifaliasreq aliasRequest;
			if (user_memcpy(&aliasRequest, userRequest, sizeof(ifaliasreq))
					!= B_OK)
				return B_BAD_ADDRESS;

			InterfaceAddress* address = NULL;
			if (aliasRequest.ifra_index < 0) {
				if (!domain->address_module->is_empty_address(
						(const sockaddr*)&aliasRequest.ifra_addr, false)) {
					// Find first address that matches the local address
					address = AddressForLocal(domain,
						(const sockaddr*)&aliasRequest.ifra_addr);
				}
				if (address == NULL) {
					// Find first address for family
					address = FirstForFamily(domain->family);
				}
				if (address == NULL) {
					// Create new on the fly
					address = new(std::nothrow) InterfaceAddress(this, domain);
					if (address == NULL)
						return B_NO_MEMORY;

					status_t status = AddAddress(address);
					if (status != B_OK) {
						delete address;
						return status;
					}

					// Note, even if setting the address failed, the empty
					// address added here will still be added to the interface.
					address->AcquireReference();
				}
			} else
				address = AddressAt(aliasRequest.ifra_index);

			if (address == NULL)
				return B_BAD_VALUE;

			status_t status = B_OK;

			if (!domain->address_module->equal_addresses(
					(sockaddr*)&aliasRequest.ifra_addr, address->local)) {
				status = _ChangeAddress(locker, address, SIOCSIFADDR,
					address->local, (sockaddr*)&aliasRequest.ifra_addr);
			}

			if (status == B_OK && !domain->address_module->equal_addresses(
					(sockaddr*)&aliasRequest.ifra_mask, address->mask)
				&& !domain->address_module->is_empty_address(
					(sockaddr*)&aliasRequest.ifra_mask, false)) {
				status = _ChangeAddress(locker, address, SIOCSIFNETMASK,
					address->mask, (sockaddr*)&aliasRequest.ifra_mask);
			}

			if (status == B_OK && !domain->address_module->equal_addresses(
					(sockaddr*)&aliasRequest.ifra_destination,
					address->destination)
				&& !domain->address_module->is_empty_address(
					(sockaddr*)&aliasRequest.ifra_destination, false)) {
				status = _ChangeAddress(locker, address,
					(domain->address_module->flags
						& NET_ADDRESS_MODULE_FLAG_BROADCAST_ADDRESS) != 0
							? SIOCSIFBRDADDR : SIOCSIFDSTADDR,
					address->destination,
					(sockaddr*)&aliasRequest.ifra_destination);
			}

			address->ReleaseReference();
			return status;
		}

		case SIOCSIFADDR:
		case SIOCSIFNETMASK:
		case SIOCSIFBRDADDR:
		case SIOCSIFDSTADDR:
		case SIOCDIFADDR:
		{
			if (length != sizeof(ifreq))
				return B_BAD_VALUE;

			RecursiveLocker locker(fLock);

			InterfaceAddress* address = NULL;
			sockaddr_storage newAddress;

			size_t size = max_c(request.ifr_addr.sa_len, sizeof(sockaddr));
			if (size > sizeof(sockaddr_storage))
				size = sizeof(sockaddr_storage);

			if (user_memcpy(&newAddress, &userRequest->ifr_addr, size) != B_OK)
				return B_BAD_ADDRESS;

			if (option == SIOCDIFADDR) {
				// Find referring address - we can't use the hash, as another
				// interface might use the same address.
				AddressList::Iterator iterator = fAddresses.GetIterator();
				while ((address = iterator.Next()) != NULL) {
					if (address->domain == domain
						&& domain->address_module->equal_addresses(
							address->local, (sockaddr*)&newAddress))
						break;
				}

				if (address == NULL)
					return B_BAD_VALUE;
			} else {
				// Just use the first address for this family
				address = _FirstForFamily(domain->family);
				if (address == NULL) {
					// Create new on the fly
					address = new(std::nothrow) InterfaceAddress(this, domain);
					if (address == NULL)
						return B_NO_MEMORY;

					status_t status = AddAddress(address);
					if (status != B_OK) {
						delete address;
						return status;
					}

					// Note, even if setting the address failed, the empty
					// address added here will still be added to the interface.
				}
			}

			return _ChangeAddress(locker, address, option,
				*address->AddressFor(option),
				option != SIOCDIFADDR ? (sockaddr*)&newAddress : NULL);
		}

		default:
			// pass the request into the datalink protocol stack
			domain_datalink* datalink = DomainDatalink(domain->family);
			if (datalink->first_info != NULL) {
				return datalink->first_info->control(
					datalink->first_protocol, option, userRequest, length);
			}
			break;
	}

	return B_BAD_VALUE;
}


void
Interface::SetDown()
{
	if ((flags & IFF_UP) == 0)
		return;

	RecursiveLocker interfacesLocker(sLock);

	if (IsBusy())
		return;

	SetBusy(true);
	interfacesLocker.Unlock();

	DatalinkTable::Iterator iterator = fDatalinkTable.GetIterator();
	while (domain_datalink* datalink = iterator.Next()) {
		datalink->first_info->interface_down(datalink->first_protocol);
	}

	flags &= ~IFF_UP;

	SetBusy(false);
}


/*!	Called when a device lost its IFF_UP status. We will invalidate all
	interface routes here.
*/
void
Interface::WentDown()
{
	TRACE("Interface %p: went down\n", this);

	RecursiveLocker locker(fLock);

	AddressList::Iterator iterator = fAddresses.GetIterator();
	while (InterfaceAddress* address = iterator.Next()) {
		if (address->domain != NULL)
			invalidate_routes(address->domain, this);
	}
}


status_t
Interface::CreateDomainDatalinkIfNeeded(net_domain* domain)
{
	RecursiveLocker locker(fLock);

	if (fDatalinkTable.Lookup(domain->family) != NULL)
		return B_OK;

	TRACE("Interface %p: create domain datalink for domain %p\n", this, domain);

	domain_datalink* datalink = new(std::nothrow) domain_datalink;
	if (datalink == NULL)
		return B_NO_MEMORY;

	datalink->first_protocol = NULL;
	datalink->first_info = NULL;
	datalink->domain = domain;

	// setup direct route for bound devices
	datalink->direct_route.destination = NULL;
	datalink->direct_route.mask = NULL;
	datalink->direct_route.gateway = NULL;
	datalink->direct_route.flags = 0;
	datalink->direct_route.mtu = 0;
	datalink->direct_route.interface_address = &datalink->direct_address;
	datalink->direct_route.ref_count = 1;
		// make sure this doesn't get deleted accidently

	// provide its link back to the interface
	datalink->direct_address.local = NULL;
	datalink->direct_address.destination = NULL;
	datalink->direct_address.mask = NULL;
	datalink->direct_address.domain = domain;
	datalink->direct_address.interface = this;
	datalink->direct_address.flags = IFAF_DIRECT_ADDRESS;

	fDatalinkTable.Insert(datalink);

	status_t status = get_domain_datalink_protocols(this, domain);
	if (status == B_OK)
		return B_OK;

	fDatalinkTable.Remove(datalink);
	delete datalink;

	return status;
}


domain_datalink*
Interface::DomainDatalink(uint8 family)
{
	// Note: domain datalinks cannot be removed while the interface is alive,
	// since this would require us either to hold the lock while calling this
	// function, or introduce reference counting for the domain_datalink
	// structure.
	RecursiveLocker locker(fLock);
	return fDatalinkTable.Lookup(family);
}


#if ENABLE_DEBUGGER_COMMANDS


void
Interface::Dump() const
{
	kprintf("name:                %s\n", name);
	kprintf("device:              %p\n", device);
	kprintf("device_interface:    %p\n", fDeviceInterface);
	kprintf("index:               %" B_PRIu32 "\n", index);
	kprintf("flags:               %#" B_PRIx32 "\n", flags);
	kprintf("type:                %u\n", type);
	kprintf("mtu:                 %" B_PRIu32 "\n", mtu);
	kprintf("metric:              %" B_PRIu32 "\n", metric);
	kprintf("ref count:           %" B_PRId32 "\n", CountReferences());

	kprintf("datalink protocols:\n");

	DatalinkTable::Iterator datalinkIterator = fDatalinkTable.GetIterator();
	size_t i = 0;
	while (domain_datalink* datalink = datalinkIterator.Next()) {
		kprintf("%2zu. domain:          %p\n", ++i, datalink->domain);
		kprintf("    first_protocol:  %p\n", datalink->first_protocol);
		kprintf("    first_info:      %p\n", datalink->first_info);
		kprintf("    direct_route:    %p\n", &datalink->direct_route);
	}

	kprintf("addresses:\n");

	AddressList::ConstIterator iterator = fAddresses.GetIterator();
	i = 0;
	while (InterfaceAddress* address = iterator.Next()) {
		address->Dump(++i, true);
	}
}


#endif	// ENABLE_DEBUGGER_COMMANDS


status_t
Interface::_SetUp()
{
	status_t status = up_device_interface(fDeviceInterface);
	if (status != B_OK)
		return status;

	RecursiveLocker interfacesLocker(sLock);
	SetBusy(true);
	interfacesLocker.Unlock();

	// Propagate flag to all datalink protocols
	DatalinkTable::Iterator iterator = fDatalinkTable.GetIterator();
	while (domain_datalink* datalink = iterator.Next()) {
		status = datalink->first_info->interface_up(datalink->first_protocol);
		if (status != B_OK) {
			// Revert "up" status
			DatalinkTable::Iterator secondIterator
				= fDatalinkTable.GetIterator();
			while (secondIterator.HasNext()) {
				domain_datalink* secondDatalink = secondIterator.Next();
				if (secondDatalink == NULL || secondDatalink == datalink)
					break;

				secondDatalink->first_info->interface_down(
					secondDatalink->first_protocol);
			}

			down_device_interface(fDeviceInterface);
			SetBusy(false);
			return status;
		}
	}

	// Add default routes for the existing addresses

	AddressList::Iterator addressIterator = fAddresses.GetIterator();
	while (InterfaceAddress* address = addressIterator.Next()) {
		address->AddDefaultRoutes(SIOCSIFADDR);
	}

	flags |= IFF_UP;
	SetBusy(false);

	return B_OK;
}


InterfaceAddress*
Interface::_FirstForFamily(int family)
{
	ASSERT_LOCKED_RECURSIVE(&fLock);

	AddressList::Iterator iterator = fAddresses.GetIterator();
	while (InterfaceAddress* address = iterator.Next()) {
		if (address->domain != NULL && address->domain->family == family)
			return address;
	}

	return NULL;
}


status_t
Interface::_ChangeAddress(RecursiveLocker& locker, InterfaceAddress* address,
	int32 option, const sockaddr* originalAddress,
	const sockaddr* requestedAddress)
{
	// Copy old address
	sockaddr_storage oldAddress;
	if (address->domain->address_module->set_to((sockaddr*)&oldAddress,
			originalAddress) != B_OK)
		oldAddress.ss_family = AF_UNSPEC;

	// Copy new address (this also makes sure that sockaddr::sa_len is set
	// correctly)
	sockaddr_storage newAddress;
	if (address->domain->address_module->set_to((sockaddr*)&newAddress,
			requestedAddress) != B_OK)
		newAddress.ss_family = AF_UNSPEC;

	// Test if anything changed for real
	if (address->domain->address_module->equal_addresses(
			(sockaddr*)&oldAddress, (sockaddr*)&newAddress)) {
		// Nothing to do
		TRACE("  option %" B_PRId32 " addresses are equal!\n", option);
		return B_OK;
	}

	// TODO: mark this address busy or call while holding the lock!
	address->AcquireReference();
	locker.Unlock();

	domain_datalink* datalink = DomainDatalink(address->domain);
	status_t status = datalink->first_protocol->module->change_address(
		datalink->first_protocol, address, option,
		oldAddress.ss_family != AF_UNSPEC ? (sockaddr*)&oldAddress : NULL,
		newAddress.ss_family != AF_UNSPEC ? (sockaddr*)&newAddress : NULL);

	locker.Lock();
	address->ReleaseReference();
	return status;
}


// #pragma mark -


/*!	Searches for a specific interface by name.
	You need to have the interface list's lock hold when calling this function.
*/
static Interface*
find_interface(const char* name)
{
	ASSERT_LOCKED_RECURSIVE(&sLock);

	InterfaceList::Iterator iterator = sInterfaces.GetIterator();
	while (Interface* interface = iterator.Next()) {
		if (!strcmp(interface->name, name))
			return interface;
	}

	return NULL;
}


/*!	Searches for a specific interface by index.
	You need to have the interface list's lock hold when calling this function.
*/
static Interface*
find_interface(uint32 index)
{
	InterfaceList::Iterator iterator = sInterfaces.GetIterator();
	while (Interface* interface = iterator.Next()) {
		if (interface->index == index)
			return interface;
	}

	return NULL;
}


// #pragma mark -


status_t
add_interface(const char* name, net_domain_private* domain,
	const ifaliasreq& request, net_device_interface* deviceInterface)
{
	RecursiveLocker locker(sLock);

	if (find_interface(name) != NULL)
		return B_NAME_IN_USE;

	Interface* interface
		= new(std::nothrow) Interface(name, deviceInterface);
	if (interface == NULL)
		return B_NO_MEMORY;

	sInterfaces.Add(interface);
	interface->AcquireReference();
		// We need another reference to be able to use the interface without
		// holding sLock.

	locker.Unlock();

	status_t status = add_interface_address(interface, domain, request);
	if (status == B_OK)
		notify_interface_added(interface);
	else {
		locker.Lock();
		sInterfaces.Remove(interface);
		locker.Unlock();
		interface->ReleaseReference();
	}

	interface->ReleaseReference();

	return status;
}


/*!	Removes the interface from the list, and puts the stack's reference to it.
*/
void
remove_interface(Interface* interface)
{
	interface->SetDown();
	interface->RemoveAddresses();

	RecursiveLocker locker(sLock);
	sInterfaces.Remove(interface);
	locker.Unlock();

	notify_interface_removed(interface);

	interface->ReleaseReference();
}


/*!	This is called whenever a device interface is being removed. We will get
	the corresponding Interface, and remove it.
*/
void
interface_removed_device_interface(net_device_interface* deviceInterface)
{
	RecursiveLocker locker(sLock);

	Interface* interface = find_interface(deviceInterface->device->name);
	if (interface != NULL)
		remove_interface(interface);
}


status_t
add_interface_address(Interface* interface, net_domain_private* domain,
	const ifaliasreq& request)
{
	// Make sure the family of the provided addresses is valid
	if ((request.ifra_addr.ss_family != domain->family
			&& request.ifra_addr.ss_family != AF_UNSPEC)
		|| (request.ifra_mask.ss_family != domain->family
			&& request.ifra_mask.ss_family != AF_UNSPEC)
		|| (request.ifra_broadaddr.ss_family != domain->family
			&& request.ifra_broadaddr.ss_family != AF_UNSPEC))
		return B_BAD_VALUE;

	RecursiveLocker locker(interface->Lock());

	InterfaceAddress* address
		= new(std::nothrow) InterfaceAddress(interface, domain);
	if (address == NULL)
		return B_NO_MEMORY;

	status_t status = address->SetTo(request);
	if (status == B_OK)
		status = interface->CreateDomainDatalinkIfNeeded(domain);
	if (status == B_OK)
		status = interface->AddAddress(address);

	if (status == B_OK && address->local != NULL) {
		// update the datalink protocols
		domain_datalink* datalink = interface->DomainDatalink(domain->family);

		status = datalink->first_protocol->module->change_address(
			datalink->first_protocol, address, SIOCAIFADDR, NULL,
			address->local);
		if (status != B_OK)
			interface->RemoveAddress(address);
	}
	if (status == B_OK)
		notify_interface_changed(interface);
	else
		delete address;

	return status;
}


status_t
update_interface_address(InterfaceAddress* interfaceAddress, int32 option,
	const sockaddr* oldAddress, const sockaddr* newAddress)
{
	TRACE("%s(address %p, option %" B_PRId32 ", oldAddress %s, newAddress "
		"%s)\n", __FUNCTION__, interfaceAddress, option,
		AddressString(interfaceAddress->domain, oldAddress).Data(),
		AddressString(interfaceAddress->domain, newAddress).Data());

	MutexLocker locker(sHashLock);

	// set logical interface address
	sockaddr** _address = interfaceAddress->AddressFor(option);
	if (_address == NULL)
		return B_BAD_VALUE;

	Interface* interface = (Interface*)interfaceAddress->interface;

	interfaceAddress->RemoveDefaultRoutes(option);

	if (option == SIOCDIFADDR) {
		// Remove address, and release its reference (causing our caller to
		// delete it)
		locker.Unlock();

		invalidate_routes(interfaceAddress);

		interface->RemoveAddress(interfaceAddress);
		interfaceAddress->ReleaseReference();
		return B_OK;
	}

	if (interfaceAddress->LocalIsDefined())
		sAddressTable.Remove(interfaceAddress);

	// Copy new address over
	status_t status = InterfaceAddress::Set(_address, newAddress);
	if (status == B_OK) {
		sockaddr* address = *_address;

		if (option == SIOCSIFADDR || option == SIOCSIFNETMASK) {
			// Reset netmask and broadcast addresses to defaults
			net_domain* domain = interfaceAddress->domain;
			sockaddr* defaultNetmask = NULL;
			const sockaddr* netmask = NULL;
			if (option == SIOCSIFADDR) {
				defaultNetmask = InterfaceAddress::Prepare(
					&interfaceAddress->mask, address->sa_len);
			} else
				netmask = newAddress;

			// Reset the broadcast address if the address family has
			// such
			sockaddr* defaultBroadcast = NULL;
			if ((domain->address_module->flags
					& NET_ADDRESS_MODULE_FLAG_BROADCAST_ADDRESS) != 0) {
				defaultBroadcast = InterfaceAddress::Prepare(
					&interfaceAddress->destination, address->sa_len);
			} else
				InterfaceAddress::Set(&interfaceAddress->destination, NULL);

			domain->address_module->set_to_defaults(defaultNetmask,
				defaultBroadcast, interfaceAddress->local, netmask);
		}

		interfaceAddress->AddDefaultRoutes(option);
		notify_interface_changed(interface);
	}

	if (interfaceAddress->LocalIsDefined())
		sAddressTable.Insert(interfaceAddress);
	return status;
}


Interface*
get_interface(net_domain* domain, uint32 index)
{
	RecursiveLocker locker(sLock);

	Interface* interface;
	if (index == 0)
		interface = sInterfaces.First();
	else
		interface = find_interface(index);
	if (interface == NULL || interface->IsBusy())
		return NULL;

	if (interface->CreateDomainDatalinkIfNeeded(domain) != B_OK)
		return NULL;

	interface->AcquireReference();
	return interface;
}


Interface*
get_interface(net_domain* domain, const char* name)
{
	RecursiveLocker locker(sLock);

	Interface* interface = find_interface(name);
	if (interface == NULL || interface->IsBusy())
		return NULL;

	if (interface->CreateDomainDatalinkIfNeeded(domain) != B_OK)
		return NULL;

	interface->AcquireReference();
	return interface;
}


Interface*
get_interface_for_device(net_domain* domain, uint32 index)
{
	RecursiveLocker locker(sLock);

	InterfaceList::Iterator iterator = sInterfaces.GetIterator();
	while (Interface* interface = iterator.Next()) {
		if (interface->device->index == index) {
			if (interface->IsBusy())
				return NULL;
			if (interface->CreateDomainDatalinkIfNeeded(domain) != B_OK)
				return NULL;

			interface->AcquireReference();
			return interface;
		}
	}

	return NULL;
}


/*!	Returns a reference to an Interface that matches the given \a linkAddress.
	The link address is checked against its hardware address, or its interface
	name, or finally the interface index.
*/
Interface*
get_interface_for_link(net_domain* domain, const sockaddr* _linkAddress)
{
	sockaddr_dl& linkAddress = *(sockaddr_dl*)_linkAddress;

	RecursiveLocker locker(sLock);

	InterfaceList::Iterator iterator = sInterfaces.GetIterator();
	while (Interface* interface = iterator.Next()) {
		if (interface->IsBusy())
			continue;
		// Test if the hardware address matches, or if the given interface
		// matches, or if at least the index matches.
		if ((linkAddress.sdl_alen == interface->device->address.length
				&& memcmp(LLADDR(&linkAddress), interface->device->address.data,
					linkAddress.sdl_alen) == 0)
			|| (linkAddress.sdl_nlen > 0
				&& !strcmp(interface->name, (const char*)linkAddress.sdl_data))
			|| (linkAddress.sdl_nlen == 0 && linkAddress.sdl_alen == 0
				&& linkAddress.sdl_index == interface->index)) {
			if (interface->IsBusy())
				return NULL;
			if (interface->CreateDomainDatalinkIfNeeded(domain) != B_OK)
				return NULL;

			interface->AcquireReference();
			return interface;
		}
	}

	return NULL;
}


InterfaceAddress*
get_interface_address(const sockaddr* local)
{
	if (local->sa_family == AF_UNSPEC)
		return NULL;

	MutexLocker locker(sHashLock);

	InterfaceAddress* address = sAddressTable.Lookup(local);
	if (address == NULL)
		return NULL;

	address->AcquireReference();
	return address;
}


InterfaceAddress*
get_interface_address_for_destination(net_domain* domain,
	const sockaddr* destination)
{
	RecursiveLocker locker(sLock);

	InterfaceList::Iterator iterator = sInterfaces.GetIterator();
	while (Interface* interface = iterator.Next()) {
		if (interface->IsBusy())
			continue;

		InterfaceAddress* address
			= interface->AddressForDestination(domain, destination);
		if (address != NULL)
			return address;
	}

	return NULL;
}


/*!	Returns a reference to an InterfaceAddress of the specified \a domain that
	belongs to the interface identified via \a linkAddress. Only the hardware
	address is matched.

	If \a unconfiguredOnly is set, the interface address must not yet be
	configured, or must currently be in the process of being configured.
*/
InterfaceAddress*
get_interface_address_for_link(net_domain* domain, const sockaddr* address,
	bool unconfiguredOnly)
{
	sockaddr_dl& linkAddress = *(sockaddr_dl*)address;

	RecursiveLocker locker(sLock);

	InterfaceList::Iterator iterator = sInterfaces.GetIterator();
	while (Interface* interface = iterator.Next()) {
		if (interface->IsBusy())
			continue;
		// Test if the hardware address matches, or if the given interface
		// matches, or if at least the index matches.
		if (linkAddress.sdl_alen == interface->device->address.length
			&& memcmp(LLADDR(&linkAddress), interface->device->address.data,
				linkAddress.sdl_alen) == 0) {
			TRACE("  %s matches\n", interface->name);
			// link address matches
			if (unconfiguredOnly)
				return interface->FirstUnconfiguredForFamily(domain->family);

			return interface->FirstForFamily(domain->family);
		}
	}

	return NULL;
}


uint32
count_interfaces()
{
	RecursiveLocker locker(sLock);

	return sInterfaces.Count();
}


/*!	Dumps a list of all interfaces into the supplied userland buffer.
	If the interfaces don't fit into the buffer, an error (\c ENOBUFS) is
	returned.
*/
status_t
list_interfaces(int family, void* _buffer, size_t* bufferSize)
{
	RecursiveLocker locker(sLock);

	UserBuffer buffer(_buffer, *bufferSize);

	InterfaceList::Iterator iterator = sInterfaces.GetIterator();
	while (Interface* interface = iterator.Next()) {
		// Copy name
		buffer.Push(interface->name, IF_NAMESIZE);

		// Copy address
		InterfaceAddress* address = interface->FirstForFamily(family);
		size_t length = 0;

		if (address != NULL && address->local != NULL) {
			// Actual address
			buffer.Push(address->local, length = address->local->sa_len);
		} else {
			// Empty address
			sockaddr empty;
			empty.sa_len = 2;
			empty.sa_family = AF_UNSPEC;
			buffer.Push(&empty, length = empty.sa_len);
		}

		if (address != NULL)
			address->ReleaseReference();

		if (IF_NAMESIZE + length < sizeof(ifreq)) {
			// Make sure at least sizeof(ifreq) bytes are written for each
			// interface for compatibility with other platforms
			buffer.Pad(sizeof(ifreq) - IF_NAMESIZE - length);
		}

		if (buffer.Status() != B_OK)
			return buffer.Status();
	}

	*bufferSize = buffer.BytesConsumed();
	return B_OK;
}


//	#pragma mark -


status_t
init_interfaces()
{
	recursive_lock_init(&sLock, "net interfaces");
	mutex_init(&sHashLock, "net local addresses");

	new (&sInterfaces) InterfaceList;
	new (&sAddressTable) AddressTable;
		// static C++ objects are not initialized in the module startup

#if ENABLE_DEBUGGER_COMMANDS
	add_debugger_command("net_interface", &dump_interface,
		"Dump the given network interface");
	add_debugger_command("net_interfaces", &dump_interfaces,
		"Dump all network interfaces");
	add_debugger_command("net_local", &dump_local,
		"Dump all local interface addresses");
	add_debugger_command("net_route", &dump_route,
		"Dump the given network route");
#endif
	return B_OK;
}


status_t
uninit_interfaces()
{
#if ENABLE_DEBUGGER_COMMANDS
	remove_debugger_command("net_interface", &dump_interface);
	remove_debugger_command("net_interfaces", &dump_interfaces);
	remove_debugger_command("net_local", &dump_local);
	remove_debugger_command("net_route", &dump_route);
#endif

	recursive_lock_destroy(&sLock);
	mutex_destroy(&sHashLock);
	return B_OK;
}

