#ifndef _beserved_transport_h_
#define _beserved_transport_h_

#include <Directory.h>
#include <DataIO.h>
#include <Message.h>
#include <fstream>
#include <string>

#include "betalk.h"


#if (!__MWERKS__ || defined(WIN32))
using namespace std;
#else 
#define std
#endif

class BeServedTransport : public BDataIO
{
public:
	BeServedTransport(BMessage *msg);
	virtual ~BeServedTransport();
	virtual ssize_t Read(void *buffer, size_t size);
	virtual ssize_t Write(const void *buffer, size_t size);

	bool getNewCredentials();

	bool operator !() const;
	bool fail() const;

private:
	char server[256];
	char printerName[256];
	char *serverJobId;

	BDirectory dir;
	char file[B_PATH_NAME_LENGTH];
	bt_rpcinfo info;

	char user[MAX_USERNAME_LENGTH + 1];
	char password[BT_AUTH_TOKEN_LENGTH * 2 + 1];

	int32 jobId;
	fstream fs;
	bool error;
};

inline bool BeServedTransport::fail() const
{
	return error;
}

inline bool BeServedTransport::operator !() const
{
	return fail();
}

#endif
