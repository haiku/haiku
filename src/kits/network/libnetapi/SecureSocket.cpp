/*
 * Copyright 2014 Haiku, Inc.
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include <SecureSocket.h>

#ifdef OPENSSL_ENABLED
#	include <openssl/ssl.h>
#endif

#include <Certificate.h>
#include <FindDirectory.h>
#include <Path.h>

#include "CertificatePrivate.h"


//#define TRACE_SOCKET
#ifdef TRACE_SOCKET
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...) ;
#endif


#ifdef OPENSSL_ENABLED


class BSecureSocket::Private {
public:
	static	SSL_CTX*			Context();
	static	int					VerifyCallback(int ok, X509_STORE_CTX* ctx);
public:
			SSL*				fSSL;
			BIO*				fBIO;
	static	int					sDataIndex;
private:
	static	SSL_CTX*			sContext;
		// FIXME When do we SSL_CTX_free it?
	static	vint32				sInitOnce;
};


/* static */ SSL_CTX* BSecureSocket::Private::sContext = NULL;
/* static */ int BSecureSocket::Private::sDataIndex;
/* static */ vint32 BSecureSocket::Private::sInitOnce = false;


/* static */ SSL_CTX*
BSecureSocket::Private::Context()
{
	// We use lazy initialisation here, because reading certificates from disk
	// and parsing them is a relatively long operation and uses some memory.
	// We don't want programs that don't use SSL to waste resources with that.
	if (!sInitOnce) {
		sInitOnce = true;
		sContext = SSL_CTX_new(SSLv23_method());

		// Setup certificate verification
		BPath certificateStore;
		find_directory(B_SYSTEM_DATA_DIRECTORY, &certificateStore);
		certificateStore.Append("ssl/CARootCertificates.pem");
		// TODO we may want to add a non-packaged certificate directory?
		// (would make it possible to store user-added certificate exceptions
		// there)
		SSL_CTX_load_verify_locations(sContext, certificateStore.Path(), NULL);
		SSL_CTX_set_verify(sContext, SSL_VERIFY_PEER, VerifyCallback);

		// Get an unique index number for storing application data in SSL
		// structs. We will store a pointer to the BSecureSocket class there.
		sDataIndex = SSL_get_ex_new_index(0, NULL, NULL, NULL, NULL);
	}

	return sContext;
}


// This is called each time a certificate verification occurs. It allows us to
// catch failures and report them.
/* static */ int
BSecureSocket::Private::VerifyCallback(int ok, X509_STORE_CTX* ctx)
{
	// OpenSSL already checked the certificate again the certificate store for
	// us, and tells the result of that in the ok parameter.
	
	// If the verification succeeded, no need for any further checks. Let's
	// proceed with the connection.
	if (ok)
		return ok;

	// The certificate verification failed. Signal this to the caller, and fail.

	// First of all, get the affected BSecureSocket
	SSL* ssl = (SSL*)X509_STORE_CTX_get_ex_data(ctx,
		SSL_get_ex_data_X509_STORE_CTX_idx());
	BSecureSocket* socket = (BSecureSocket*)SSL_get_ex_data(ssl, sDataIndex);

	// Get the certificate that we could not validate (this may not be the one
	// we got from the server, but something higher up in the certificate
	// chain)
	X509* certificate = X509_STORE_CTX_get_current_cert(ctx);

	return socket->CertificateVerificationFailed(BCertificate(
		new BCertificate::Private(certificate)));
}


// # pragma mark - BSecureSocket


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
	fPrivate = (BSecureSocket::Private*)malloc(sizeof(BSecureSocket::Private));
	if (fPrivate != NULL) {
		memcpy(fPrivate, other.fPrivate, sizeof(BSecureSocket::Private));
			// TODO: this won't work this way!
		SSL_set_ex_data(fPrivate->fSSL, Private::sDataIndex, this);
	} else
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

	fPrivate->fSSL = SSL_new(BSecureSocket::Private::Context());
	fPrivate->fBIO = BIO_new_socket(fSocket, BIO_NOCLOSE);
	SSL_set_bio(fPrivate->fSSL, fPrivate->fBIO, fPrivate->fBIO);
	SSL_set_ex_data(fPrivate->fSSL, Private::sDataIndex, this);

	printf("Connecting %p\n", fPrivate->fSSL);
	int sslStatus = SSL_connect(fPrivate->fSSL);
   
	if (sslStatus <= 0)	{
		TRACE("SSLConnection can't connect\n");
		BSocket::Disconnect();

		switch (SSL_get_error(fPrivate->fSSL, sslStatus)) {
			case SSL_ERROR_NONE:
				// Shouldn't happen...
				return B_NO_ERROR;
			case SSL_ERROR_ZERO_RETURN:
				// Socket is closed
				return B_CANCELED;
			case SSL_ERROR_SSL:
				// Probably no certificate
				return B_NOT_ALLOWED;
			default:
				return B_ERROR;
		}
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

		BSocket::Disconnect();
			// Must do this before freeing the BIO, to make sure any pending
			// read or write gets unlocked properly.
		if (fPrivate->fBIO != NULL) {
			BIO_free(fPrivate->fBIO);
			fPrivate->fBIO = NULL;
		}
	}
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


bool
BSecureSocket::CertificateVerificationFailed(BCertificate)
{
	// Until apps actually make use of the certificate API, let's keep the old
	// behavior and accept all connections, even if the certificate validation
	// didn't work.
	return true;
}


//	#pragma mark - BDataIO implementation


ssize_t
BSecureSocket::Read(void* buffer, size_t size)
{
	if (!IsConnected())
		return B_ERROR;

	int bytesRead = SSL_read(fPrivate->fSSL, buffer, size);
	if (bytesRead >= 0)
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
	if (bytesWritten >= 0)
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
