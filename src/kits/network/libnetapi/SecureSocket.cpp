/*
 * Copyright 2013-2015 Haiku, Inc.
 * Copyright 2011-2015, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include <SecureSocket.h>

#ifdef OPENSSL_ENABLED
#	include <openssl/ssl.h>
#endif

#include <pthread.h>

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
								Private();
								~Private();

			status_t			InitCheck();
			status_t			ErrorCode(int returnValue);

	static	SSL_CTX*			Context();
	static	int					VerifyCallback(int ok, X509_STORE_CTX* ctx);

private:
	static	void				_CreateContext();

public:
			SSL*				fSSL;
			BIO*				fBIO;
	static	int					sDataIndex;

private:
	static	SSL_CTX*			sContext;
		// FIXME When do we SSL_CTX_free it?
	static	pthread_once_t		sInitOnce;
};


/* static */ SSL_CTX* BSecureSocket::Private::sContext = NULL;
/* static */ int BSecureSocket::Private::sDataIndex;
/* static */ pthread_once_t BSecureSocket::Private::sInitOnce
	= PTHREAD_ONCE_INIT;


BSecureSocket::Private::Private()
	:
	fSSL(NULL),
	fBIO(BIO_new(BIO_s_socket()))
{
}


BSecureSocket::Private::~Private()
{
	// SSL_free also frees the underlying BIO.
	if (fSSL != NULL)
		SSL_free(fSSL);
	else {
		// The SSL session was never created (Connect() was not called or
		// failed). We must free the BIO we created in the constructor.
		BIO_free(fBIO);
	}
}


status_t
BSecureSocket::Private::InitCheck()
{
	if (fBIO == NULL)
		return B_NO_MEMORY;
	return B_OK;
}


status_t
BSecureSocket::Private::ErrorCode(int returnValue)
{
	int error = SSL_get_error(fSSL, returnValue);
	switch (error) {
		case SSL_ERROR_NONE:
			// Shouldn't happen...
			return B_NO_ERROR;
		case SSL_ERROR_ZERO_RETURN:
			// Socket is closed
			return B_CANCELED;
		case SSL_ERROR_SSL:
			// Probably no certificate
			return B_NOT_ALLOWED;

		case SSL_ERROR_WANT_READ:
		case SSL_ERROR_WANT_WRITE:
		case SSL_ERROR_WANT_CONNECT:
		case SSL_ERROR_WANT_ACCEPT:
		case SSL_ERROR_WANT_X509_LOOKUP:
		case SSL_ERROR_SYSCALL:
		default:
			// TODO: translate SSL error codes!
			fprintf(stderr, "SSL error: %d\n", error);
			return B_ERROR;
	}
}


/* static */ SSL_CTX*
BSecureSocket::Private::Context()
{
	// We use lazy initialisation here, because reading certificates from disk
	// and parsing them is a relatively long operation and uses some memory.
	// We don't want programs that don't use SSL to waste resources with that.
	pthread_once(&sInitOnce, _CreateContext);

	return sContext;
}


/*!	This is called each time a certificate verification occurs. It allows us to
	catch failures and report them.
*/
/* static */ int
BSecureSocket::Private::VerifyCallback(int ok, X509_STORE_CTX* ctx)
{
	// OpenSSL already checked the certificate again the certificate store for
	// us, and tells the result of that in the ok parameter.

	// If the verification succeeded, no need for any further checks. Let's
	// proceed with the connection.
	if (ok)
		return ok;

	// The certificate verification failed. Signal this to the BSecureSocket.

	// First of all, get the affected BSecureSocket
	SSL* ssl = (SSL*)X509_STORE_CTX_get_ex_data(ctx,
		SSL_get_ex_data_X509_STORE_CTX_idx());
	BSecureSocket* socket = (BSecureSocket*)SSL_get_ex_data(ssl, sDataIndex);

	// Get the certificate that we could not validate (this may not be the one
	// we got from the server, but something higher up in the certificate
	// chain)
	X509* x509 = X509_STORE_CTX_get_current_cert(ctx);
	BCertificate::Private* certificate
		= new(std::nothrow) BCertificate::Private(x509);

	if (certificate == NULL)
		return 0;

	int error = X509_STORE_CTX_get_error(ctx);
	const char* message = X509_verify_cert_error_string(error);

	// Let the BSecureSocket (or subclass) decide if we should continue anyway.
	BCertificate failedCertificate(certificate);
	return socket->CertificateVerificationFailed(failedCertificate, message);
}


/* static */ void
BSecureSocket::Private::_CreateContext()
{
	sContext = SSL_CTX_new(SSLv23_method());

	// Disable legacy protocols. They have known vulnerabilities.
	SSL_CTX_set_options(sContext, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

	// Don't bother us with ERROR_WANT_READ.
	SSL_CTX_set_mode(sContext, SSL_MODE_AUTO_RETRY);

	// Setup certificate verification
	BPath certificateStore;
	find_directory(B_SYSTEM_DATA_DIRECTORY, &certificateStore);
	certificateStore.Append("ssl/CARootCertificates.pem");
	// TODO we may want to add a non-packaged certificate directory?
	// (would make it possible to store user-added certificate exceptions
	// there)
	SSL_CTX_load_verify_locations(sContext, certificateStore.Path(), NULL);
	SSL_CTX_set_verify(sContext, SSL_VERIFY_PEER, VerifyCallback);

	// OpenSSL 1.0.2 and later: use the alternate "trusted first" algorithm to validate certificate
	// chains. This makes the validation stop as soon as a recognized certificate is found in the
	// chain, instead of validating the whole chain, then seeing if the root certificate is known.
#ifdef X509_V_FLAG_TRUSTED_FIRST
	X509_VERIFY_PARAM* verifyParam = X509_VERIFY_PARAM_new();
	X509_VERIFY_PARAM_set_flags(verifyParam, X509_V_FLAG_TRUSTED_FIRST);
	SSL_CTX_set1_param(sContext, verifyParam);

	// TODO we need to free this after freeing the SSL context (which we currently never do)
	// X509_VERIFY_PARAM_free(verifyParam);
#endif

	// Get an unique index number for storing application data in SSL
	// structs. We will store a pointer to the BSecureSocket class there.
	sDataIndex = SSL_get_ex_new_index(0, NULL, NULL, NULL, NULL);
}


// # pragma mark - BSecureSocket


BSecureSocket::BSecureSocket()
	:
	fPrivate(new(std::nothrow) BSecureSocket::Private())
{
	fInitStatus = fPrivate != NULL ? fPrivate->InitCheck() : B_NO_MEMORY;
}


BSecureSocket::BSecureSocket(const BNetworkAddress& peer, bigtime_t timeout)
	:
	fPrivate(new(std::nothrow) BSecureSocket::Private())
{
	fInitStatus = fPrivate != NULL ? fPrivate->InitCheck() : B_NO_MEMORY;
	Connect(peer, timeout);
}


BSecureSocket::BSecureSocket(const BSecureSocket& other)
	:
	BSocket(other)
{
	fPrivate = new(std::nothrow) BSecureSocket::Private(*other.fPrivate);
		// TODO: this won't work this way! - write working copy constructor for
		// Private.

	if (fPrivate != NULL)
		SSL_set_ex_data(fPrivate->fSSL, Private::sDataIndex, this);
	else
		fInitStatus = B_NO_MEMORY;

}


BSecureSocket::~BSecureSocket()
{
	delete fPrivate;
}


status_t
BSecureSocket::Connect(const BNetworkAddress& peer, bigtime_t timeout)
{
	status_t status = InitCheck();
	if (status != B_OK)
		return status;

	status = BSocket::Connect(peer, timeout);
	if (status != B_OK)
		return status;

	return _Setup();
}


void
BSecureSocket::Disconnect()
{
	if (IsConnected()) {
		if (fPrivate->fSSL != NULL)
			SSL_shutdown(fPrivate->fSSL);

		BSocket::Disconnect();
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


status_t
BSecureSocket::InitCheck()
{
	if (fPrivate == NULL)
		return B_NO_MEMORY;

	status_t state = fPrivate->InitCheck();
	return state;
}


bool
BSecureSocket::CertificateVerificationFailed(BCertificate&, const char*)
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

	return fPrivate->ErrorCode(bytesRead);
}


ssize_t
BSecureSocket::Write(const void* buffer, size_t size)
{
	if (!IsConnected())
		return B_ERROR;

	int bytesWritten = SSL_write(fPrivate->fSSL, buffer, size);
	if (bytesWritten >= 0)
		return bytesWritten;

	return fPrivate->ErrorCode(bytesWritten);
}


status_t
BSecureSocket::_Setup()
{
	// Do this only after BSocket::Connect has checked wether we're already
	// connected. We don't want to kill an existing SSL session, as that would
	// likely crash the protocol loop for it.
	if (fPrivate->fSSL != NULL) {
		SSL_free(fPrivate->fSSL);
	}

	fPrivate->fSSL = SSL_new(BSecureSocket::Private::Context());
	if (fPrivate->fSSL == NULL) {
		BSocket::Disconnect();
		return B_NO_MEMORY;
	}

	BIO_set_fd(fPrivate->fBIO, fSocket, BIO_NOCLOSE);
	SSL_set_bio(fPrivate->fSSL, fPrivate->fBIO, fPrivate->fBIO);
	SSL_set_ex_data(fPrivate->fSSL, Private::sDataIndex, this);

	int returnValue = SSL_connect(fPrivate->fSSL);
	if (returnValue <= 0) {
		TRACE("SSLConnection can't connect\n");
		BSocket::Disconnect();
		return fPrivate->ErrorCode(returnValue);
	}

	return B_OK;
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


bool
BSecureSocket::CertificateVerificationFailed(BCertificate& certificate, const char*)
{
	(void)certificate;
	return false;
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


status_t
BSecureSocket::InitCheck()
{
	return B_UNSUPPORTED;
}


status_t
BSecureSocket::_Setup()
{
	return B_UNSUPPORTED;
}


#endif	// !OPENSSL_ENABLED
