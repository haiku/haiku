/*
 * Copyright 2013-2016 Haiku, Inc.
 * Copyright 2011-2015, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2016, Rene Gollent, rene@gollent.com.
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include <SecureSocket.h>

#ifdef OPENSSL_ENABLED
#	include <openssl/ssl.h>
#	include <openssl/ssl3.h> // for TRACE_SESSION_KEY only
#	include <openssl/err.h>
#endif

#include <pthread.h>

#include <Certificate.h>
#include <FindDirectory.h>
#include <Path.h>

#include <AutoDeleter.h>

#include "CertificatePrivate.h"


//#define TRACE_SOCKET
#ifdef TRACE_SOCKET
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...) ;
#endif

//#define TRACE_SESSION_KEY


#ifdef OPENSSL_ENABLED

#ifdef TRACE_SESSION_KEY
#if OPENSSL_VERSION_NUMBER < 0x10100000L
/*
 * print session id and master key in NSS keylog format (RSA
 * Session-ID:<session id> Master-Key:<master key>)
 */
int SSL_SESSION_print_keylog(BIO *bp, const SSL_SESSION *x)
{
	size_t i;

	if (x == NULL)
		goto err;
	if (x->session_id_length == 0 || x->master_key_length == 0)
		goto err;

	// the RSA prefix is required by the format's definition although there's
	// nothing RSA-specific in the output, therefore, we don't have to check if
	// the cipher suite is based on RSA
	if (BIO_puts(bp, "RSA ") <= 0)
		goto err;

	if (BIO_puts(bp, "Session-ID:") <= 0)
		goto err;
	for (i = 0; i < x->session_id_length; i++) {
		if (BIO_printf(bp, "%02X", x->session_id[i]) <= 0)
			goto err;
	}
	if (BIO_puts(bp, " Master-Key:") <= 0)
		goto err;
	for (i = 0; i < (size_t)x->master_key_length; i++) {
		if (BIO_printf(bp, "%02X", x->master_key[i]) <= 0)
			goto err;
	}
	if (BIO_puts(bp, "\n") <= 0)
		goto err;

	return (1);
err:
	return (0);
}


#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */


// print client random id and master key in NSS keylog format
// as session ID is not enough.
int SSL_SESSION_print_client_random(BIO *bp, const SSL *ssl)
{
	const SSL_SESSION *x = SSL_get_session(ssl);
	size_t i;

	if (x == NULL)
		goto err;
	if (x->session_id_length == 0 || x->master_key_length == 0)
		goto err;

	if (BIO_puts(bp, "CLIENT_RANDOM ") <= 0)
		goto err;

	for (i = 0; i < sizeof(ssl->s3->client_random); i++) {
		if (BIO_printf(bp, "%02X", ssl->s3->client_random[i]) <= 0)
			goto err;
	}
	if (BIO_puts(bp, " ") <= 0)
		goto err;
	for (i = 0; i < (size_t)x->master_key_length; i++) {
		if (BIO_printf(bp, "%02X", x->master_key[i]) <= 0)
			goto err;
	}
	if (BIO_puts(bp, "\n") <= 0)
		goto err;

	return (1);
err:
	return (0);
}


#endif /* TRACE_SESSION_KEY */

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
#ifdef TRACE_SESSION_KEY
public:
	static	BIO*				sKeyLogBIO;
#endif

};


/* static */ SSL_CTX* BSecureSocket::Private::sContext = NULL;
/* static */ int BSecureSocket::Private::sDataIndex;
/* static */ pthread_once_t BSecureSocket::Private::sInitOnce
	= PTHREAD_ONCE_INIT;
#ifdef TRACE_SESSION_KEY
/* static */ BIO* BSecureSocket::Private::sKeyLogBIO = NULL;
#endif


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

		case SSL_ERROR_SYSCALL:
		{
			unsigned long error2;
			// Check for extra errors in the error stack...
			for (;;) {
				error2 = ERR_get_error();
				if (error2 == 0)
					break;
				fprintf(stderr, "SSL ERR %s\n", ERR_error_string(error2, NULL));
			}

			if (returnValue == 0)
			{
				// unexpected EOF, the remote host closed the socket without
				// telling us why.
				return ECONNREFUSED;
			}

			if (returnValue == -1)
			{
				fprintf(stderr, "SSL rv -1 %s\n", ERR_error_string(error, NULL));
				return errno;
			}

			fprintf(stderr, "SSL rv other %s\n", ERR_error_string(error, NULL));
			return B_ERROR;
		}

		case SSL_ERROR_WANT_READ:
		case SSL_ERROR_WANT_WRITE:
		case SSL_ERROR_WANT_CONNECT:
		case SSL_ERROR_WANT_ACCEPT:
		case SSL_ERROR_WANT_X509_LOOKUP:
		default:
			// TODO: translate SSL error codes!
			fprintf(stderr, "SSL other %s\n", ERR_error_string(error, NULL));
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


#if TRACE_SSL
static void apps_ssl_info_callback(const SSL *s, int where, int ret)
{
	const char *str;
	int w;

	w=where& ~SSL_ST_MASK;

	if (w & SSL_ST_CONNECT)
		str="SSL_connect";
	else if (w & SSL_ST_ACCEPT)
		str="SSL_accept";
	else
		str="undefined";

	if (where & SSL_CB_LOOP) {
		fprintf(stderr, "%s:%s\n", str, SSL_state_string_long(s));
	} else if (where & SSL_CB_ALERT) {
		str = (where & SSL_CB_READ) ? "read" : "write";
		fprintf(stderr, "SSL3 alert %s:%s:%s\n",
				str,
				SSL_alert_type_string_long(ret),
				SSL_alert_desc_string_long(ret));
	} else if (where & SSL_CB_EXIT) {
		if (ret == 0)
			fprintf(stderr, "%s:failed in %s\n",
					str, SSL_state_string_long(s));
		else if (ret < 0) {
			fprintf(stderr, "%s:error in %s\n",
					str, SSL_state_string_long(s));
		}
	}
}


#endif


/* static */ void
BSecureSocket::Private::_CreateContext()
{
	// We want SSL to report errors in human readable format.
	SSL_load_error_strings();

	// "SSLv23" means "any SSL or TLS version". We disable SSL v2 and v3 below
	// to keep only TLS 1.0 and above.
	sContext = SSL_CTX_new(SSLv23_method());

#if TRACE_SSL
	// For debugging purposes: get all SSL messages to the standard error.
	SSL_CTX_set_info_callback(sContext, apps_ssl_info_callback);
#endif

	// Disable legacy protocols. They have known vulnerabilities.
	SSL_CTX_set_options(sContext, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);

	// Disable SSL/TLS compression to prevent the CRIME attack.
	SSL_CTX_set_options(sContext, SSL_OP_NO_COMPRESSION);

	// Don't bother us with ERROR_WANT_READ.
	SSL_CTX_set_mode(sContext, SSL_MODE_AUTO_RETRY);

	// Setup cipher suites.
	// Only accept reasonably secure ones ("HIGH") and disable some known
	// broken stuff (https://wiki.openssl.org/index.php/SSL/TLS_Client)
	SSL_CTX_set_cipher_list(sContext, "HIGH:!aNULL:!PSK:!SRP:!MD5:!RC4");

	// Setup certificate verification
	SSL_CTX_set_default_verify_file(sContext);

	// OpenSSL 1.0.2 and later: use the alternate "trusted first" algorithm to
	// validate certificate chains. This makes the validation stop as soon as a
	// recognized certificate is found in the chain, instead of validating the
	// whole chain, then seeing if the root certificate is known.
#ifdef X509_V_FLAG_TRUSTED_FIRST
	X509_VERIFY_PARAM* verifyParam = X509_VERIFY_PARAM_new();
	X509_VERIFY_PARAM_set_flags(verifyParam, X509_V_FLAG_TRUSTED_FIRST);
	SSL_CTX_set1_param(sContext, verifyParam);

	// TODO we need to free this after freeing the SSL context (which we
	// currently never do)
	// X509_VERIFY_PARAM_free(verifyParam);
#endif

	// Get an unique index number for storing application data in SSL
	// structs. We will store a pointer to the BSecureSocket class there.
	sDataIndex = SSL_get_ex_new_index(0, NULL, NULL, NULL, NULL);

#ifdef TRACE_SESSION_KEY
	FILE *keylog = NULL;
	const char *logpath = getenv("SSLKEYLOGFILE");
	if (logpath)
		keylog = fopen(logpath, "w+");
	if (keylog) {
		fprintf(keylog, "# Key Log File generated by Haiku Network Kit\n");
		sKeyLogBIO = BIO_new_fp(keylog, BIO_NOCLOSE);
	}
#endif
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
BSecureSocket::Accept(BAbstractSocket*& _socket)
{
	int fd = -1;
	BNetworkAddress peer;
	status_t error = AcceptNext(fd, peer);
	if (error != B_OK)
		return error;
	BSecureSocket* socket = new(std::nothrow) BSecureSocket();
	ObjectDeleter<BSecureSocket> socketDeleter(socket);
	if (socket == NULL || socket->InitCheck() != B_OK) {
		close(fd);
		return B_NO_MEMORY;
	}

	socket->_SetTo(fd, fLocal, peer);
	error = socket->_SetupAccept();
	if (error != B_OK)
		return error;

	_socket = socket;
	socketDeleter.Detach();

	return B_OK;
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

	return _SetupConnect(peer.HostName().String());
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
	return false;
}


//	#pragma mark - BDataIO implementation


ssize_t
BSecureSocket::Read(void* buffer, size_t size)
{
	if (!IsConnected())
		return B_ERROR;

	int bytesRead;
	int retry;
	do {
		bytesRead = SSL_read(fPrivate->fSSL, buffer, size);
		if (bytesRead >= 0)
			return bytesRead;

		if (errno != EINTR) {
			// Don't retry in cases of "no data available" for non-blocking
			// sockets.
			int error = SSL_get_error(fPrivate->fSSL, bytesRead);
			if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE)
				return B_WOULD_BLOCK;
		}

		// See if the error was retryable. We may have been interrupted by
		// a signal, in which case we will retry. But it is also possible that
		// another error has occurred which is not retryable. openssl will
		// decide for us here.
		retry = BIO_should_retry(SSL_get_rbio(fPrivate->fSSL));
	} while (retry != 0);

	return fPrivate->ErrorCode(bytesRead);
}


ssize_t
BSecureSocket::Write(const void* buffer, size_t size)
{
	if (!IsConnected())
		return B_ERROR;

	int bytesWritten;
	int retry;
	do {
		bytesWritten = SSL_write(fPrivate->fSSL, buffer, size);
		if (bytesWritten >= 0)
			return bytesWritten;

		if (errno != EINTR) {
			// Don't retry in cases of "no buffer space available" for
			// non-blocking sockets.
			int error = SSL_get_error(fPrivate->fSSL, bytesWritten);
			if (error == SSL_ERROR_WANT_READ || error == SSL_ERROR_WANT_WRITE)
				return B_WOULD_BLOCK;
		}

		// See if the error was retryable. We may have been interrupted by
		// a signal, in which case we will retry. But it is also possible that
		// another error has occurred which is not retryable. openssl will
		// decide for us here.
		retry = BIO_should_retry(SSL_get_wbio(fPrivate->fSSL));
	} while (retry != 0);

	return fPrivate->ErrorCode(bytesWritten);
}


status_t
BSecureSocket::_SetupCommon(const char* host)
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
	if (host != NULL && host[0] != '\0') {
		SSL_set_tlsext_host_name(fPrivate->fSSL, host);
		X509_VERIFY_PARAM_set1_host(SSL_get0_param(fPrivate->fSSL), host, 0);
	}

	return B_OK;
}


status_t
BSecureSocket::_SetupConnect(const char* host)
{
	status_t error = _SetupCommon(host);
	if (error != B_OK)
		return error;

	int returnValue = SSL_connect(fPrivate->fSSL);
	if (returnValue <= 0) {
		TRACE("SSLConnection can't connect\n");
		BSocket::Disconnect();
		return fPrivate->ErrorCode(returnValue);
	}

#ifdef TRACE_SESSION_KEY
	fprintf(stderr, "SSL SESSION INFO:\n");
	//SSL_SESSION_print_fp(stderr, SSL_get_session(fPrivate->fSSL));
	SSL_SESSION_print_keylog(fPrivate->sKeyLogBIO, SSL_get_session(fPrivate->fSSL));
	SSL_SESSION_print_client_random(fPrivate->sKeyLogBIO, fPrivate->fSSL);
	fprintf(stderr, "\n");
#endif

	return B_OK;
}


status_t
BSecureSocket::_SetupAccept()
{
	status_t error = _SetupCommon();
	if (error != B_OK)
		return error;

	int returnValue = SSL_accept(fPrivate->fSSL);
	if (returnValue <= 0) {
		TRACE("SSLConnection can't accept\n");
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
BSecureSocket::Accept(BAbstractSocket*& _socket)
{
	return B_UNSUPPORTED;
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
BSecureSocket::_SetupCommon(const char* host)
{
	return B_UNSUPPORTED;
}


status_t
BSecureSocket::_SetupConnect(const char* host)
{
	return B_UNSUPPORTED;
}


status_t
BSecureSocket::_SetupAccept()
{
	return B_UNSUPPORTED;
}


#endif	// !OPENSSL_ENABLED
