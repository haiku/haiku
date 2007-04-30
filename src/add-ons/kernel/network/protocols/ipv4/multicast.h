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

#include <netinet/in.h>

struct net_buffer;
struct net_interface;
struct net_protocol;

// This code is template'ized as it is reusable for IPv6

template<typename Addressing> class MulticastFilter;
template<typename Addressing> class MulticastGroupState;

// TODO move this elsewhere...
struct IPv4Multicast {
	typedef struct in_addr AddressType;
	typedef struct ipv4_protocol ProtocolType;
	typedef MulticastGroupState<IPv4Multicast> GroupState;

	static status_t JoinGroup(GroupState *);
	static status_t LeaveGroup(GroupState *);

	static const in_addr &AddressFromSockAddr(const sockaddr *sockaddr)
		{ return ((const sockaddr_in *)sockaddr)->sin_addr; }
	static size_t HashAddress(const in_addr &address)
		{ return address.s_addr; }
};

template<typename AddressType>
struct MulticastSource
	: DoublyLinkedListLinkImpl< MulticastSource<AddressType> > {
	AddressType address;
};

template<typename AddressType>
class MulticastGroupInterfaceState
	: public DoublyLinkedListLinkImpl< MulticastGroupInterfaceState<AddressType> > {
public:
	MulticastGroupInterfaceState(net_interface *interface);
	~MulticastGroupInterfaceState();

	net_interface *Interface() const { return fInterface; }

	status_t Add(const AddressType &address);
	status_t Remove(const AddressType &address);

	bool Contains(const AddressType &address)
		{ return _Get(address, false) != NULL; }

private:
	typedef MulticastSource<AddressType> Source;
	typedef DoublyLinkedList<Source> SourceList;

	Source *_Get(const AddressType &address, bool create);
	void _Remove(Source *state);

	net_interface *fInterface;
	// TODO make this an hash table as well
	SourceList fSources;
};

template<typename Addressing>
class MulticastGroupState
	: public DoublyLinkedListLinkImpl< MulticastGroupState<Addressing> > {
public:
	typedef MulticastGroupState<Addressing> ThisType;
	typedef HashTableLink<ThisType> HashLink;
	typedef typename Addressing::AddressType AddressType;
	typedef typename Addressing::ProtocolType ProtocolType;

	MulticastGroupState(ProtocolType *parent, const AddressType &address);
	~MulticastGroupState();

	ProtocolType *Socket() const { return fSocket; }

	const AddressType &Address() const { return fMulticastAddress; }
	bool IsEmpty() const
		{ return fFilterMode == kInclude && fInterfaces.IsEmpty(); }

	status_t Add(net_interface *interface);
	status_t Drop(net_interface *interface);
	status_t BlockSource(net_interface *interface,
		const AddressType &sourceAddress);
	status_t UnblockSource(net_interface *interface,
		const AddressType &sourceAddress);
	status_t AddSSM(net_interface *interface,
		const AddressType &sourceAddress);
	status_t DropSSM(net_interface *interface,
		const AddressType &sourceAddress);

	void Clear();

	bool FilterAccepts(net_buffer *buffer);

	struct HashDefinition {
		typedef void ParentType;
		typedef typename MulticastGroupState::AddressType KeyType;
		typedef typename MulticastGroupState::ThisType ValueType;
		typedef typename MulticastGroupState::HashLink HashLink;

		size_t HashKey(const KeyType &key) const
			{ return Addressing::HashAddress(key); }
		size_t Hash(ValueType *value) const
			{ return HashKey(value->Address()); }
		bool Compare(const KeyType &key, ValueType *value) const
			{ return key == value->Address(); }
		HashLink *GetLink(ValueType *value) const { return &value->fHashLink; }
	};

private:
	// for g++ 2.95
	friend class HashDefinition;

	typedef MulticastGroupInterfaceState<AddressType> InterfaceState;
	typedef DoublyLinkedList<InterfaceState> InterfaceList;

	InterfaceState *_GetInterface(net_interface *interface, bool create);
	void _RemoveInterface(InterfaceState *state);

	enum FilterMode {
		kInclude,
		kExclude
	};

	ProtocolType *fSocket;
	AddressType fMulticastAddress;
	FilterMode fFilterMode;
	InterfaceList fInterfaces;
	HashLink fHashLink;
};

template<typename Addressing>
class MulticastFilter {
public:
	typedef typename Addressing::AddressType AddressType;
	typedef typename Addressing::ProtocolType ProtocolType;
	typedef MulticastGroupState<Addressing> GroupState;

	MulticastFilter(ProtocolType *parent);
	~MulticastFilter();

	ProtocolType *Parent() const { return fParent; }

	GroupState *GetGroup(const AddressType &groupAddress, bool create);
	void ReturnGroup(GroupState *group);

private:
	typedef typename GroupState::HashDefinition GroupHashDefinition;
	typedef OpenHashTable<GroupHashDefinition> States;

	ProtocolType *fParent;
	States fStates;
};

#endif
