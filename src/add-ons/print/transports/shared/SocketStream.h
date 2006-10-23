// Sun, 18 Jun 2000
// Y.Takagi

#ifndef __SocketStream_H
#define __SocketStream_H

#include <iostream>

class Socket;

class socketstreambuf : public streambuf {
public:
	explicit socketstreambuf(Socket *sock, streamsize n);
	~socketstreambuf();

protected:
	virtual int underflow();
	virtual int overflow(int);
	virtual int sync();

private:
	Socket *__sock;
	streamsize __alsize;
	char *__pu;
	char *__po;
};

class socketstreambase : public virtual ios {
public:
	socketstreambuf *rdbuf();

protected:
	socketstreambase(Socket *sock, streamsize n);
	~socketstreambase() {}

private:
	socketstreambuf buf;
};

inline socketstreambuf *socketstreambase::rdbuf()
{
	return &this->buf;
}

class isocketstream : public socketstreambase, public istream {
public:
	explicit isocketstream(Socket *sock, streamsize n = 4096);
	virtual ~isocketstream();
};


class osocketstream : public socketstreambase, public ostream {
public:
	explicit osocketstream(Socket *sock, streamsize n = 4096);
	virtual ~osocketstream();
};

#endif	// __SocketStream_H
