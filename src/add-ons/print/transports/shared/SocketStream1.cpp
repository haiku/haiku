// Sun, 18 Jun 2000
// Y.Takagi

#ifdef WIN32
#include <winsock.h>

#ifdef _DEBUG
#include <iostream>
#include <fstream>
#endif

#include "Socket.h"
#include "SocketStream.h"

namespace std {

/*-----------------------------------------------------------------*/

socketstreambuf::socketstreambuf(Socket *sock, streamsize n)
	: basic_streambuf<char_type, traits>(), __alsize(n), __sock(sock), __pu(NULL), __po(NULL)
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


socketstreambuf::int_type socketstreambuf::underflow()
{
//	cout << "***** underflow" << endl;

	if (__pu == NULL) {
		__pu = new char[__alsize];
	}

	int bytes = __sock->read(__pu, __alsize);
	if (bytes > 0) {
#ifdef _DEBUG
		ofstream ofs("recv.log", ios::binary | ios::app);
		ofs.write(__pu, bytes);
#endif
		setg(__pu, __pu, __pu + bytes);
		return traits::to_int_type(*gptr());
	}

	return traits::eof();
}

socketstreambuf::int_type socketstreambuf::overflow(socketstreambuf::int_type c)
{
//	cout << "***** overflow" << endl;

	if (__po == NULL) {
		__po = new char[__alsize];
		setp(__po, __po + __alsize);
	} else if (sync() != 0) {
		return traits::eof();
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

/*-----------------------------------------------------------------*/

isocketstream::isocketstream(Socket *sock, streamsize n)
	:  basic_istream<char_type, traits>(&ssb_), ssb_(sock, n)
{
}

isocketstream::~isocketstream()
{
}

/*-----------------------------------------------------------------*/

osocketstream::osocketstream(Socket *sock, streamsize n)
	:  basic_ostream<char_type, traits>(&ssb_), ssb_(sock, n)
{
}

osocketstream::~osocketstream()
{
	flush();
}

/*-----------------------------------------------------------------*/

}

#endif	// WIN32
