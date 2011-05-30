#include "ServerConnection.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#ifdef USE_SSL
#include <openssl/ssl.h>
#include <openssl/rand.h>
#else
#include <string.h>
#include <sys/time.h>
#endif

#include <Autolock.h>
#include <Locker.h>


#define DEBUG_SERVER_CONNECTION

#ifdef DEBUG_SERVER_CONNECTION
#include <stdio.h>
#define TRACE(x...) printf(x)
#else
#define TRACE(x...) /* nothing */
#endif


class SocketConnection : public AbstractConnection {
public:
								SocketConnection();

			status_t			Connect(const char* server, uint32 port);
			status_t			Disconnect();

			status_t			WaitForData(bigtime_t timeout);

			int32				Read(char* buffer, uint32 nBytes);
			int32				Write(const char* buffer, uint32 nBytes);

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


	status_t
	InitCheck()
	{
		return fInit ? B_OK : B_ERROR;
	}

private:
			bool				fInit;
};


static InitSSL gInitSSL;


class SSLConnection : public SocketConnection {
public:
								SSLConnection();

			status_t			Connect(const char* server, uint32 port);
			status_t			Disconnect();

			status_t			WaitForData(bigtime_t timeout);

			int32				Read(char* buffer, uint32 nBytes);
			int32				Write(const char* buffer, uint32 nBytes);

private:
			SSL_CTX*			fCTX;
			SSL*				fSSL;
			BIO*				fBIO;
};
#endif


AbstractConnection::~AbstractConnection()
{

}


ServerConnection::ServerConnection()
	:
	fConnection(NULL)
{

}


ServerConnection::~ServerConnection()
{
	if (fConnection)
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


int32
ServerConnection::Read(char* buffer, uint32 nBytes)
{
	if (fConnection == NULL)
		return B_ERROR;
	return fConnection->Read(buffer, nBytes);
}


int32
ServerConnection::Write(const char* buffer, uint32 nBytes)
{
	if (fConnection == NULL)
		return B_ERROR;
	return fConnection->Write(buffer, nBytes);
}


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
	uint32 hostIP = inet_addr(server);
		// first see if we can parse it as a numeric address
	if (hostIP == 0 || hostIP == (uint32)-1) {
		struct hostent *he = gethostbyname(server);
		hostIP = he ? *((uint32*)he->h_addr) : 0;
	}
	if (hostIP == 0)
		return B_ERROR;

	fSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (fSocket < 0)
		return B_ERROR;

	sockaddr_in saAddr;
	memset(&saAddr, 0, sizeof(saAddr));
	saAddr.sin_family = AF_INET;
	saAddr.sin_port = htons(port);
	saAddr.sin_addr.s_addr = hostIP;
	int result = connect(fSocket, (struct sockaddr*)&saAddr,
		sizeof(saAddr));
	if (result < 0) {
		close(fSocket);
		return B_ERROR;
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
	timeval tv;
	fd_set fds;
	tv.tv_sec = long(timeout / 1e6);
	tv.tv_usec = long(timeout - (tv.tv_sec * 1e6));

	/* Initialize (clear) the socket mask. */
	FD_ZERO(&fds);
	/* Set the socket in the mask. */
	FD_SET(fSocket, &fds);

	int result = select(fSocket + 1, &fds, NULL, NULL, &tv);
	if (result == 0)
		return B_TIMED_OUT;
	if (result < 0)
		return B_ERROR;
	return B_OK;
}


int32
SocketConnection::Read(char* buffer, uint32 nBytes)
{
	return recv(fSocket, buffer, nBytes, 0);
}


int32
SocketConnection::Write(const char* buffer, uint32 nBytes)
{
	return send(fSocket, buffer, nBytes, 0);
}


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
	if (!fSSL)
		return B_ERROR;
	if (SSL_pending(fSSL) > 0) {
		return B_OK;
	}
	return SocketConnection::WaitForData(timeout);
}


int32
SSLConnection::Read(char* buffer, uint32 nBytes)
{
	if (!fSSL)
		return B_ERROR;
	return SSL_read(fSSL, buffer, nBytes);
}


int32
SSLConnection::Write(const char* buffer, uint32 nBytes)
{
	if (!fSSL)
		return B_ERROR;
	return SSL_write(fSSL, buffer, nBytes);
}


#endif
