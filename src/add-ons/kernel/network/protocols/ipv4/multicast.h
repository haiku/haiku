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

struct net_interface;
struct net_socket;

template<typename AddressType> class MulticastFilter;

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

template<typename AddressType>
class MulticastGroupState {
public:
	MulticastGroupState(MulticastFilter<AddressType> *parent,
		const AddressType &address);
	~MulticastGroupState();

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

	MulticastFilter<AddressType> *fParent;
	AddressType fMulticastAddress;
	FilterMode fFilterMode;
	InterfaceList fInterfaces;
};

template<typename AddressType>
class MulticastFilter {
public:
	typedef MulticastGroupState<AddressType> GroupState;

	MulticastFilter(net_socket *parent);
	~MulticastFilter();

	net_socket *Parent() const { return fParent; }

	GroupState *GetGroup(const AddressType &groupAddress, bool create);
	void ReturnGroup(GroupState *group);

private:
	typedef DoublyLinkedListCLink<GroupState> GroupStateLink;
	typedef DoublyLinkedList<GroupState, GroupStateLink> States;

	net_socket *fParent;

	// TODO change this into an hash table or tree
	States fStates;
};

#endif
