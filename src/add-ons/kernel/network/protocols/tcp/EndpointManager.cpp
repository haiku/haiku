/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Hugo Santos, hugosantos@gmail.com
 */


#include "EndpointManager.h"

#include <new>
#include <unistd.h>

#include <KernelExport.h>

#include <NetUtilities.h>
#include <tracing.h>

#include "TCPEndpoint.h"


//#define TRACE_ENDPOINT_MANAGER
#ifdef TRACE_ENDPOINT_MANAGER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif

#if TCP_TRACING
#	define ENDPOINT_TRACING
#endif
#ifdef ENDPOINT_TRACING
namespace EndpointTracing {

class Bind : public AbstractTraceEntry {
public:
	Bind(TCPEndpoint* endpoint, ConstSocketAddress& address, bool ephemeral)
		:
		fEndpoint(endpoint),
		fEphemeral(ephemeral)
	{
		address.AsString(fAddress, sizeof(fAddress), true);
		Initialized();
	}

	Bind(TCPEndpoint* endpoint, SocketAddress& address, bool ephemeral)
		:
		fEndpoint(endpoint),
		fEphemeral(ephemeral)
	{
		address.AsString(fAddress, sizeof(fAddress), true);
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("tcp:e:%p bind%s address %s", fEndpoint,
			fEphemeral ? " ephemeral" : "", fAddress);
	}

protected:
	TCPEndpoint*	fEndpoint;
	char			fAddress[32];
	bool			fEphemeral;
};

class Connect : public AbstractTraceEntry {
public:
	Connect(TCPEndpoint* endpoint)
		:
		fEndpoint(endpoint)
	{
		endpoint->LocalAddress().AsString(fLocal, sizeof(fLocal), true);
		endpoint->PeerAddress().AsString(fPeer, sizeof(fPeer), true);
		Initialized();
	}

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("tcp:e:%p connect local %s, peer %s", fEndpoint, fLocal,
			fPeer);
	}

protected:
	TCPEndpoint*	fEndpoint;
	char			fLocal[32];
	char			fPeer[32];
};

class Unbind : public AbstractTraceEntry {
public:
	Unbind(TCPEndpoint* endpoint)
		:
		fEndpoint(endpoint)
	{
		//fStackTrace = capture_tracing_stack_trace(10, 0, false);

		endpoint->LocalAddress().AsString(fLocal, sizeof(fLocal), true);
		endpoint->PeerAddress().AsString(fPeer, sizeof(fPeer), true);
		Initialized();
	}

#if 0
	virtual void DumpStackTrace(TraceOutput& out)
	{
		out.PrintStackTrace(fStackTrace);
	}
#endif

	virtual void AddDump(TraceOutput& out)
	{
		out.Print("tcp:e:%p unbind, local %s, peer %s", fEndpoint, fLocal,
			fPeer);
	}

protected:
	TCPEndpoint*	fEndpoint;
	//tracing_stack_trace* fStackTrace;
	char			fLocal[32];
	char			fPeer[32];
};

}	// namespace EndpointTracing

#	define T(x)	new(std::nothrow) EndpointTracing::x
#else
#	define T(x)
#endif	// ENDPOINT_TRACING


static const uint16 kLastReservedPort = 1023;
static const uint16 kFirstEphemeralPort = 40000;


ConnectionHashDefinition::ConnectionHashDefinition(EndpointManager* manager)
	:
	fManager(manager)
{
}


size_t
ConnectionHashDefinition::HashKey(const KeyType& key) const
{
	return ConstSocketAddress(fManager->AddressModule(),
		key.first).HashPair(key.second);
}


size_t
ConnectionHashDefinition::Hash(TCPEndpoint* endpoint) const
{
	return endpoint->LocalAddress().HashPair(*endpoint->PeerAddress());
}


bool
ConnectionHashDefinition::Compare(const KeyType& key,
	TCPEndpoint* endpoint) const
{
	return endpoint->LocalAddress().EqualTo(key.first, true)
		&& endpoint->PeerAddress().EqualTo(key.second, true);
}


TCPEndpoint*&
ConnectionHashDefinition::GetLink(TCPEndpoint* endpoint) const
{
	return endpoint->fConnectionHashLink;
}


//	#pragma mark -


size_t
EndpointHashDefinition::HashKey(uint16 port) const
{
	return port;
}


size_t
EndpointHashDefinition::Hash(TCPEndpoint* endpoint) const
{
	return endpoint->LocalAddress().Port();
}


bool
EndpointHashDefinition::Compare(uint16 port, TCPEndpoint* endpoint) const
{
	return endpoint->LocalAddress().Port() == port;
}


bool
EndpointHashDefinition::CompareValues(TCPEndpoint* first,
	TCPEndpoint* second) const
{
	return first->LocalAddress().Port() == second->LocalAddress().Port();
}


TCPEndpoint*&
EndpointHashDefinition::GetLink(TCPEndpoint* endpoint) const
{
	return endpoint->fEndpointHashLink;
}


//	#pragma mark -


EndpointManager::EndpointManager(net_domain* domain)
	:
	fDomain(domain),
	fConnectionHash(this),
	fLastPort(kFirstEphemeralPort)
{
	rw_lock_init(&fLock, "TCP endpoint manager");
}


EndpointManager::~EndpointManager()
{
	rw_lock_destroy(&fLock);
}


status_t
EndpointManager::Init()
{
	status_t status = fConnectionHash.Init();
	if (status == B_OK)
		status = fEndpointHash.Init();

	return status;
}


//	#pragma mark - connections


/*!	Returns the endpoint matching the connection.
	You must hold the manager's lock when calling this method (either read or
	write).
*/
TCPEndpoint*
EndpointManager::_LookupConnection(const sockaddr* local, const sockaddr* peer)
{
	return fConnectionHash.Lookup(std::make_pair(local, peer));
}


status_t
EndpointManager::SetConnection(TCPEndpoint* endpoint, const sockaddr* _local,
	const sockaddr* peer, const sockaddr* interfaceLocal)
{
	TRACE(("EndpointManager::SetConnection(%p)\n", endpoint));

	WriteLocker _(fLock);

	SocketAddressStorage local(AddressModule());
	local.SetTo(_local);

	if (local.IsEmpty(false)) {
		uint16 port = local.Port();
		local.SetTo(interfaceLocal);
		local.SetPort(port);
	}

	// We want to create a connection for (local, peer), so check to make sure
	// that this pair is not already in use by an existing connection.
	if (_LookupConnection(*local, peer) != NULL)
		return EADDRINUSE;

	endpoint->LocalAddress().SetTo(*local);
	endpoint->PeerAddress().SetTo(peer);
	T(Connect(endpoint));

	// BOpenHashTable doesn't support inserting duplicate objects. Since
	// BOpenHashTable is a chained hash table where the items are required to
	// be intrusive linked list nodes, inserting the same object twice will
	// create a cycle in the linked list, which is not handled currently.
	//
	// We need to makes sure to remove any existing copy of this endpoint
	// object from the table in order to handle calling connect() on a closed
	// socket to connect to a different remote (address, port) than it was
	// originally used for.
	//
	// We use RemoveUnchecked here because we don't want the hash table to
	// resize itself after this removal when we are planning to just add
	// another.
	fConnectionHash.RemoveUnchecked(endpoint);

	fConnectionHash.Insert(endpoint);
	return B_OK;
}


status_t
EndpointManager::SetPassive(TCPEndpoint* endpoint)
{
	WriteLocker _(fLock);

	if (!endpoint->IsBound()) {
		// if the socket is unbound first bind it to ephemeral
		SocketAddressStorage local(AddressModule());
		local.SetToEmpty();

		status_t status = _BindToEphemeral(endpoint, *local);
		if (status < B_OK)
			return status;
	}

	SocketAddressStorage passive(AddressModule());
	passive.SetToEmpty();

	if (_LookupConnection(*endpoint->LocalAddress(), *passive))
		return EADDRINUSE;

	endpoint->PeerAddress().SetTo(*passive);
	fConnectionHash.Insert(endpoint);
	return B_OK;
}


TCPEndpoint*
EndpointManager::FindConnection(sockaddr* local, sockaddr* peer)
{
	ReadLocker _(fLock);

	TCPEndpoint *endpoint = _LookupConnection(local, peer);
	if (endpoint != NULL) {
		TRACE(("TCP: Received packet corresponds to explicit endpoint %p\n",
			endpoint));
		if (gSocketModule->acquire_socket(endpoint->socket))
			return endpoint;
	}

	// no explicit endpoint exists, check for wildcard endpoints

	SocketAddressStorage wildcard(AddressModule());
	wildcard.SetToEmpty();

	endpoint = _LookupConnection(local, *wildcard);
	if (endpoint != NULL) {
		TRACE(("TCP: Received packet corresponds to wildcard endpoint %p\n",
			endpoint));
		if (gSocketModule->acquire_socket(endpoint->socket))
			return endpoint;
	}

	SocketAddressStorage localWildcard(AddressModule());
	localWildcard.SetToEmpty();
	localWildcard.SetPort(AddressModule()->get_port(local));

	endpoint = _LookupConnection(*localWildcard, *wildcard);
	if (endpoint != NULL) {
		TRACE(("TCP: Received packet corresponds to local wildcard endpoint "
			"%p\n", endpoint));
		if (gSocketModule->acquire_socket(endpoint->socket))
			return endpoint;
	}

	// no matching endpoint exists
	TRACE(("TCP: no matching endpoint!\n"));

	return NULL;
}


//	#pragma mark - endpoints


status_t
EndpointManager::Bind(TCPEndpoint* endpoint, const sockaddr* address)
{
	// check the family
	if (!AddressModule()->is_same_family(address))
		return EAFNOSUPPORT;

	WriteLocker locker(fLock);

	if (AddressModule()->get_port(address) == 0)
		return _BindToEphemeral(endpoint, address);

	return _BindToAddress(locker, endpoint, address);
}


status_t
EndpointManager::BindChild(TCPEndpoint* endpoint)
{
	WriteLocker _(fLock);
	return _Bind(endpoint, *endpoint->LocalAddress());
}


/*! You must have fLock write locked when calling this method. */
status_t
EndpointManager::_BindToAddress(WriteLocker& locker, TCPEndpoint* endpoint,
	const sockaddr* _address)
{
	ConstSocketAddress address(AddressModule(), _address);
	uint16 port = address.Port();

	TRACE(("EndpointManager::BindToAddress(%p)\n", endpoint));
	T(Bind(endpoint, address, false));

	// TODO: this check follows very typical UNIX semantics
	// and generally should be improved.
	if (ntohs(port) <= kLastReservedPort && geteuid() != 0)
		return B_PERMISSION_DENIED;

	bool retrying = false;
	int32 retry = 0;
	do {
		EndpointTable::ValueIterator portUsers = fEndpointHash.Lookup(port);
		retry = false;

		while (portUsers.HasNext()) {
			TCPEndpoint* user = portUsers.Next();

			if (user->LocalAddress().IsEmpty(false)
				|| address.EqualTo(*user->LocalAddress(), false)) {
				// Check if this belongs to a local connection

				// Note, while we hold our lock, the endpoint cannot go away,
				// it can only change its state - IsLocal() is safe to be used
				// without having the endpoint locked.
				tcp_state userState = user->State();
				if (user->IsLocal()
					&& (userState > ESTABLISHED || userState == CLOSED)) {
					// This is a closing local connection - wait until it's
					// gone away for real
					locker.Unlock();
					snooze(10000);
					locker.Lock();
						// TODO: make this better
					if (!retrying) {
						retrying = true;
						retry = 5;
					}
					break;
				}

				if ((endpoint->socket->options & SO_REUSEADDR) == 0)
					return EADDRINUSE;

				if (userState != TIME_WAIT && userState != CLOSED)
					return EADDRINUSE;
			}
		}
	} while (retry-- > 0);

	return _Bind(endpoint, *address);
}


/*! You must have fLock write locked when calling this method. */
status_t
EndpointManager::_BindToEphemeral(TCPEndpoint* endpoint,
	const sockaddr* address)
{
	TRACE(("EndpointManager::BindToEphemeral(%p)\n", endpoint));

	uint32 max = fLastPort + 65536;

	for (int32 i = 1; i < 5; i++) {
		// try to retrieve a more or less random port
		uint32 step = i == 4 ? 1 : (system_time() & 0x1f) + 1;
		uint32 counter = fLastPort + step;

		while (counter < max) {
			uint16 port = counter & 0xffff;
			if (port <= kLastReservedPort)
				port += kLastReservedPort;

			fLastPort = port;
			port = htons(port);

			if (!fEndpointHash.Lookup(port).HasNext()) {
				// found a port
				SocketAddressStorage newAddress(AddressModule());
				newAddress.SetTo(address);
				newAddress.SetPort(port);

				TRACE(("   EndpointManager::BindToEphemeral(%p) -> %s\n",
					endpoint, AddressString(Domain(), *newAddress,
					true).Data()));
				T(Bind(endpoint, newAddress, true));

				return _Bind(endpoint, *newAddress);
			}

			counter += step;
		}
	}

	// could not find a port!
	return EADDRINUSE;
}


status_t
EndpointManager::_Bind(TCPEndpoint* endpoint, const sockaddr* address)
{
	// Thus far we have checked if the Bind() is allowed

	status_t status = endpoint->next->module->bind(endpoint->next, address);
	if (status < B_OK)
		return status;

	fEndpointHash.Insert(endpoint);

	return B_OK;
}


status_t
EndpointManager::Unbind(TCPEndpoint* endpoint)
{
	TRACE(("EndpointManager::Unbind(%p)\n", endpoint));
	T(Unbind(endpoint));

	if (endpoint == NULL || !endpoint->IsBound()) {
		TRACE(("  endpoint is unbound.\n"));
		return B_BAD_VALUE;
	}

	WriteLocker _(fLock);

	if (!fEndpointHash.Remove(endpoint))
		panic("bound endpoint %p not in hash!", endpoint);

	fConnectionHash.Remove(endpoint);

	(*endpoint->LocalAddress())->sa_len = 0;

	return B_OK;
}


status_t
EndpointManager::ReplyWithReset(tcp_segment_header& segment, net_buffer* buffer)
{
	TRACE(("TCP: Sending RST...\n"));

	net_buffer* reply = gBufferModule->create(512);
	if (reply == NULL)
		return B_NO_MEMORY;

	AddressModule()->set_to(reply->source, buffer->destination);
	AddressModule()->set_to(reply->destination, buffer->source);

	tcp_segment_header outSegment(TCP_FLAG_RESET);
	outSegment.sequence = 0;
	outSegment.acknowledge = 0;
	outSegment.advertised_window = 0;
	outSegment.urgent_offset = 0;

	if ((segment.flags & TCP_FLAG_ACKNOWLEDGE) == 0) {
		outSegment.flags |= TCP_FLAG_ACKNOWLEDGE;
		outSegment.acknowledge = segment.sequence + buffer->size;
		// TODO: Confirm:
		if ((segment.flags & (TCP_FLAG_SYNCHRONIZE | TCP_FLAG_FINISH)) != 0)
			outSegment.acknowledge++;
	} else
		outSegment.sequence = segment.acknowledge;

	status_t status = add_tcp_header(AddressModule(), outSegment, reply);
	if (status == B_OK)
		status = Domain()->module->send_data(NULL, reply);

	if (status != B_OK)
		gBufferModule->free(reply);

	return status;
}


void
EndpointManager::Dump() const
{
	kprintf("-------- TCP Domain %p ---------\n", this);
	kprintf("%10s %21s %21s %8s %8s %12s\n", "address", "local", "peer",
		"recv-q", "send-q", "state");

	ConnectionTable::Iterator iterator = fConnectionHash.GetIterator();

	while (iterator.HasNext()) {
		TCPEndpoint *endpoint = iterator.Next();

		char localBuf[64], peerBuf[64];
		endpoint->LocalAddress().AsString(localBuf, sizeof(localBuf), true);
		endpoint->PeerAddress().AsString(peerBuf, sizeof(peerBuf), true);

		kprintf("%p %21s %21s %8lu %8lu %12s\n", endpoint, localBuf, peerBuf,
			endpoint->fReceiveQueue.Available(), endpoint->fSendQueue.Used(),
			name_for_state(endpoint->State()));
	}
}

