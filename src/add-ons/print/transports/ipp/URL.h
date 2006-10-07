// Sun, 18 Jun 2000
// Y.Takagi

#ifndef __URL_H
#define __URL_H

#include <string>

#if (!__MWERKS__)
using namespace std;
#else 
#define std
#endif

class URL {
public:
	URL(const char *spec);
	URL(const char *protocol, const char *host, int port, const char *file);
	URL(const char *protocol, const char *host, const char *file);
//	URL(URL &, const char *spec);

	int getPort() const;
	const char *getProtocol() const;
	const char *getHost() const;
	const char *getFile() const;
	const char *getRef() const;

	URL(const URL &);
	URL &operator = (const URL &);
	bool operator == (const URL &);

protected:
//	void set(const char *protocol, const char *host, int port, const char *file, const char *ref);

private:
	string __protocol;
	string __host;
	string __file;
	string __ref;
	int __port;
};

inline int URL::getPort() const
{
	return __port;
}

inline const char *URL::getProtocol() const
{
	return __protocol.c_str();
}

inline const char *URL::getHost() const
{
	return __host.c_str();
}

inline const char *URL::getFile() const
{
	return __file.c_str();
}

inline const char *URL::getRef() const
{
	return __ref.c_str();
}

#endif	// __URL_H
