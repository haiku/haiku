/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNIX_STREAM_ENDPOINT_H
#define UNIX_STREAM_ENDPOINT_H

#include <sys/stat.h>

#include <Referenceable.h>

#include <util/DoublyLinkedList.h>
#include <util/OpenHashTable.h>

#include "unix.h"
#include "UnixEndpoint.h"

class UnixStreamEndpoint;
class UnixFifo;


enum class unix_stream_endpoint_state {
	NotConnected,
	Listening,
	Connected,
	Closed
};


typedef AutoLocker<UnixStreamEndpoint> UnixStreamEndpointLocker;


class UnixStreamEndpoint : public UnixEndpoint, public BReferenceable {
public:
								UnixStreamEndpoint(net_socket* socket);
	virtual						~UnixStreamEndpoint() override;

			status_t			Init() override;
			void				Uninit() override;

			status_t			Open() override;
			status_t			Close() override;
			status_t			Free() override;

			status_t			Bind(const struct sockaddr* _address) override;
			status_t			Unbind() override;
			status_t			Listen(int backlog) override;
			status_t			Connect(const struct sockaddr* address) override;
			status_t			Accept(net_socket** _acceptedSocket) override;

			ssize_t				Send(const iovec* vecs, size_t vecCount,
									ancillary_data_container* ancillaryData,
									const struct sockaddr* address,
									socklen_t addressLength, int flags) override;
			ssize_t				Receive(const iovec* vecs, size_t vecCount,
									ancillary_data_container** _ancillaryData,
									struct sockaddr* _address,
									socklen_t* _addressLength, int flags) override;

			ssize_t				Sendable() override;
			ssize_t				Receivable() override;

			status_t			SetReceiveBufferSize(size_t size) override;
			status_t			GetPeerCredentials(ucred* credentials) override;

			status_t			Shutdown(int direction) override;

	bool IsBound() const
	{
		return !fIsChild && fAddress.IsValid();
	}

private:
			void				_Spawn(UnixStreamEndpoint* connectingEndpoint,
									UnixStreamEndpoint* listeningEndpoint, UnixFifo* fifo);
			void				_Disconnect();
			status_t			_LockConnectedEndpoints(UnixStreamEndpointLocker& locker,
									UnixStreamEndpointLocker& peerLocker);

			status_t			_Unbind();

			void				_UnsetReceiveFifo();
			void				_StopListening();

private:
	UnixStreamEndpoint*			fPeerEndpoint;
	UnixFifo*					fReceiveFifo;
	unix_stream_endpoint_state	fState;
	sem_id						fAcceptSemaphore;
	ucred						fCredentials;
	bool						fIsChild;
	bool						fWasConnected;
};

#endif	// UNIX_STREAM_ENDPOINT_H
