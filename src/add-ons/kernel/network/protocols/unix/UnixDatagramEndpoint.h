/*
 * Copyright 2023, Trung Nguyen, trungnt282910@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNIX_DATAGRAM_ENDPOINT_H
#define UNIX_DATAGRAM_ENDPOINT_H


#include <Referenceable.h>

#include "UnixEndpoint.h"


class UnixFifo;


class UnixDatagramEndpoint : public UnixEndpoint, public BReferenceable {
public:
								UnixDatagramEndpoint(net_socket* socket);
	virtual						~UnixDatagramEndpoint() override;

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
		return fAddress.IsValid();
	}

private:
	static	status_t			_InitializeEndpoint(const struct sockaddr* _address,
									BReference<UnixDatagramEndpoint> &outEndpoint);

			status_t			_Disconnect();
			void				_UnsetReceiveFifo();

private:
		UnixDatagramEndpoint*	fTargetEndpoint;
		UnixFifo*				fReceiveFifo;
		bool					fShutdownWrite:1;
		bool					fShutdownRead:1;
};


#endif	// UNIX_DATAGRAM_ENDPOINT_H
