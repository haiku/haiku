// Sun, 18 Jun 2000
// Y.Takagi

#ifndef __IppTransport_H
#define __IppTransport_H

#include <DataIO.h>
#include <Message.h>
#include <fstream>
#include <string>

#if (!__MWERKS__)
using namespace std;
#else 
#define std
#endif

class IppTransport : public BDataIO {
public:
	IppTransport(BMessage *msg);
	virtual ~IppTransport();
	virtual ssize_t Read(void *buffer, size_t size);
	virtual ssize_t Write(const void *buffer, size_t size);

	bool operator !() const;
	bool fail() const;

private:
	char    __url[256];
	char    __user[256];
	char    __file[256];
	int32   __jobid;
	bool    __error;
	fstream __fs;
};

inline bool IppTransport::fail() const
{
	return __error;
}

inline bool IppTransport::operator !() const
{
	return fail();
}

#endif	// __IppTransport_H
