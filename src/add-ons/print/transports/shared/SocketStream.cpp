// Sun, 18 Jun 2000
// Y.Takagi

#if defined(__HAIKU__) || defined(HAIKU_TARGET_PLATFORM_BONE)
#	include <sys/socket.h>
#else
#	include <net/socket.h>
#endif

#ifdef _DEBUG
#include <iostream>
#include <fstream>
#endif

#include "Socket.h"
#include "SocketStream.h"

/*-----------------------------------------------------------------*/

socketstreambuf::socketstreambuf(Socket *sock, streamsize n)
	: streambuf(), __sock(sock), __alsize(n), __pu(NULL), __po(NULL)
{
	setg(0, 0, 0);
	setp(0, 0);
}

socketstreambuf::~socketstreambuf()
{
	if (__pu)
		delete [] __pu;
	if (__po)
		delete [] __po;
}

int socketstreambuf::underflow()
{
//	cout << "***** underflow" << endl;

	int bytes;

	if (__pu == NULL) {
		__pu = new char[__alsize];
	}

	bytes = __sock->read(__pu, __alsize);
	if (bytes > 0) {
#ifdef _DEBUG
		ofstream ofs("recv.log", ios::binary | ios::app);
		ofs.write(__pu, bytes);
#endif
		setg(__pu, __pu, __pu + bytes);
		return *gptr();
	}

	return EOF;
}

int socketstreambuf::overflow(int c)
{
//	cout << "***** overflow" << endl;

	if (__po == NULL) {
		__po = new char[__alsize];
		setp(__po, __po + __alsize);
	} else if (sync() != 0) {
		return EOF;
	}
	return sputc(c);
}

int socketstreambuf::sync()
{
//	cout << "***** sync" << endl;

	if (__po) {
		int length = pptr() - pbase();
		if (length > 0) {
			const char *buffer = pbase();
			int bytes;
			while (length > 0) {
				bytes = __sock->write(buffer, length);
				if (bytes <= 0) {
					return EOF;
				}
#ifdef _DEBUG
				ofstream ofs("send.log", ios::binary | ios::app);
				ofs.write(buffer, bytes);
#endif
				length -= bytes;
				buffer += bytes;
			}
			pbump(pbase() - pptr());
		}
	}

	return 0;
}

/* -------------------------------------------------------------- */

socketstreambase::socketstreambase(Socket *sock, streamsize n)
	: buf(sock, n)
{
	ios::init(&this->buf);
}

/*-----------------------------------------------------------------*/

isocketstream::isocketstream(Socket *sock, streamsize n)
	: socketstreambase(sock, n), istream(socketstreambase::rdbuf())
{
}

isocketstream::~isocketstream()
{
}

/*-----------------------------------------------------------------*/

osocketstream::osocketstream(Socket *sock, streamsize n)
	: socketstreambase(sock, n), ostream(socketstreambase::rdbuf())
{
}

osocketstream::~osocketstream()
{
	flush();
}
