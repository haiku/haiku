/*
 * Copyright 2023, Trung Nguyen, trungnt282910@gmail.com.
 * Distributed under the terms of the MIT License.
 */
#ifndef UNIX_ENDPOINT_H
#define UNIX_ENDPOINT_H


#include <net_protocol.h>
#include <net_socket.h>
#include <ProtocolUtilities.h>

#include <lock.h>
#include <vfs.h>

#include "UnixAddress.h"


class UnixEndpoint : public net_protocol, public ProtocolSocket {
public:
	virtual						~UnixEndpoint();

	bool Lock()
	{
		return mutex_lock(&fLock) == B_OK;
	}

	void Unlock()
	{
		mutex_unlock(&fLock);
	}

	const UnixAddress& Address() const
	{
		return fAddress;
	}

	UnixEndpoint*& HashTableLink()
	{
		return fAddressHashLink;
	}

	virtual	status_t			Init() = 0;
	virtual	void				Uninit() = 0;

	virtual	status_t			Open() = 0;
	virtual	status_t			Close() = 0;
	virtual	status_t			Free() = 0;

	virtual	status_t			Bind(const struct sockaddr* _address) = 0;
	virtual	status_t			Unbind() = 0;
	virtual	status_t			Listen(int backlog) = 0;
	virtual	status_t			Connect(const struct sockaddr* address) = 0;
	virtual	status_t			Accept(net_socket** _acceptedSocket) = 0;

	virtual	ssize_t				Send(const iovec* vecs, size_t vecCount,
									ancillary_data_container* ancillaryData,
									const struct sockaddr* address,
									socklen_t addressLength, int flags) = 0;
	virtual	ssize_t				Receive(const iovec* vecs, size_t vecCount,
									ancillary_data_container** _ancillaryData,
									struct sockaddr* _address, socklen_t* _addressLength,
									int flags) = 0;

	virtual	ssize_t				Sendable() = 0;
	virtual	ssize_t				Receivable() = 0;

	virtual	status_t			SetReceiveBufferSize(size_t size) = 0;
	virtual	status_t			GetPeerCredentials(ucred* credentials) = 0;

	virtual	status_t			Shutdown(int direction) = 0;

	static	status_t			Create(net_socket* socket, UnixEndpoint** _endpoint);

protected:
								UnixEndpoint(net_socket* socket);

	// These functions perform no locking or checking on the endpoint.
			status_t			_Bind(const struct sockaddr_un* address);
			status_t			_Unbind();

private:
			status_t			_Bind(struct vnode* vnode);
			status_t			_Bind(int32 internalID);

protected:
			UnixAddress			fAddress;

private:
			mutex				fLock;
			UnixEndpoint*		fAddressHashLink;
};


static inline bigtime_t
absolute_timeout(bigtime_t timeout)
{
	if (timeout == 0 || timeout == B_INFINITE_TIMEOUT)
		return timeout;

// TODO: Make overflow safe!
	return timeout + system_time();
}


#endif	// UNIX_ENDPOINT_H
