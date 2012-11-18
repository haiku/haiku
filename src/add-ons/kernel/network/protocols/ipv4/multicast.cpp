/*
 * Copyright 2007-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Hugo Santos, hugosantos@gmail.com
 */

#include "multicast.h"

#include <net_buffer.h>

#include <netinet/in.h>

#include <new>


using std::nothrow;


static inline bool
operator==(const in_addr& a, const in_addr& b)
{
	return a.s_addr == b.s_addr;
}


// #pragma mark -


template<typename Addressing>
MulticastGroupInterface<Addressing>::MulticastGroupInterface(Filter *parent,
	const AddressType &address, net_interface *interface)
	:
	fParent(parent),
	fMulticastAddress(address),
	fInterface(interface)
{
}


template<typename Addressing>
MulticastGroupInterface<Addressing>::~MulticastGroupInterface()
{
	Clear();
}


template<typename Addressing> status_t
MulticastGroupInterface<Addressing>::Add()
{
	if (fFilterMode == kInclude && !fAddresses.IsEmpty())
		return EINVAL;

	fFilterMode = kExclude;
	return B_OK;
}


template<typename Addressing> status_t
MulticastGroupInterface<Addressing>::Drop()
{
	fAddresses.Clear();
	Addressing::LeaveGroup(this);
	fFilterMode = kInclude;
	return B_OK;
}


template<typename Addressing> status_t
MulticastGroupInterface<Addressing>::BlockSource(
	const AddressType &sourceAddress)
{
	if (fFilterMode != kExclude)
		return EINVAL;

	fAddresses.Add(sourceAddress);
	return B_OK;
}


template<typename Addressing> status_t
MulticastGroupInterface<Addressing>::UnblockSource(
	const AddressType &sourceAddress)
{
	if (fFilterMode != kExclude)
		return EINVAL;

	if (!fAddresses.Has(sourceAddress))
		return EADDRNOTAVAIL;

	fAddresses.Add(sourceAddress);
	return B_OK;
}


template<typename Addressing> status_t
MulticastGroupInterface<Addressing>::AddSSM(const AddressType &sourceAddress)
{
	if (fFilterMode == kExclude)
		return EINVAL;

	fAddresses.Add(sourceAddress);
	return B_OK;
}


template<typename Addressing> status_t
MulticastGroupInterface<Addressing>::DropSSM(const AddressType &sourceAddress)
{
	if (fFilterMode == kExclude)
		return EINVAL;

	if (!fAddresses.Has(sourceAddress))
		return EADDRNOTAVAIL;

	fAddresses.Add(sourceAddress);
	return B_OK;
}


template<typename Addressing> bool
MulticastGroupInterface<Addressing>::IsEmpty() const
{
	return fFilterMode == kInclude && fAddresses.IsEmpty();
}


template<typename Addressing> void
MulticastGroupInterface<Addressing>::Clear()
{
	if (IsEmpty())
		return;

	fFilterMode = kInclude;
	fAddresses.Clear();
	Addressing::LeaveGroup(this);
}


template<typename Addressing> bool
MulticastGroupInterface<Addressing>::FilterAccepts(net_buffer *buffer) const
{
	bool has = fAddresses.Has(Addressing::AddressFromSockAddr(
		buffer->source));

	return (has && fFilterMode == kInclude)
		|| (!has && fFilterMode == kExclude);
}


template<typename Addressing>
MulticastFilter<Addressing>::MulticastFilter(ProtocolType *socket)
	: fParent(socket), fStates()
{
}


template<typename Addressing>
MulticastFilter<Addressing>::~MulticastFilter()
{
	while (true) {
		typename States::Iterator iterator = fStates.GetIterator();
		if (!iterator.HasNext())
			return;

		GroupInterface *state = iterator.Next();
		state->Clear();
		_ReturnState(state);
	}
}


template<typename Addressing> status_t
MulticastFilter<Addressing>::GetState(const AddressType &groupAddress,
	net_interface *interface, GroupInterface* &state, bool create)
{
	state = fStates.Lookup(std::make_pair(&groupAddress, interface->index));

	if (state == NULL && create) {
		state = new (nothrow) GroupInterface(this, groupAddress, interface);
		if (state == NULL)
			return B_NO_MEMORY;

		status_t status = fStates.Insert(state);
		if (status < B_OK) {
			delete state;
			return status;
		}

		status = Addressing::JoinGroup(state);
		if (status < B_OK) {
			fStates.Remove(state);
			delete state;
			return status;
		}

	}

	return B_OK;
}


template<typename Addressing> void
MulticastFilter<Addressing>::ReturnState(GroupInterface *state)
{
	if (state->IsEmpty())
		_ReturnState(state);
}


template<typename Addressing> void
MulticastFilter<Addressing>::_ReturnState(GroupInterface *state)
{
	fStates.Remove(state);
	delete state;
}

// IPv4 explicit template instantiation
template class MulticastFilter<IPv4Multicast>;
template class MulticastGroupInterface<IPv4Multicast>;
