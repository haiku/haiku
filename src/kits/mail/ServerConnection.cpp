/*
 * Copyright 2010-2011, Haiku, Inc. All rights reserved.
 * Copyright 2010, Clemens Zeidler <haiku@clemens-zeidler.de>
 * Distributed under the terms of the MIT License.
 */


#include "ServerConnection.h"

#include <errno.h>
#include <sys/poll.h>
#include <unistd.h>

#ifdef USE_SSL
#	include <openssl/ssl.h>
#	include <openssl/rand.h>
#endif

#include <Autolock.h>
#include <Locker.h>
#include <NetworkAddress.h>


#define DEBUG_SERVER_CONNECTION
#ifdef DEBUG_SERVER_CONNECTION
#	include <stdio.h>
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...) ;
#endif


namespace BPrivate {


class AbstractConnection {
public:
	virtual						~AbstractConnection();

	virtual status_t			Connect(const char* server, uint32 port) = 0;
	virtual status_t			Disconnect() = 0;

	virtual status_t			WaitForData(bigtime_t timeout) = 0;

	virtual ssize_t				Read(char* buffer, uint32 length) = 0;
	virtual ssize_t				Write(const char* buffer, uint32 length) = 0;
};


class SocketConnection : public AbstractConnection {
public:
								SocketConnection();

			status_t			Connect(const char* server, uint32 port);
			status_t			Disconnect();

			status_t			WaitForData(bigtime_t timeout);

			ssize_t				Read(char* buffer, uint32 length);
			ssize_t				Write(const char* buffer, uint32 length);

protected:
			int					fSocket;
};


#ifdef USE_SSL


class InitSSL {
public:
	InitSSL()
	{
		if (SSL_library_init() != 1) {
			fInit = false;
			return;
		}

		// use combination of more or less random this pointer and system time
		int64 seed = (int64)this + system_time();
		RAND_seed(&seed, sizeof(seed));

		fInit = true;
		return;
	};


	status_t InitCheck()
	{
		return fInit ? B_OK : B_ERROR;
	}

private:
			bool				fInit;
};


class SSLConnection : public SocketConnection {
public:
								SSLConnection();

			status_t			Connect(const char* server, uint32 port);
			status_t			Disconnect();

			status_t			WaitForData(bigtime_t timeout);

			ssize_t				Read(char* buffer, uint32 length);
			ssize_t				Write(const char* buffer, uint32 length);

private:
			SSL_CTX*			fCTX;
			SSL*				fSSL;
			BIO*				fBIO;
};


static InitSSL gInitSSL;


#endif	// USE_SSL


AbstractConnection::~AbstractConnection()
{
}


// #pragma mark -


ServerConnection::ServerConnection()
	:
	fConnection(NULL)
{
}


ServerConnection::~ServerConnection()
{
	if (fConnection != NULL)
		fConnection->Disconnect();
	delete fConnection;
}


status_t
ServerConnection::ConnectSSL(const char* server, uint32 port)
{
#ifdef USE_SSL
	delete fConnection;
	fConnection = new SSLConnection;
	return fConnection->Connect(server, port);
#else
	return B_ERROR;
#endif
}


status_t
ServerConnection::ConnectSocket(const char* server, uint32 port)
{
	delete fConnection;
	fConnection = new SocketConnection;
	return fConnection->Connect(server, port);
}


status_t
ServerConnection::Disconnect()
{
	if (fConnection == NULL)
		return B_ERROR;
	return fConnection->Disconnect();
}


status_t
ServerConnection::WaitForData(bigtime_t timeout)
{
	if (fConnection == NULL)
		return B_ERROR;
	return fConnection->WaitForData(timeout);
}


ssize_t
ServerConnection::Read(char* buffer, uint32 nBytes)
{
	if (fConnection == NULL)
		return B_ERROR;
	return fConnection->Read(buffer, nBytes);
}


ssize_t
ServerConnection::Write(const char* buffer, uint32 nBytes)
{
	if (fConnection == NULL)
		return B_ERROR;
	return fConnection->Write(buffer, nBytes);
}


// #pragma mark -


SocketConnection::SocketConnection()
	:
	fSocket(-1)
{
}


status_t
SocketConnection::Connect(const char* server, uint32 port)
{
	if (fSocket >= 0)
		Disconnect();

	TRACE("SocketConnection to server %s:%i\n", server, (int)port);

	BNetworkAddress address;
	status_t status = address.SetTo(server, port);
	if (status != B_OK)
		return status;

	fSocket = socket(address.Family(), SOCK_STREAM, 0);
	if (fSocket < 0)
		return errno;

	int result = connect(fSocket, address, address.Length());
	if (result < 0) {
		close(fSocket);
		return errno;
	}

	TRACE("SocketConnection: connected\n");

	return B_OK;
}


status_t
SocketConnection::Disconnect()
{
	close(fSocket);
	fSocket = -1;
	return B_OK;
}


status_t
SocketConnection::WaitForData(bigtime_t timeout)
{
	struct pollfd entry;
	entry.fd = fSocket;
	entry.events = POLLIN;

	int timeoutMillis = -1;
	if (timeout > 0)
		timeoutMillis = timeout / 1000;

	int result = poll(&entry, 1, timeoutMillis);
	if (result == 0)
		return B_TIMED_OUT;
	if (result < 0)
		return errno;

	return B_OK;
}


ssize_t
SocketConnection::Read(char* buffer, uint32 length)
{
	ssize_t bytesReceived = recv(fSocket, buffer, length, 0);
	if (bytesReceived < 0)
		return errno;

	return bytesReceived;
}


ssize_t
SocketConnection::Write(const char* buffer, uint32 length)
{
	ssize_t bytesWritten = send(fSocket, buffer, length, 0);
	if (bytesWritten < 0)
		return errno;

	return bytesWritten;
}


// #pragma mark -


#ifdef USE_SSL


SSLConnection::SSLConnection()
	:
	fCTX(NULL),
	fSSL(NULL),
	fBIO(NULL)
{
}


status_t
SSLConnection::Connect(const char* server, uint32 port)
{
	if (fSSL != NULL)
		Disconnect();

	if (gInitSSL.InitCheck() != B_OK)
		return B_ERROR;

	status_t status = SocketConnection::Connect(server, port);
	if (status != B_OK)
		return status;

	fCTX = SSL_CTX_new(SSLv23_method());
	fSSL = SSL_new(fCTX);
	fBIO = BIO_new_socket(fSocket, BIO_NOCLOSE);
	SSL_set_bio(fSSL, fBIO, fBIO);

	if (SSL_connect(fSSL) <= 0) {
		TRACE("SSLConnection can't connect\n");
		SocketConnection::Disconnect();
    	return B_ERROR;
	}

	TRACE("SSLConnection connected\n");
	return B_OK;
}


status_t
SSLConnection::Disconnect()
{
	TRACE("SSLConnection::Disconnect()\n");

	if (fSSL)
		SSL_shutdown(fSSL);
	if (fCTX)
		SSL_CTX_free(fCTX);
	if (fBIO)
		BIO_free(fBIO);

	fSSL = NULL;
	fCTX = NULL;
	fBIO = NULL;
	return SocketConnection::Disconnect();
}


status_t
SSLConnection::WaitForData(bigtime_t timeout)
{
	if (fSSL == NULL)
		return B_NO_INIT;
	if (SSL_pending(fSSL) > 0)
		return B_OK;

	return SocketConnection::WaitForData(timeout);
}


ssize_t
SSLConnection::Read(char* buffer, uint32 length)
{
	if (fSSL == NULL)
		return B_NO_INIT;

	int bytesRead = SSL_read(fSSL, buffer, length);
	if (bytesRead > 0)
		return bytesRead;

	// TODO: translate SSL error codes!
	return B_ERROR;
}


ssize_t
SSLConnection::Write(const char* buffer, uint32 length)
{
	if (fSSL == NULL)
		return B_NO_INIT;

	int bytesWritten = SSL_write(fSSL, buffer, length);
	if (bytesWritten > 0)
		return bytesWritten;

	// TODO: translate SSL error codes!
	return B_ERROR;
}


#endif	// USE_SSL


}	// namespace BPrivate
