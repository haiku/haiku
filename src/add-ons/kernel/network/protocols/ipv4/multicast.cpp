/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#include "multicast.h"

#include <netinet/in.h>

#include <new>

template class MulticastFilter<in_addr>;
template class MulticastGroupState<in_addr>;
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


template<typename AddressType>
MulticastGroupState<AddressType>::MulticastGroupState(
	MulticastFilter<AddressType> *parent, const AddressType &address)
	: fParent(parent), fMulticastAddress(address), fFilterMode(kInclude)
{
}


template<typename AddressType>
MulticastGroupState<AddressType>::~MulticastGroupState()
{
	InterfaceList::Iterator iterator = fInterfaces.GetIterator();
	while (iterator.HasNext())
		_RemoveInterface(iterator.Next());
}


template<typename AddressType> status_t
MulticastGroupState<AddressType>::Add(net_interface *interface)
{
	if (fFilterMode == kInclude && !fInterfaces.IsEmpty())
		return EINVAL;

	fFilterMode = kExclude;

	return _GetInterface(interface, true) != NULL ? B_OK : ENOBUFS;
}


template<typename AddressType> status_t
MulticastGroupState<AddressType>::Drop(net_interface *interface)
{
	InterfaceState *state = _GetInterface(interface, false);
	if (state == NULL)
		return EADDRNOTAVAIL;

	_RemoveInterface(state);

	if (fInterfaces.IsEmpty())
		fFilterMode = kInclude;

	return B_OK;
}


template<typename AddressType> status_t
MulticastGroupState<AddressType>::BlockSource(net_interface *interface,
	const AddressType &sourceAddress)
{
	if (fFilterMode != kExclude)
		return EINVAL;

	InterfaceState *state = _GetInterface(interface, false);
	if (state == NULL)
		return EINVAL;

	return state->Add(sourceAddress);
}


template<typename AddressType> status_t
MulticastGroupState<AddressType>::UnblockSource(net_interface *interface,
	const AddressType &sourceAddress)
{
	if (fFilterMode != kExclude)
		return EINVAL;

	InterfaceState *state = _GetInterface(interface, false);
	if (state == NULL)
		return EINVAL;

	return state->Remove(sourceAddress);
}

template<typename AddressType> status_t
MulticastGroupState<AddressType>::AddSSM(net_interface *interface,
	const AddressType &sourceAddress)
{
	if (fFilterMode == kExclude)
		return EINVAL;

	InterfaceState *state = _GetInterface(interface, true);
	if (state == NULL)
		return ENOBUFS;

	return state->Add(sourceAddress);
}


template<typename AddressType> status_t
MulticastGroupState<AddressType>::DropSSM(net_interface *interface,
	const AddressType &sourceAddress)
{
	if (fFilterMode == kExclude)
		return EINVAL;

	InterfaceState *state = _GetInterface(interface, false);
	if (state == NULL)
		return EADDRNOTAVAIL;

	return state->Remove(sourceAddress);
}


template<typename AddressType> MulticastGroupState<AddressType>::InterfaceState *
MulticastGroupState<AddressType>::_GetInterface(net_interface *interface,
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


template<typename AddressType> void
MulticastGroupState<AddressType>::_RemoveInterface(InterfaceState *state)
{
	fInterfaces.Remove(state);
	delete state;
}


template<typename AddressType>
MulticastFilter<AddressType>::MulticastFilter(net_socket *socket)
	: fParent(socket)
{
}


template<typename AddressType>
MulticastFilter<AddressType>::~MulticastFilter()
{
	States::Iterator iterator = fStates.GetIterator();
	while (iterator.HasNext()) {
		GroupState *state = iterator.Next();
		fStates.Remove(state);
		delete state;
	}
}


template<typename AddressType> MulticastFilter<AddressType>::GroupState *
MulticastFilter<AddressType>::GetGroup(const AddressType &groupAddress,
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

	GroupState *state = new (nothrow) GroupState(this, groupAddress);
	if (state)
		fStates.Add(state);
	return state;
}


template<typename AddressType> void
MulticastFilter<AddressType>::ReturnGroup(GroupState *group)
{
	if (group->IsEmpty()) {
		fStates.Remove(group);
		delete group;
	}
}

