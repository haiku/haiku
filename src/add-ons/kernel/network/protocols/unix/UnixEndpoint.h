/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNIX_ENDPOINT_H
#define UNIX_ENDPOINT_H

#include <sys/stat.h>

#include <Referenceable.h>

#include <lock.h>
#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>
#include <vfs.h>

#include <net_protocol.h>
#include <net_socket.h>
#include <ProtocolUtilities.h>

#include "unix.h"
#include "UnixAddress.h"


class UnixEndpoint;
class UnixFifo;


enum unix_endpoint_state {
	UNIX_ENDPOINT_NOT_CONNECTED,
	UNIX_ENDPOINT_LISTENING,
	UNIX_ENDPOINT_CONNECTED,
	UNIX_ENDPOINT_CLOSED
};


typedef AutoLocker<UnixEndpoint> UnixEndpointLocker;


class UnixEndpoint : public net_protocol, public ProtocolSocket,
	public Referenceable {
public:
	UnixEndpoint(net_socket* socket);
	virtual ~UnixEndpoint();

	status_t Init();
	void Uninit();

	status_t Open();
	status_t Close();
	status_t Free();

	bool Lock()
	{
		return benaphore_lock(&fLock) == B_OK;
	}

	void Unlock()
	{
		benaphore_unlock(&fLock);
	}

	status_t Bind(const struct sockaddr *_address);
	status_t Unbind();
	status_t Listen(int backlog);
	status_t Connect(const struct sockaddr *address);
	status_t Accept(net_socket **_acceptedSocket);

	status_t Send(net_buffer *buffer);
	status_t Receive(size_t numBytes, uint32 flags, net_buffer **_buffer);

	ssize_t Sendable();
	ssize_t Receivable();

	void SetReceiveBufferSize(size_t size);

	status_t Shutdown(int direction);

	bool IsBound() const
	{
		return !fIsChild && fAddress.IsValid();
	}

	const UnixAddress& Address() const
	{
		return fAddress;
	}

	HashTableLink<UnixEndpoint>* HashTableLink()
	{
		return &fAddressHashLink;
	}

private:
	void _Spawn(UnixEndpoint* connectingEndpoint, UnixFifo* fifo);
	void _Disconnect();
	status_t _LockConnectedEndpoints(UnixEndpointLocker& locker,
		UnixEndpointLocker& peerLocker);

	status_t _Bind(struct vnode* vnode);
	status_t _Bind(int32 internalID);
	status_t _Unbind();

	void _StopListening();

private:
	benaphore						fLock;
	UnixAddress						fAddress;
	::HashTableLink<UnixEndpoint>	fAddressHashLink;
	UnixEndpoint*					fPeerEndpoint;
	UnixFifo*						fReceiveFifo;
	unix_endpoint_state				fState;
	sem_id							fAcceptSemaphore;
	bool							fIsChild;
};


#endif	// UNIX_ENDPOINT_H
