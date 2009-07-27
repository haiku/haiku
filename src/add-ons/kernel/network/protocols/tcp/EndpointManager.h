/*
 * Copyright 2006-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Hugo Santos, hugosantos@gmail.com
 */
#ifndef ENDPOINT_MANAGER_H
#define ENDPOINT_MANAGER_H


#include "tcp.h"

#include <AddressUtilities.h>

#include <lock.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/MultiHashTable.h>
#include <util/OpenHashTable.h>

#include <utility>


struct net_address_module_info;
struct net_domain;
class EndpointManager;
class TCPEndpoint;


struct ConnectionHashDefinition {
public:
	typedef std::pair<const sockaddr*, const sockaddr*> KeyType;
	typedef TCPEndpoint ValueType;

							ConnectionHashDefinition(EndpointManager* manager);
							ConnectionHashDefinition(
									const ConnectionHashDefinition& definition)
								: fManager(definition.fManager)
							{
							}

			size_t			HashKey(const KeyType& key) const;
			size_t			Hash(TCPEndpoint* endpoint) const;
			bool			Compare(const KeyType& key,
								TCPEndpoint* endpoint) const;
			TCPEndpoint*& GetLink(TCPEndpoint* endpoint) const;

private:
	EndpointManager*		fManager;
};


class EndpointHashDefinition {
public:
	typedef uint16 KeyType;
	typedef TCPEndpoint ValueType;

			size_t			HashKey(uint16 port) const;
			size_t			Hash(TCPEndpoint* endpoint) const;
			bool			Compare(uint16 port, TCPEndpoint* endpoint) const;
			bool			CompareValues(TCPEndpoint* first,
								TCPEndpoint* second) const;
			TCPEndpoint*&	GetLink(TCPEndpoint* endpoint) const;
};


class EndpointManager : public DoublyLinkedListLinkImpl<EndpointManager> {
public:
							EndpointManager(net_domain* domain);
							~EndpointManager();

			status_t		Init();

			TCPEndpoint*	FindConnection(sockaddr* local, sockaddr* peer);

			status_t		SetConnection(TCPEndpoint* endpoint,
								const sockaddr* local, const sockaddr* peer,
								const sockaddr* interfaceLocal);
			status_t		SetPassive(TCPEndpoint* endpoint);

			status_t		Bind(TCPEndpoint* endpoint,
								const sockaddr* address);
			status_t		BindChild(TCPEndpoint* endpoint);
			status_t		Unbind(TCPEndpoint* endpoint);

			status_t		ReplyWithReset(tcp_segment_header& segment,
								net_buffer* buffer);

			net_domain*		Domain() const { return fDomain; }
			net_address_module_info* AddressModule() const
								{ return Domain()->address_module; }

			void			Dump() const;

private:
			TCPEndpoint*	_LookupConnection(const sockaddr* local,
								const sockaddr* peer);
			status_t		_Bind(TCPEndpoint* endpoint,
								const sockaddr* address);
			status_t		_BindToAddress(WriteLocker& locker,
								TCPEndpoint* endpoint, const sockaddr* address);
			status_t		_BindToEphemeral(TCPEndpoint* endpoint,
								const sockaddr* address);

	typedef BOpenHashTable<ConnectionHashDefinition> ConnectionTable;
	typedef MultiHashTable<EndpointHashDefinition> EndpointTable;

	rw_lock					fLock;
	net_domain*				fDomain;
	ConnectionTable			fConnectionHash;
	EndpointTable			fEndpointHash;
	uint16					fLastPort;
};

#endif	// ENDPOINT_MANAGER_H
