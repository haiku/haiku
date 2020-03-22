/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */
#ifndef _PRIVATE_MULTICAST_H_
#define _PRIVATE_MULTICAST_H_


#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>

#include <net_datalink.h>

#include <netinet/in.h>

#include <utility>


struct net_buffer;
struct net_protocol;

// This code is template'ized as it is reusable for IPv6

template<typename Addressing> class MulticastFilter;
template<typename Addressing> class MulticastGroupInterface;

// TODO move this elsewhere...
struct IPv4Multicast {
	typedef struct in_addr AddressType;
	typedef struct ipv4_protocol ProtocolType;
	typedef MulticastGroupInterface<IPv4Multicast> GroupInterface;

	static status_t JoinGroup(GroupInterface *);
	static status_t LeaveGroup(GroupInterface *);

	static const in_addr &AddressFromSockAddr(const sockaddr *sockaddr)
		{ return ((const sockaddr_in *)sockaddr)->sin_addr; }
	static size_t HashAddress(const in_addr &address)
		{ return address.s_addr; }
};

template<typename AddressType>
class AddressSet {
	struct ContainedAddress : DoublyLinkedListLinkImpl<ContainedAddress> {
		AddressType address;
	};

	typedef DoublyLinkedList<ContainedAddress> AddressList;

public:
	AddressSet()
		: fCount(0) {}

	~AddressSet() { Clear(); }

	status_t Add(const AddressType &address)
	{
		if (Has(address))
			return B_OK;

		ContainedAddress *container = new ContainedAddress();
		if (container == NULL)
			return B_NO_MEMORY;

		container->address = address;
		fAddresses.Add(container);

		return B_OK;
	}

	void Remove(const AddressType &address)
	{
		ContainedAddress *container = _Get(address);
		if (container == NULL)
			return;

		fAddresses.Remove(container);
		delete container;
	}

	bool Has(const AddressType &address) const
	{
		return _Get(address) != NULL;
	}

	bool IsEmpty() const { return fAddresses.IsEmpty(); }

	void Clear()
	{
		while (!fAddresses.IsEmpty())
			Remove(fAddresses.Head()->address);
	}

	class Iterator {
	public:
		Iterator(const AddressList &addresses)
			: fBaseIterator(addresses.GetIterator()) {}

		bool HasNext() const { return fBaseIterator.HasNext(); }
		AddressType &Next() { return fBaseIterator.Next()->address; }

	private:
		typename AddressList::ConstIterator fBaseIterator;
	};

	Iterator GetIterator() const { return Iterator(fAddresses); }

private:
	ContainedAddress *_Get(const AddressType &address) const
	{
		typename AddressList::ConstIterator it = fAddresses.GetIterator();
		while (it.HasNext()) {
			ContainedAddress *container = it.Next();
			if (container->address == address)
				return container;
		}
		return NULL;
	}

	AddressList fAddresses;
	int fCount;
};


template<typename Addressing>
class MulticastGroupInterface {
public:
	typedef MulticastGroupInterface<Addressing> ThisType;
	typedef typename Addressing::AddressType AddressType;
	typedef MulticastFilter<Addressing> Filter;
	typedef ::AddressSet<AddressType> AddressSet;

	enum FilterMode {
		kInclude,
		kExclude
	};

	MulticastGroupInterface(Filter *parent, const AddressType &address,
		net_interface *interface);
	~MulticastGroupInterface();

	Filter *Parent() const { return fParent; }

	const AddressType &Address() const { return fMulticastAddress; }
	net_interface *Interface() const { return fInterface; }

	status_t Add();
	status_t Drop();
	status_t BlockSource(const AddressType &sourceAddress);
	status_t UnblockSource(const AddressType &sourceAddress);
	status_t AddSSM(const AddressType &sourceAddress);
	status_t DropSSM(const AddressType &sourceAddress);

	bool IsEmpty() const;
	void Clear();

	FilterMode Mode() const { return fFilterMode; }
	const AddressSet &Sources() const { return fAddresses; }

	bool FilterAccepts(net_buffer *buffer) const;

	struct HashDefinition {
		typedef std::pair<const AddressType *, uint32> KeyType;
		typedef ThisType ValueType;

		size_t HashKey(const KeyType &key) const
			{ return Addressing::HashAddress(*key.first) ^ key.second; }
		size_t Hash(ValueType *value) const
			{ return HashKey(std::make_pair(&value->Address(),
				value->Interface()->index)); }
		bool Compare(const KeyType &key, ValueType *value) const
			{ return value->Interface()->index == key.second
				&& value->Address().s_addr == key.first->s_addr; }
		MulticastGroupInterface*& GetLink(ValueType *value) const
			{ return value->HashLink(); }
	};

	MulticastGroupInterface*& HashLink() { return fLink; }
	MulticastGroupInterface*& MulticastGroupsHashLink() { return fMulticastGroupsLink; }

private:
	// for g++ 2.95
	friend struct HashDefinition;

	Filter *fParent;
	AddressType fMulticastAddress;
	net_interface *fInterface;
	FilterMode fFilterMode;
	AddressSet fAddresses;
	MulticastGroupInterface* fLink;
	MulticastGroupInterface* fMulticastGroupsLink;
};

template<typename Addressing>
class MulticastFilter {
public:
	typedef typename Addressing::AddressType AddressType;
	typedef typename Addressing::ProtocolType ProtocolType;
	typedef MulticastGroupInterface<Addressing> GroupInterface;

	MulticastFilter(ProtocolType *parent);
	~MulticastFilter();

	ProtocolType *Socket() const { return fParent; }

	status_t GetState(const AddressType &groupAddress,
		net_interface *interface, GroupInterface* &state, bool create);
	void ReturnState(GroupInterface *state);

private:
	typedef typename GroupInterface::HashDefinition HashDefinition;
	typedef BOpenHashTable<HashDefinition> States;

	void _ReturnState(GroupInterface *state);

	ProtocolType *fParent;
	States fStates;
};

#endif
