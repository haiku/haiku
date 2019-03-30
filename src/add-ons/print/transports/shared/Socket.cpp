// Sun, 18 Jun 2000
// Y.Takagi

#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "Socket.h"
#include "SocketStream.h"

#ifndef INADDR_NONE
#define INADDR_NONE		0xffffffff
#endif

#include <errno.h>

Socket::Socket(const char *host, int port)
	: __sock(-1), __is(NULL), __os(NULL), __error(false)
{
	__host         = host;
	__port         = port;
	__localPort    = -1;
	__error_msg[0] = '\0';

	open();
}

Socket::Socket(const char *host, int port, int localPort)
	: __sock(-1), __is(NULL), __os(NULL), __error(false)
{
	__host         = host;
	__port         = port;
	__localPort    = localPort;
	__error_msg[0] = '\0';

	open();
}

Socket::~Socket()
{
	close();
	if (__is) {
		delete __is;
	}
	if (__os) {
		delete __os;
	}
}

istream &Socket::getInputStream()
{
	if (__is == NULL) {
		__is = new isocketstream(this);
	}
	return *__is;
}

ostream &Socket::getOutputStream()
{
	if (__os == NULL) {
		__os = new osocketstream(this);
	}
	return *__os;
}

void Socket::open()
{
	if (__sock == -1 && !__error) {

		sockaddr_in sin;
		memset(&sin, 0, sizeof(sin));

		unsigned long inaddr;
		hostent *host_info;

		if ((inaddr = inet_addr(__host.c_str())) != INADDR_NONE) {
			memcpy(&sin.sin_addr, &inaddr, sizeof(inaddr));
			sin.sin_family = AF_INET;
		} else if ((host_info = gethostbyname(__host.c_str())) != NULL) {
			memcpy(&sin.sin_addr, host_info->h_addr, host_info->h_length);
			sin.sin_family = host_info->h_addrtype;
		} else {
			sprintf(__error_msg, "gethostbyname failed. errno = %d", errno);
			__error = true;
			return;
		}

		if ((__sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			sprintf(__error_msg, "socket failed. errno = %d", errno);
			__error = true;
		} else {
			if (__localPort >= 0) {
				sockaddr_in cin;
				memset(&cin, 0, sizeof(cin));
				cin.sin_family = AF_INET;
				cin.sin_port   = htons(__localPort);
				if (::bind(__sock, (sockaddr *)&cin, sizeof(cin)) != 0) {
					sprintf(__error_msg, "bind failed. errno = %d", errno);
					::close(__sock);
					__sock = -1;
					__error = true;
				}
			}
			sin.sin_port = htons(__port);
			if (::connect(__sock, (sockaddr *)&(sin), sizeof(sin)) != 0) {
				sprintf(__error_msg, "connect failed. errno = %d", errno);
				::close(__sock);
				__sock = -1;
				__error = true;
			}
		}
	}
}

void Socket::close()
{
	if (__sock != -1) {
		::shutdown(__sock, 2);
		::close(__sock);
		__sock = -1;
	}
}

bool Socket::fail() const
{
	return __sock == -1 || __error;
}

bool Socket::good() const
{
	return !fail();
}

bool Socket::operator !() const
{
	return fail();
}

int Socket::read(char *buffer, int size, int flags)
{
	if (fail()) {
		size = 0;
	} else {
		size = ::recv(__sock, buffer, size, flags);
		if (size <= 0) {
			sprintf(__error_msg, "recv failed. errno = %d", errno);
			__error = true;
			close();
		}
	}
	return size;
}

int Socket::write(const char *buffer, int size, int flags)
{
	if (fail()) {
		size = 0;
	} else {
		size = ::send(__sock, buffer, size, flags);
		if (size <= 0) {
			sprintf(__error_msg, "send failed. errno = %d", errno);
			__error = true;
			close();
		}
	}
	return size;
}
