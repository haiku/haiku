/*
 * Copyright 2011-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SECURE_SOCKET_H
#define _SECURE_SOCKET_H


#include <Socket.h>


class BCertificate;


class BSecureSocket : public BSocket {
public:
								BSecureSocket();
								BSecureSocket(const BNetworkAddress& peer,
									bigtime_t timeout = B_INFINITE_TIMEOUT);
								BSecureSocket(const BSecureSocket& other);
	virtual						~BSecureSocket();

	virtual bool				CertificateVerificationFailed(BCertificate&
									certificate, const char* message);

			status_t			InitCheck();

	// BSocket implementation

	virtual	status_t			Connect(const BNetworkAddress& peer,
									bigtime_t timeout = B_INFINITE_TIMEOUT);
	virtual	void				Disconnect();

	virtual	status_t			WaitForReadable(bigtime_t timeout
										= B_INFINITE_TIMEOUT) const;

	// BDataIO implementation

	virtual ssize_t				Read(void* buffer, size_t size);
	virtual ssize_t				Write(const void* buffer, size_t size);

protected:
			status_t			_Setup();

private:
	friend class BCertificate;
			class Private;
			Private*			fPrivate;
};


#endif	// _SECURE_SOCKET_H
