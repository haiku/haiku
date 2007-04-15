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
	MulticastGroupState(const AddressType &address);
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

	AddressType fMulticastAddress;
	FilterMode fFilterMode;
	InterfaceList fInterfaces;
};

template<typename AddressType>
class MulticastFilter {
public:
	typedef MulticastGroupState<AddressType> GroupState;

	~MulticastFilter();

	GroupState *GetGroup(const AddressType &groupAddress, bool create);
	void ReturnGroup(GroupState *group);

private:
	typedef DoublyLinkedListCLink<GroupState> GroupStateLink;
	typedef DoublyLinkedList<GroupState, GroupStateLink> States;

	// TODO change this into an hash table or tree
	States fStates;
};

#endif
