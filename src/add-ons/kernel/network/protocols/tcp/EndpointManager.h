/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef ENDPOINT_MANAGER_H
#define ENDPOINT_MANAGER_H

#include "tcp.h"

#include <AddressUtilities.h>

#include <lock.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>

#include <utility>


struct net_address_module_info;
struct net_domain;
class EndpointManager;
class TCPEndpoint;

struct ConnectionHashDefinition {
	typedef EndpointManager *ParentType;
	typedef std::pair<const sockaddr *, const sockaddr *> KeyType;
	typedef TCPEndpoint ValueType;

	static size_t HashKey(EndpointManager *manager, const KeyType &key);
	static size_t Hash(EndpointManager *manager, TCPEndpoint *endpoint);
	static bool Compare(EndpointManager *manager, const KeyType &key,
		TCPEndpoint *endpoint);
};


struct EndpointHashDefinition {
	typedef EndpointManager *ParentType;
	typedef uint16 KeyType;
	typedef TCPEndpoint ValueType;

	static size_t HashKey(EndpointManager *manager, uint16 port);
	static size_t Hash(EndpointManager *manager, TCPEndpoint *endpoint);
	static bool Compare(EndpointManager *manager, uint16 port,
		TCPEndpoint *endpoint);
};


class EndpointManager : public DoublyLinkedListLinkImpl<EndpointManager> {
	public:
		EndpointManager(net_domain *domain);
		~EndpointManager();

		status_t InitCheck() const;

		TCPEndpoint *FindConnection(sockaddr *local, sockaddr *peer);

		status_t SetConnection(TCPEndpoint *endpoint, const sockaddr *local,
			const sockaddr *peer, const sockaddr *interfaceLocal);
		status_t SetPassive(TCPEndpoint *endpoint);

		status_t Bind(TCPEndpoint *endpoint, const sockaddr *address);
		status_t Unbind(TCPEndpoint *endpoint);

		status_t ReplyWithReset(tcp_segment_header &segment,
			net_buffer *buffer);

		net_domain *Domain() const { return fDomain; }
		net_address_module_info *AddressModule() const
			{ return Domain()->address_module; }

	private:
		TCPEndpoint *_LookupConnection(const sockaddr *local,
			const sockaddr *peer);
		status_t _Bind(TCPEndpoint *endpoint, const sockaddr *address);
		status_t _BindToAddress(TCPEndpoint *endpoint, const sockaddr *address);
		status_t _BindToEphemeral(TCPEndpoint *endpoint,
			const sockaddr *address);

		net_domain *fDomain;

		OpenHashTable<ConnectionHashDefinition> fConnectionHash;
		OpenHashTable<EndpointHashDefinition> fEndpointHash;
		benaphore fLock;
};

#endif	// ENDPOINT_MANAGER_H
