/*
 * Copyright 2006-2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef INTERFACES_H
#define INTERFACES_H


#include "routes.h"
#include "stack_private.h"

#include <net_datalink.h>
#include <net_stack.h>

#include <Referenceable.h>

#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>


struct net_device_interface;


// Additional address flags
#define IFAF_DIRECT_ADDRESS		0x1000


struct InterfaceAddress : DoublyLinkedListLinkImpl<InterfaceAddress>,
		net_interface_address, BReferenceable {
								InterfaceAddress();
								InterfaceAddress(net_interface* interface,
									net_domain* domain);
	virtual						~InterfaceAddress();

			status_t			SetTo(const ifaliasreq& request);

			status_t			SetLocal(const sockaddr* to);
			status_t			SetDestination(const sockaddr* to);
			status_t			SetMask(const sockaddr* to);

			sockaddr**			AddressFor(int32 option);

			void				AddDefaultRoutes(int32 option);
			void				RemoveDefaultRoutes(int32 option);

			bool				LocalIsDefined() const;

			InterfaceAddress*&	HashTableLink() { return fLink; }

#if ENABLE_DEBUGGER_COMMANDS
			void				Dump(size_t index = 0,
									bool hideInterface = false);
#endif

	static	status_t			Set(sockaddr** _address, const sockaddr* to);
	static	sockaddr*			Prepare(sockaddr** _address, size_t length);

private:
			void				_Init(net_interface* interface,
									net_domain* domain);

private:
			InterfaceAddress*	fLink;
};

typedef DoublyLinkedList<InterfaceAddress> AddressList;

struct domain_datalink {
	domain_datalink*	hash_link;
	net_domain*			domain;

	struct net_datalink_protocol* first_protocol;
	struct net_datalink_protocol_module_info* first_info;

	// support for binding to an interface
	net_route_private	direct_route;
	InterfaceAddress	direct_address;
};

struct DatalinkHashDefinition {
	typedef const int KeyType;
	typedef domain_datalink ValueType;

	DatalinkHashDefinition()
	{
	}

	size_t HashKey(const KeyType& key) const
	{
		return (size_t)key;
	}

	size_t Hash(domain_datalink* datalink) const
	{
		return datalink->domain->family;
	}

	bool Compare(const KeyType& key, domain_datalink* datalink) const
	{
		return datalink->domain->family == key;
	}

	domain_datalink*& GetLink(domain_datalink* datalink) const
	{
		return datalink->hash_link;
	}
};

typedef BOpenHashTable<DatalinkHashDefinition, true, true> DatalinkTable;


class Interface : public DoublyLinkedListLinkImpl<Interface>,
		public net_interface, public BReferenceable {
public:
								Interface(const char* name,
									net_device_interface* deviceInterface);
	virtual						~Interface();

			InterfaceAddress*	FirstForFamily(int family);
			InterfaceAddress*	FirstUnconfiguredForFamily(int family);
			InterfaceAddress*	AddressForDestination(net_domain* domain,
									const sockaddr* destination);
			InterfaceAddress*	AddressForLocal(net_domain* domain,
									const sockaddr* local);

			status_t			AddAddress(InterfaceAddress* address);
			void				RemoveAddress(InterfaceAddress* address);
			bool				GetNextAddress(InterfaceAddress** _address);
			InterfaceAddress*	AddressAt(size_t index);
			int32				IndexOfAddress(InterfaceAddress* address);
			size_t				CountAddresses();
			void				RemoveAddresses();

			status_t			Control(net_domain* domain, int32 option,
									ifreq& request, ifreq* userRequest,
									size_t length);

			void				SetDown();
			void				WentDown();

			recursive_lock&		Lock() { return fLock; }

			net_device_interface* DeviceInterface() { return fDeviceInterface; }

			status_t			CreateDomainDatalinkIfNeeded(
									net_domain* domain);
			domain_datalink*	DomainDatalink(uint8 family);
			domain_datalink*	DomainDatalink(net_domain* domain)
									{ return DomainDatalink(domain->family); }

	inline	void				SetBusy(bool busy) { atomic_set(&fBusy, busy ? 1 : 0); }
	inline	bool				IsBusy() const { return atomic_get((int32*)&fBusy) == 1 ; }

#if ENABLE_DEBUGGER_COMMANDS
			void				Dump() const;
#endif

private:
			status_t			_SetUp();
			InterfaceAddress*	_FirstForFamily(int family);
			status_t			_ChangeAddress(RecursiveLocker& locker,
									InterfaceAddress* address, int32 option,
									const sockaddr* oldAddress,
									const sockaddr* newAddress);

private:
			recursive_lock		fLock;
			int32				fBusy;
			net_device_interface* fDeviceInterface;
			AddressList			fAddresses;
			DatalinkTable		fDatalinkTable;
};

typedef DoublyLinkedList<Interface> InterfaceList;


status_t init_interfaces();
status_t uninit_interfaces();

// interfaces
status_t add_interface(const char* name, net_domain_private* domain,
	const ifaliasreq& request, net_device_interface* deviceInterface);
void remove_interface(Interface* interface);
void interface_removed_device_interface(net_device_interface* deviceInterface);

status_t add_interface_address(Interface* interface, net_domain_private* domain,
	const ifaliasreq& request);
status_t update_interface_address(InterfaceAddress* interfaceAddress,
	int32 option, const sockaddr* oldAddress, const sockaddr* newAddress);

Interface* get_interface(net_domain* domain, uint32 index);
Interface* get_interface(net_domain* domain, const char* name);
Interface* get_interface_for_device(net_domain* domain, uint32 index);
Interface* get_interface_for_link(net_domain* domain, const sockaddr* address);
InterfaceAddress* get_interface_address(const struct sockaddr* address);
InterfaceAddress* get_interface_address_for_destination(net_domain* domain,
	const sockaddr* destination);
InterfaceAddress* get_interface_address_for_link(net_domain* domain,
	const sockaddr* linkAddress, bool unconfiguredOnly);

uint32 count_interfaces();
status_t list_interfaces(int family, void* buffer, size_t* _bufferSize);


#endif	// INTERFACES_H
