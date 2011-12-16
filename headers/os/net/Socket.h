/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SOCKET_H
#define _SOCKET_H


#include <AbstractSocket.h>


class BSocket : public BAbstractSocket {
public:
								BSocket();
								BSocket(const BNetworkAddress& peer,
									bigtime_t timeout = B_INFINITE_TIMEOUT);
								BSocket(const BSocket& other);
	virtual						~BSocket();

	virtual	status_t			Bind(const BNetworkAddress& peer);
	virtual	status_t			Connect(const BNetworkAddress& peer,
									bigtime_t timeout = B_INFINITE_TIMEOUT);

	// BDataIO implementation

	virtual ssize_t				Read(void* buffer, size_t size);
	virtual ssize_t				Write(const void* buffer, size_t size);

private:
	friend class BServerSocket;

			void				_SetTo(int fd, const BNetworkAddress& local,
									const BNetworkAddress& peer);
};


#endif	// _SOCKET_H
