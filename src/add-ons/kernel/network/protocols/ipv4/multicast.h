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
#include <util/list.h>

#include <netinet/in.h>

struct net_buffer;
struct net_interface;
struct net_protocol;

// This code is template'ized as it is reusable for IPv6

template<typename Addressing> class MulticastFilter;
template<typename Addressing> struct MulticastGroupLink;
template<typename Addressing> class MulticastGroupState;

// TODO move this elsewhere...
struct IPv4Multicast {
	typedef struct in_addr AddressType;

	static status_t JoinGroup(const in_addr &, MulticastGroupLink<IPv4Multicast> *);
	static status_t LeaveGroup(const in_addr &, MulticastGroupLink<IPv4Multicast> *);

	static in_addr *AddressFromSockAddr(sockaddr *sockaddr)
	{
		return &((sockaddr_in *)sockaddr)->sin_addr;
	}
};

template<typename AddressType>
struct MulticastSource {
	AddressType address;
	list_link link;
};

template<typename AddressType>
class MulticastGroupInterfaceState {
public:
	MulticastGroupInterfaceState(net_interface *interface);
	~MulticastGroupInterfaceState();

	net_interface *Interface() const { return fInterface; }

	status_t Add(const AddressType &address);
	status_t Remove(const AddressType &address);

	bool Contains(const AddressType &address)
		{ return _Get(address, false) != NULL; }

	list_link link;
private:
	typedef MulticastSource<AddressType> Source;
	typedef DoublyLinkedListCLink<Source> SourceLink;
	typedef DoublyLinkedList<Source, SourceLink> SourceList;

	Source *_Get(const AddressType &address, bool create);
	void _Remove(Source *state);

	net_interface *fInterface;
	// TODO make this an hash table as well
	SourceList fSources;
};

template<typename Addressing>
struct MulticastGroupLink {
	MulticastGroupState<Addressing> *group;
	list_link link;
};

template<typename Addressing>
class MulticastGroupState {
public:
	typedef typename Addressing::AddressType AddressType;

	MulticastGroupState(net_protocol *parent, const AddressType &address);
	~MulticastGroupState();

	net_protocol *Socket() const { return fSocket; }

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

	MulticastGroupLink<Addressing> *ProtocolLink() { return &fInternalLink; }

	list_link link;
private:
	typedef MulticastGroupInterfaceState<AddressType> InterfaceState;
	typedef DoublyLinkedListCLink<InterfaceState> InterfaceStateLink;
	typedef DoublyLinkedList<InterfaceState, InterfaceStateLink> InterfaceList;

	InterfaceState *_GetInterface(net_interface *interface, bool create);
	void _RemoveInterface(InterfaceState *state);

	enum FilterMode {
		kInclude,
		kExclude
	};

	net_protocol *fSocket;
	AddressType fMulticastAddress;
	FilterMode fFilterMode;
	InterfaceList fInterfaces;
	MulticastGroupLink<Addressing> fInternalLink;
};

template<typename Addressing>
class MulticastFilter {
public:
	typedef typename Addressing::AddressType AddressType;
	typedef MulticastGroupState<Addressing> GroupState;

	MulticastFilter(net_protocol *parent);
	~MulticastFilter();

	net_protocol *Parent() const { return fParent; }

	GroupState *GetGroup(const AddressType &groupAddress, bool create);
	void ReturnGroup(GroupState *group);

private:
	typedef DoublyLinkedListCLink<GroupState> GroupStateLink;
	typedef DoublyLinkedList<GroupState, GroupStateLink> States;

	net_protocol *fParent;

	// TODO change this into an hash table or tree
	States fStates;
};

#endif
