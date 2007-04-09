/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "EndpointManager.h"
#include "TCPEndpoint.h"

#include <NetUtilities.h>

#include <util/AutoLock.h>

#include <KernelExport.h>


//#define TRACE_ENDPOINT_MANAGER
#ifdef TRACE_ENDPOINT_MANAGER
#	define TRACE(x) dprintf x
#else
#	define TRACE(x)
#endif


struct connection_key {
	const sockaddr	*local;
	const sockaddr	*peer;
};

struct endpoint_key {
	uint16	port;
};


static const uint32 kConnectionHashBuckets = 256;
static const uint32 kEndpointHashBuckets = 256;

static const uint16 kLastReservedPort = 1023;
static const uint16 kFirstEphemeralPort = 40000;


EndpointManager::EndpointManager()
{
	fConnectionHash = hash_init(kConnectionHashBuckets,
		offsetof(TCPEndpoint, fConnectionHashNext),
		&_ConnectionCompare, &_ConnectionHash);
	fEndpointHash = hash_init(kEndpointHashBuckets,
		offsetof(TCPEndpoint, fEndpointHashNext),
		&_EndpointCompare, &_EndpointHash);

	recursive_lock_init(&fLock, "endpoint manager");
}


EndpointManager::~EndpointManager()
{
	hash_uninit(fConnectionHash);
	hash_uninit(fEndpointHash);

	recursive_lock_destroy(&fLock);
}


status_t
EndpointManager::InitCheck() const
{
	if (fConnectionHash == NULL
		|| fEndpointHash == NULL)
		return B_NO_MEMORY;

	if (fLock.sem < B_OK)
		return fLock.sem;

	return B_OK;
}


//	#pragma mark - connections


/*!
	Returns the endpoint matching the connection.
	You must hold the manager's lock when calling this method.
*/
TCPEndpoint *
EndpointManager::_LookupConnection(sockaddr *local, sockaddr *peer)
{
	connection_key key;
	key.local = local;
	key.peer = peer;

	return (TCPEndpoint *)hash_lookup(fConnectionHash, &key);
}


status_t
EndpointManager::_RemoveConnection(TCPEndpoint *endpoint)
{
	RecursiveLocker locker(&fLock);
	return hash_remove(fConnectionHash, endpoint);
}


void
EndpointManager::_DumpConnections()
{
	RecursiveLocker lock(&fLock);

	if (gDomain == NULL)
		return;

	struct hash_iterator iterator;
	hash_open(fConnectionHash, &iterator);

	TRACE(("Active TCP Connections:\n"));

	TCPEndpoint *endpoint;
	while ((endpoint = (TCPEndpoint *)hash_next(fConnectionHash, &iterator)) != NULL) {
		TRACE(("  TCPEndpoint %p: local %s, peer %s\n", endpoint,
			AddressString(gDomain, (sockaddr *)&endpoint->socket->address, true).Data(),
			AddressString(gDomain, (sockaddr *)&endpoint->socket->peer, true).Data()));
	}

	hash_close(fConnectionHash, &iterator, false);
}


status_t
EndpointManager::SetConnection(TCPEndpoint *endpoint,
	const sockaddr *local, const sockaddr *peer, const sockaddr *interfaceLocal)
{
	TRACE(("EndpointManager::SetConnection(%p)\n", endpoint));

	RecursiveLocker locker(&fLock);
	sockaddr localBuffer;

	// need to associate this connection with a real address, not INADDR_ANY
	if (gAddressModule->is_empty_address(local, false)) {
		gAddressModule->set_to(&localBuffer, interfaceLocal);
		gAddressModule->set_port(&localBuffer, gAddressModule->get_port(local));
		local = &localBuffer;
	}

	connection_key key;
	key.local = local;
	key.peer = peer;

	if (hash_lookup(fConnectionHash, &key) != NULL)
		return EADDRINUSE;

	_RemoveConnection(endpoint);

	gAddressModule->set_to((sockaddr *)&endpoint->socket->address, local);
	gAddressModule->set_to((sockaddr *)&endpoint->socket->peer, peer);

	return hash_insert(fConnectionHash, endpoint);
}


TCPEndpoint *
EndpointManager::FindConnection(sockaddr *local, sockaddr *peer)
{
	TCPEndpoint *endpoint = _LookupConnection(local, peer);
	if (endpoint != NULL) {
		TRACE(("TCP: Received packet corresponds to explicit endpoint %p\n", endpoint));
		return endpoint;
	}

	// no explicit endpoint exists, check for wildcard endpoints

	sockaddr wildcard;
	gAddressModule->set_to_empty_address(&wildcard);

	endpoint = _LookupConnection(local, &wildcard);
	if (endpoint != NULL) {
		TRACE(("TCP: Received packet corresponds to wildcard endpoint %p\n", endpoint));
		return endpoint;
	}

	sockaddr localWildcard;
	gAddressModule->set_to_empty_address(&localWildcard);
	gAddressModule->set_port(&localWildcard, gAddressModule->get_port(local));

	endpoint = _LookupConnection(&localWildcard, &wildcard);
	if (endpoint != NULL) {
		TRACE(("TCP: Received packet corresponds to local wildcard endpoint %p\n", endpoint));
		return endpoint;
	}

	// no matching endpoint exists	
	TRACE(("TCP: no matching endpoint!\n"));
	_DumpConnections();

	return NULL;
}


//	#pragma mark - endpoints


TCPEndpoint *
EndpointManager::_LookupEndpoint(uint16 port)
{
	endpoint_key key;
	key.port = port;

	return (TCPEndpoint *)hash_lookup(fEndpointHash, &key);
}


status_t
EndpointManager::Bind(TCPEndpoint *endpoint, sockaddr *address)
{
	TRACE(("EndpointManager::Bind(%p, %s)\n", endpoint, AddressString(gDomain, address, true).Data()));

	if (gAddressModule->is_empty_address(address, true))
		return B_BAD_VALUE;

	uint16 port = gAddressModule->get_port(address);

	// TODO: check the root group instead?
	if (ntohs(port) <= kLastReservedPort && geteuid() != 0)
		return B_PERMISSION_DENIED;

	RecursiveLocker locker(&fLock);

	TCPEndpoint *first = _LookupEndpoint(port);

	// If there is already an endpoint bound to that port, SO_REUSEADDR has to be
	// specified by the new endpoint to be allowed to bind to that same port.
	// Alternatively, all endpoints must have the SO_REUSEPORT option set.
	if (first != NULL
		&& (endpoint->socket->options & SO_REUSEADDR) == 0
		&& ((endpoint->socket->options & SO_REUSEPORT) == 0
			|| (first->socket->options & SO_REUSEPORT) == 0))
		return EADDRINUSE;

	if (first != NULL) {
		TCPEndpoint *last = first;
		while (true) {
			// check if this endpoint binds to a wildcard address
			if (gAddressModule->is_empty_address((sockaddr *)&last->socket->address, false)) {
				// you cannot specialize a wildcard endpoint - you have to open the
				// wildcard endpoint last
				return B_PERMISSION_DENIED;
			}

			if (last->fEndpointNextWithSamePort == NULL)
				break;

			last = last->fEndpointNextWithSamePort;
		}

		// "first" stays the first item in the list
		last->fEndpointNextWithSamePort = endpoint;
	} else
		hash_insert(fEndpointHash, endpoint);

	gAddressModule->set_to((sockaddr *)&endpoint->socket->address, address);

	endpoint->fEndpointNextWithSamePort = NULL;
	hash_insert(fConnectionHash, endpoint);

	return B_OK;
}


status_t
EndpointManager::BindToEphemeral(TCPEndpoint *endpoint, sockaddr *address)
{
	TRACE(("EndpointManager::BindToEphemeral(%p)\n", endpoint));

	RecursiveLocker locker(&fLock);

	uint32 max = kFirstEphemeralPort + 65536;

	for (int32 i = 1; i < 5; i++) {
		// try to retrieve a more or less random port
		uint32 counter = kFirstEphemeralPort;
		uint32 step = i == 4 ? 1 : (system_time() & 0x1f) + 1;

		while (counter < max) {
			uint16 port = counter & 0xffff;
			if (port <= kLastReservedPort)
				port += kLastReservedPort;

			port = htons(port);

			TCPEndpoint *other = _LookupEndpoint(port);
			if (other == NULL) {
				// found a port
				gAddressModule->set_to_empty_address((sockaddr *)&endpoint->socket->address);
				gAddressModule->set_port((sockaddr *)&endpoint->socket->address, port);
				TRACE(("   EndpointManager::BindToEphemeral(%p) -> %s\n", endpoint,
					AddressString(gDomain, (sockaddr *)&endpoint->socket->address, true).Data()));
				endpoint->fEndpointNextWithSamePort = NULL;
				hash_insert(fEndpointHash, endpoint);
				hash_insert(fConnectionHash, endpoint);
				return B_OK;
			}

			counter += step;
		}
	}

	// could not find a port!
	return EADDRINUSE;
}


status_t
EndpointManager::Unbind(TCPEndpoint *endpoint)
{
	if (endpoint == NULL || !endpoint->IsBound())
		return B_BAD_VALUE;

	RecursiveLocker locker(&fLock);

	TCPEndpoint *other = _LookupEndpoint(gAddressModule->get_port((sockaddr *)&endpoint->socket->address));
	if (other != endpoint) {
		// remove endpoint from the list of endpoints with the same port
		while (other != NULL && other->fEndpointNextWithSamePort != endpoint) {
			other = other->fEndpointNextWithSamePort;
		}

		if (other != NULL)
			other->fEndpointNextWithSamePort = endpoint->fEndpointNextWithSamePort;
		else
			panic("bound endpoint %p not in hash!", endpoint);
	} else {
		// we need to replace the first endpoint in the list
		hash_remove(fEndpointHash, endpoint);

		other = endpoint->fEndpointNextWithSamePort;
		if (other != NULL)
			hash_insert(fEndpointHash, other);
	}

	endpoint->fEndpointNextWithSamePort = NULL;
	_RemoveConnection(endpoint);

	endpoint->socket->address.ss_len = 0;

	return B_OK;
}


//	#pragma mark - hash functions


/*static*/ int
EndpointManager::_ConnectionCompare(void *_endpoint, const void *_key)
{
	const connection_key *key = (connection_key *)_key;
	TCPEndpoint *endpoint = (TCPEndpoint *)_endpoint;

	if (gAddressModule->equal_addresses_and_ports(key->local,
			(sockaddr *)&endpoint->socket->address)
		&& gAddressModule->equal_addresses_and_ports(key->peer,
			(sockaddr *)&endpoint->socket->peer))
		return 0;

	return 1;
}


/*static*/ uint32
EndpointManager::_ConnectionHash(void *_endpoint, const void *_key, uint32 range)
{
	const sockaddr *local;
	const sockaddr *peer;

	if (_endpoint != NULL) {
		TCPEndpoint *endpoint = (TCPEndpoint *)_endpoint;
		local = (sockaddr *)&endpoint->socket->address;
		peer = (sockaddr *)&endpoint->socket->peer;
	} else {
		const connection_key *key = (connection_key *)_key;
		local = key->local;
		peer = key->peer;
	}

	return gAddressModule->hash_address_pair(local, peer) % range;
}


/*static*/ int
EndpointManager::_EndpointCompare(void *_endpoint, const void *_key)
{
	const endpoint_key *key = (endpoint_key *)_key;
	TCPEndpoint *endpoint = (TCPEndpoint *)_endpoint;

	return gAddressModule->get_port((sockaddr *)&endpoint->socket->address)
		== key->port ? 0 : 1;
}


/*static*/ uint32
EndpointManager::_EndpointHash(void *_endpoint, const void *_key, uint32 range)
{
	if (_endpoint != NULL) {
		TCPEndpoint *endpoint = (TCPEndpoint *)_endpoint;
		return gAddressModule->get_port((sockaddr *)&endpoint->socket->address) % range;
	}

	const endpoint_key *key = (endpoint_key *)_key;
	return key->port % range;
}

