// Sun, 18 Jun 2000
// Y.Takagi

#ifndef __LprTransport_H
#define __LprTransport_H

#include <DataIO.h>
#include <Message.h>
#include <fstream>
#include <string>


using namespace std;


class LprTransport : public BDataIO {
public:
					LprTransport(BMessage *msg);
	virtual 		~LprTransport();
	virtual ssize_t	Read(void *buffer, size_t size);
	virtual ssize_t Write(const void *buffer, size_t size);

			bool 	operator!() const;
			bool 	fail() const;

private:
	void			_SendFile();

	char    fServer[256];
	char    fQueue[256];
	char    fFile[256];
	char    fUser[256];
	int32   fJobId;
	fstream fStream;
	bool    fError;
};


inline bool
LprTransport::fail() const
{
	return fError;
}


inline bool
LprTransport::operator!() const
{
	return fail();
}

#endif	// __LprTransport_H
