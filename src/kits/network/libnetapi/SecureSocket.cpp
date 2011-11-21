/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include <SecureSocket.h>

#ifdef OPENSSL_ENABLED
#	include <openssl/ssl.h>
#endif


//#define TRACE_SOCKET
#ifdef TRACE_SOCKET
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...) ;
#endif


#ifdef OPENSSL_ENABLED


class BSecureSocket::Private {
public:
			SSL_CTX*			fCTX;
			SSL*				fSSL;
			BIO*				fBIO;
};


BSecureSocket::BSecureSocket()
	:
	fPrivate(NULL)
{
}


BSecureSocket::BSecureSocket(const BNetworkAddress& peer, bigtime_t timeout)
	:
	fPrivate(NULL)
{
	Connect(peer, timeout);
}


BSecureSocket::BSecureSocket(const BSecureSocket& other)
	:
	BSocket(other)
{
	// TODO: this won't work this way!
	fPrivate = (BSecureSocket::Private*)malloc(sizeof(BSecureSocket::Private));
	if (fPrivate != NULL)
		memcpy(fPrivate, other.fPrivate, sizeof(BSecureSocket::Private));
	else
		fInitStatus = B_NO_MEMORY;
}


BSecureSocket::~BSecureSocket()
{
	free(fPrivate);
}


status_t
BSecureSocket::Connect(const BNetworkAddress& peer, bigtime_t timeout)
{
	if (fPrivate == NULL) {
		fPrivate = (BSecureSocket::Private*)calloc(1,
			sizeof(BSecureSocket::Private));
		if (fPrivate == NULL)
			return B_NO_MEMORY;
	}

	status_t status = BSocket::Connect(peer, timeout);
	if (status != B_OK)
		return status;

	fPrivate->fCTX = SSL_CTX_new(SSLv23_method());
	fPrivate->fSSL = SSL_new(fPrivate->fCTX);
	fPrivate->fBIO = BIO_new_socket(fSocket, BIO_NOCLOSE);
	SSL_set_bio(fPrivate->fSSL, fPrivate->fBIO, fPrivate->fBIO);

	if (SSL_connect(fPrivate->fSSL) <= 0) {
		TRACE("SSLConnection can't connect\n");
		BSocket::Disconnect();
		// TODO: translate ssl to Haiku error
		return B_ERROR;
	}

	return B_OK;
}


void
BSecureSocket::Disconnect()
{
	if (IsConnected()) {
		if (fPrivate->fSSL != NULL) {
			SSL_shutdown(fPrivate->fSSL);
			fPrivate->fSSL = NULL;
		}
		if (fPrivate->fCTX != NULL) {
			SSL_CTX_free(fPrivate->fCTX);
			fPrivate->fCTX = NULL;
		}
		if (fPrivate->fBIO != NULL) {
			BIO_free(fPrivate->fBIO);
			fPrivate->fBIO = NULL;
		}
	}
	return BSocket::Disconnect();
}


status_t
BSecureSocket::WaitForReadable(bigtime_t timeout) const
{
	if (fInitStatus != B_OK)
		return fInitStatus;
	if (!IsConnected())
		return B_ERROR;

	if (SSL_pending(fPrivate->fSSL) > 0)
		return B_OK;

	return BSocket::WaitForReadable(timeout);
}


//	#pragma mark - BDataIO implementation


ssize_t
BSecureSocket::Read(void* buffer, size_t size)
{
	if (!IsConnected())
		return B_ERROR;

	int bytesRead = SSL_read(fPrivate->fSSL, buffer, size);
	if (bytesRead > 0)
		return bytesRead;

	// TODO: translate SSL error codes!
	return B_ERROR;
}


ssize_t
BSecureSocket::Write(const void* buffer, size_t size)
{
	if (!IsConnected())
		return B_ERROR;

	int bytesWritten = SSL_write(fPrivate->fSSL, buffer, size);
	if (bytesWritten > 0)
		return bytesWritten;

	// TODO: translate SSL error codes!
	return B_ERROR;
}


#else	// OPENSSL_ENABLED


// #pragma mark - No-SSL stubs


BSecureSocket::BSecureSocket()
{
}


BSecureSocket::BSecureSocket(const BNetworkAddress& peer, bigtime_t timeout)
{
	fInitStatus = B_UNSUPPORTED;
}


BSecureSocket::BSecureSocket(const BSecureSocket& other)
	:
	BSocket(other)
{
}


BSecureSocket::~BSecureSocket()
{
}


status_t
BSecureSocket::Connect(const BNetworkAddress& peer, bigtime_t timeout)
{
	return fInitStatus = B_UNSUPPORTED;
}


void
BSecureSocket::Disconnect()
{
}


status_t
BSecureSocket::WaitForReadable(bigtime_t timeout) const
{
	return B_UNSUPPORTED;
}


//	#pragma mark - BDataIO implementation


ssize_t
BSecureSocket::Read(void* buffer, size_t size)
{
	return B_UNSUPPORTED;
}


ssize_t
BSecureSocket::Write(const void* buffer, size_t size)
{
	return B_UNSUPPORTED;
}


#endif	// !OPENSSL_ENABLED
