/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SECURE_SOCKET_H
#define _SECURE_SOCKET_H


#include <Socket.h>


class BSecureSocket : public BSocket {
public:
								BSecureSocket();
								BSecureSocket(const BNetworkAddress& peer,
									bigtime_t timeout = B_INFINITE_TIMEOUT);
								BSecureSocket(const BSecureSocket& other);
	virtual						~BSecureSocket();

	virtual	status_t			Connect(const BNetworkAddress& peer,
									bigtime_t timeout = B_INFINITE_TIMEOUT);
	virtual	void				Disconnect();

	virtual	status_t			WaitForReadable(bigtime_t timeout
										= B_INFINITE_TIMEOUT) const;

	// BDataIO implementation

	virtual ssize_t				Read(void* buffer, size_t size);
	virtual ssize_t				Write(const void* buffer, size_t size);

private:
			class Private;
			Private*			fPrivate;
};


#endif	// _SECURE_SOCKET_H
