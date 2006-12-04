/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef ENDPOINT_MANAGER_H
#define ENDPOINT_MANAGER_H


#include <lock.h>
#include <util/khash.h>

#include <sys/socket.h>


class TCPConnection;

class EndpointManager {
	public:
		EndpointManager();
		~EndpointManager();

		status_t InitCheck() const;

		recursive_lock *Locker() { return &fLock; }

		status_t SetConnection(TCPConnection *endpoint, const sockaddr *local,
			const sockaddr *peer, const sockaddr *interfaceLocal);
		TCPConnection *FindConnection(sockaddr *local, sockaddr *peer);

		status_t Bind(TCPConnection *endpoint, sockaddr *address);
		status_t BindToEphemeral(TCPConnection *endpoint, sockaddr *address);
		status_t Unbind(TCPConnection *endpoint);

	private:
		TCPConnection *_LookupConnection(sockaddr *local, sockaddr *peer);
		status_t _RemoveConnection(TCPConnection *endpoint);
		TCPConnection *_LookupEndpoint(uint16 port);
		void _DumpConnections();

		static int _ConnectionCompare(void *_endpoint, const void *_key);
		static uint32 _ConnectionHash(void *_endpoint, const void *_key, uint32 range);
		static int _EndpointCompare(void *_endpoint, const void *_key);
		static uint32 _EndpointHash(void *_endpoint, const void *_key, uint32 range);

		hash_table *fConnectionHash;
		hash_table *fEndpointHash;
		recursive_lock fLock;
};

#endif	// ENDPOINT_MANAGER_H
