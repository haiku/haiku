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
	public BReferenceable {
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
		return mutex_lock(&fLock) == B_OK;
	}

	void Unlock()
	{
		mutex_unlock(&fLock);
	}

	status_t Bind(const struct sockaddr *_address);
	status_t Unbind();
	status_t Listen(int backlog);
	status_t Connect(const struct sockaddr *address);
	status_t Accept(net_socket **_acceptedSocket);

	ssize_t Send(const iovec *vecs, size_t vecCount,
		ancillary_data_container *ancillaryData);
	ssize_t Receive(const iovec *vecs, size_t vecCount,
		ancillary_data_container **_ancillaryData, struct sockaddr *_address,
		socklen_t *_addressLength);

	ssize_t Sendable();
	ssize_t Receivable();

	status_t SetReceiveBufferSize(size_t size);
	status_t GetPeerCredentials(ucred* credentials);

	status_t Shutdown(int direction);

	bool IsBound() const
	{
		return !fIsChild && fAddress.IsValid();
	}

	const UnixAddress& Address() const
	{
		return fAddress;
	}

	UnixEndpoint*& HashTableLink()
	{
		return fAddressHashLink;
	}

private:
	void _Spawn(UnixEndpoint* connectingEndpoint,
		UnixEndpoint* listeningEndpoint, UnixFifo* fifo);
	void _Disconnect();
	status_t _LockConnectedEndpoints(UnixEndpointLocker& locker,
		UnixEndpointLocker& peerLocker);

	status_t _Bind(struct vnode* vnode);
	status_t _Bind(int32 internalID);
	status_t _Unbind();

	void _UnsetReceiveFifo();
	void _StopListening();

private:
	mutex							fLock;
	UnixAddress						fAddress;
	UnixEndpoint*					fAddressHashLink;
	UnixEndpoint*					fPeerEndpoint;
	UnixFifo*						fReceiveFifo;
	unix_endpoint_state				fState;
	sem_id							fAcceptSemaphore;
	ucred							fCredentials;
	bool							fIsChild;
};

#endif	// UNIX_ENDPOINT_H
