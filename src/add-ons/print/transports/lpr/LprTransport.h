// Sun, 18 Jun 2000
// Y.Takagi

#ifndef __LprTransport_H
#define __LprTransport_H

#include <DataIO.h>
#include <Message.h>
#include <fstream>
#include <string>

#if (!__MWERKS__ || defined(WIN32))
using namespace std;
#else 
#define std
#endif

class LprTransport : public BDataIO {
public:
	LprTransport(BMessage *msg);
	virtual ~LprTransport();
	virtual ssize_t Read(void *buffer, size_t size);
	virtual ssize_t Write(const void *buffer, size_t size);

	bool operator !() const;
	bool fail() const;

private:
	char    __server[256];
	char    __queue[256];
	char    __file[256];
	char    __user[256];
	int32   __jobid;
	fstream __fs;
	bool    __error;
};

inline bool LprTransport::fail() const
{
	return __error;
}

inline bool LprTransport::operator !() const
{
	return fail();
}

#endif	// __LprTransport_H
