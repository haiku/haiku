/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#include "multicast.h"

#include <net_buffer.h>

#include <netinet/in.h>

#include <new>

template class MulticastFilter<IPv4Multicast>;
template class MulticastGroupState<IPv4Multicast>;
template class MulticastGroupInterfaceState<in_addr>;

static inline bool
operator==(const in_addr &a1, const in_addr &a2)
{
	return a1.s_addr == a2.s_addr;
}

using std::nothrow;


template<typename AddressType>
MulticastGroupInterfaceState<AddressType>::MulticastGroupInterfaceState(
	net_interface *interface)
	: fInterface(interface)
{
}


template<typename AddressType>
MulticastGroupInterfaceState<AddressType>::~MulticastGroupInterfaceState()
{
	SourceList::Iterator iterator = fSources.GetIterator();
	while (iterator.HasNext()) {
		_Remove(iterator.Next());
	}
}


template<typename AddressType> status_t
MulticastGroupInterfaceState<AddressType>::Add(const AddressType &address)
{
	return _Get(address, true) ? B_OK : ENOBUFS;
}


template<typename AddressType> status_t
MulticastGroupInterfaceState<AddressType>::Remove(const AddressType &address)
{
	Source *state = _Get(address, false);
	if (state == NULL)
		return EADDRNOTAVAIL;

	_Remove(state);
	return B_OK;
}


template<typename AddressType> MulticastGroupInterfaceState<AddressType>::Source *
MulticastGroupInterfaceState<AddressType>::_Get(const AddressType &address,
	bool create)
{
	SourceList::Iterator iterator = fSources.GetIterator();
	while (iterator.HasNext()) {
		Source *state = iterator.Next();
		if (state->address == address)
			return state;
	}

	if (!create)
		return false;

	Source *state = new (nothrow) Source;
	if (state) {
		state->address = address;
		fSources.Add(state);
	}

	return state;
}


template<typename AddressType> void
MulticastGroupInterfaceState<AddressType>::_Remove(Source *state)
{
	fSources.Remove(state);
	delete state;
}


template<typename Addressing>
MulticastGroupState<Addressing>::MulticastGroupState(net_protocol *socket,
	const AddressType &address)
	: fSocket(socket), fMulticastAddress(address), fFilterMode(kInclude)
{
}


template<typename Addressing>
MulticastGroupState<Addressing>::~MulticastGroupState()
{
	Clear();
}


template<typename Addressing> status_t
MulticastGroupState<Addressing>::Add(net_interface *interface)
{
	if (fFilterMode == kInclude && !fInterfaces.IsEmpty())
		return EINVAL;

	fFilterMode = kExclude;

	return _GetInterface(interface, true) != NULL ? B_OK : ENOBUFS;
}


template<typename Addressing> status_t
MulticastGroupState<Addressing>::Drop(net_interface *interface)
{
	InterfaceState *state = _GetInterface(interface, false);
	if (state == NULL)
		return EADDRNOTAVAIL;

	_RemoveInterface(state);

	if (fInterfaces.IsEmpty())
		fFilterMode = kInclude;

	return B_OK;
}


template<typename Addressing> status_t
MulticastGroupState<Addressing>::BlockSource(net_interface *interface,
	const AddressType &sourceAddress)
{
	if (fFilterMode != kExclude)
		return EINVAL;

	InterfaceState *state = _GetInterface(interface, false);
	if (state == NULL)
		return EINVAL;

	return state->Add(sourceAddress);
}


template<typename Addressing> status_t
MulticastGroupState<Addressing>::UnblockSource(net_interface *interface,
	const AddressType &sourceAddress)
{
	if (fFilterMode != kExclude)
		return EINVAL;

	InterfaceState *state = _GetInterface(interface, false);
	if (state == NULL)
		return EINVAL;

	return state->Remove(sourceAddress);
}

template<typename Addressing> status_t
MulticastGroupState<Addressing>::AddSSM(net_interface *interface,
	const AddressType &sourceAddress)
{
	if (fFilterMode == kExclude)
		return EINVAL;

	InterfaceState *state = _GetInterface(interface, true);
	if (state == NULL)
		return ENOBUFS;

	return state->Add(sourceAddress);
}


template<typename Addressing> status_t
MulticastGroupState<Addressing>::DropSSM(net_interface *interface,
	const AddressType &sourceAddress)
{
	if (fFilterMode == kExclude)
		return EINVAL;

	InterfaceState *state = _GetInterface(interface, false);
	if (state == NULL)
		return EADDRNOTAVAIL;

	return state->Remove(sourceAddress);
}


template<typename Addressing> void
MulticastGroupState<Addressing>::Clear()
{
	InterfaceList::Iterator iterator = fInterfaces.GetIterator();
	while (iterator.HasNext())
		_RemoveInterface(iterator.Next());
}


template<typename Addressing> bool
MulticastGroupState<Addressing>::FilterAccepts(net_buffer *buffer)
{
	InterfaceState *state = _GetInterface(buffer->interface, false);
	if (state == NULL)
		return false;

	bool has = state->Contains(*Addressing::AddressFromSockAddr(
		(sockaddr *)&buffer->source));

	return (has && fFilterMode == kInclude) || (!has && fFilterMode == kExclude);
}


template<typename Addressing> MulticastGroupState<Addressing>::InterfaceState *
MulticastGroupState<Addressing>::_GetInterface(net_interface *interface,
	bool create)
{
	InterfaceList::Iterator iterator = fInterfaces.GetIterator();
	while (iterator.HasNext()) {
		InterfaceState *state = iterator.Next();
		if (state->Interface() == interface)
			return state;
	}

	if (!create)
		return false;

	InterfaceState *state = new (nothrow) InterfaceState(interface);
	if (state)
		fInterfaces.Add(state);

	return state;
}


template<typename Addressing> void
MulticastGroupState<Addressing>::_RemoveInterface(InterfaceState *state)
{
	fInterfaces.Remove(state);
	delete state;
}


template<typename Addressing>
MulticastFilter<Addressing>::MulticastFilter(net_protocol *socket)
	: fParent(socket)
{
}


template<typename Addressing>
MulticastFilter<Addressing>::~MulticastFilter()
{
	States::Iterator iterator = fStates.GetIterator();
	while (iterator.HasNext()) {
		GroupState *state = iterator.Next();
		state->Clear();
		ReturnGroup(state);
	}
}


template<typename Addressing> MulticastFilter<Addressing>::GroupState *
MulticastFilter<Addressing>::GetGroup(const AddressType &groupAddress,
	bool create)
{
	States::Iterator iterator = fStates.GetIterator();

	while (iterator.HasNext()) {
		GroupState *state = iterator.Next();
		if (state->Address() == groupAddress)
			return state;
	}

	if (!create)
		return NULL;

	GroupState *state = new (nothrow) GroupState(fParent, groupAddress);
	if (state) {
		if (Addressing::JoinGroup(groupAddress, state->ProtocolLink()) < B_OK) {
			delete state;
			return NULL;
		}

		fStates.Add(state);
	}

	return state;
}


template<typename Addressing> void
MulticastFilter<Addressing>::ReturnGroup(GroupState *group)
{
	if (group->IsEmpty()) {
		Addressing::LeaveGroup(group->Address(), group->ProtocolLink());

		fStates.Remove(group);
		delete group;
	}
}

