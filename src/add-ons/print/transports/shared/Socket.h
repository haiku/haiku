// Sun, 18 Jun 2000
// Y.Takagi

#ifndef __Socket_H
#define __Socket_H

#include <iostream>

#include <string>

#if (!__MWERKS__)
using namespace std;
#else 
#define std
#endif

class Socket {
public:
	Socket(const char *host, int port);
	Socket(const char *host, int port, int localPort);
	~Socket();

	bool operator !() const;
	bool good() const;
	bool fail() const;

	void close();
	int read(char *buffer, int size, int flags = 0);
	int write(const char *buffer, int size, int flags = 0);

	istream &getInputStream();
	ostream &getOutputStream();

	int getPort() const;
	const char *getLastError() const;

private:
	Socket(const Socket &);
	Socket &operator = (const Socket &);
	void open();

	string  __host;
	int     __port;
	int     __localPort;
	int     __sock;
	istream *__is;
	ostream *__os;
	bool    __error;
	char    __error_msg[256];
};

inline int Socket::getPort() const
{
	return __port;
}

inline const char *Socket::getLastError() const
{
	return __error_msg;
}

#endif // __Socket_H
