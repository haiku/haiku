// Sun, 18 Jun 2000
// Y.Takagi

#ifndef __SocketStream1_H
#define __SocketStream1_H

#include <istream>
#include <ostream>
#include <streambuf>

class Socket;

namespace std {

class socketstreambuf : public basic_streambuf< char, char_traits<char> > {
public:
	typedef char				char_type;
	typedef char_traits<char>	traits;
	typedef traits::int_type	int_type;
	typedef traits::pos_type	pos_type;
	typedef traits::off_type	off_type;

	explicit socketstreambuf(Socket *sock, streamsize n);
	virtual ~socketstreambuf();

protected:
	virtual int_type underflow();
	virtual int_type overflow(int_type c = traits::eof());
	virtual int sync();

private:
	Socket *__sock;
	streamsize __alsize;
	char *__pu;
	char *__po;
};

class isocketstream : public basic_istream< char, char_traits<char> > {
public:
	typedef char				char_type;
	typedef char_traits<char>	traits;
	typedef traits::int_type	int_type;
	typedef traits::pos_type	pos_type;
	typedef traits::off_type	off_type;

	explicit isocketstream(Socket *sock, streamsize n = 4096);
	virtual ~isocketstream();

	socketstreambuf *rdbuf() const
	{
		return (socketstreambuf *)basic_ios<char_type, traits>::rdbuf();
	}

private:
	socketstreambuf ssb_;
};


class osocketstream : public basic_ostream< char, char_traits<char> > {
public:
	typedef char				char_type;
	typedef char_traits<char>	traits;
	typedef traits::int_type	int_type;
	typedef traits::pos_type	pos_type;
	typedef traits::off_type	off_type;

	explicit osocketstream(Socket *sock, streamsize n = 4096);
	virtual ~osocketstream();

	socketstreambuf *rdbuf() const
	{
		return (socketstreambuf *)basic_ios<char_type, traits>::rdbuf();
	}

private:
	socketstreambuf ssb_;
};

}

#endif	// __SocketStream1_H
