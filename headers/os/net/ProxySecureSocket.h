/*
 * Copyright 2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _PROXY_SECURE_SOCKET_H
#define _PROXY_SECURE_SOCKET_H


#include <SecureSocket.h>


class BProxySecureSocket : public BSecureSocket {
public:
								BProxySecureSocket(const BNetworkAddress& proxy);
								BProxySecureSocket(const BNetworkAddress& proxy,
									const BNetworkAddress& peer,
									bigtime_t timeout = B_INFINITE_TIMEOUT);
								BProxySecureSocket(const BProxySecureSocket& other);
	virtual						~BProxySecureSocket();

	// BSocket implementation

	virtual	status_t			Connect(const BNetworkAddress& peer,
									bigtime_t timeout = B_INFINITE_TIMEOUT);

private:
	const BNetworkAddress		fProxyAddress;
};


#endif	// _PROXY_SECURE_SOCKET_H

