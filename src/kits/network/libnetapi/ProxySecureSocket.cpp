/*
 * Copyright 2015 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <ProxySecureSocket.h>

#include <stdio.h>


BProxySecureSocket::BProxySecureSocket(const BNetworkAddress& proxy)
	:
	BSecureSocket(),
	fProxyAddress(proxy)
{
}


BProxySecureSocket::BProxySecureSocket(const BNetworkAddress& proxy, const BNetworkAddress& peer,
	bigtime_t timeout)
	:
	BSecureSocket(),
	fProxyAddress(proxy)
{
	Connect(peer, timeout);
}


BProxySecureSocket::BProxySecureSocket(const BProxySecureSocket& other)
	:
	BSecureSocket(other),
	fProxyAddress(other.fProxyAddress)
{
}


BProxySecureSocket::~BProxySecureSocket()
{
}


status_t
BProxySecureSocket::Connect(const BNetworkAddress& peer, bigtime_t timeout)
{
	status_t status = InitCheck();
	if (status != B_OK)
		return status;

	BSocket::Connect(fProxyAddress, timeout);
	if (status != B_OK)
		return status;

	BString connectRequest;
	connectRequest.SetToFormat("CONNECT %s:%d HTTP/1.0\r\n\r\n",
		peer.HostName().String(), peer.Port());
	BSocket::Write(connectRequest.String(), connectRequest.Length());

	char buffer[256];
	ssize_t length = BSocket::Read(buffer, sizeof(buffer) - 1);
	if (length <= 0)
		return length;

	buffer[length] = '\0';
	int httpStatus = 0;
	int matches = scanf(buffer, "HTTP/1.0 %d %*[^\r\n]\r\n\r\n", httpStatus);
	if (matches != 2)
		return B_BAD_DATA;

	if (httpStatus < 200 || httpStatus > 299)
		return B_BAD_VALUE;

	return _Setup();
}


