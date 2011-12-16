/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DATAGRAM_SOCKET_H
#define _DATAGRAM_SOCKET_H


#include <AbstractSocket.h>


class BDatagramSocket : public BAbstractSocket {
public:
								BDatagramSocket();
								BDatagramSocket(const BNetworkAddress& peer,
									bigtime_t timeout = -1);
								BDatagramSocket(const BDatagramSocket& other);
	virtual						~BDatagramSocket();

	virtual	status_t			Bind(const BNetworkAddress& peer);
	virtual	status_t			Connect(const BNetworkAddress& peer,
									bigtime_t timeout = B_INFINITE_TIMEOUT);

			status_t			SetBroadcast(bool broadcast);
			void				SetPeer(const BNetworkAddress& peer);

	virtual	size_t				MaxTransmissionSize() const;

	virtual	size_t				SendTo(const BNetworkAddress& address,
									const void* buffer, size_t size);
	virtual	size_t				ReceiveFrom(void* buffer, size_t bufferSize,
									BNetworkAddress& from);

	// BDataIO implementation

	virtual ssize_t				Read(void* buffer, size_t size);
	virtual ssize_t				Write(const void* buffer, size_t size);
};


#endif	// _DATAGRAM_SOCKET_H
