// Sun, 18 Jun 2000
// Y.Takagi

#include "IppTransport.h"
#include "DbgMsg.h"

IppTransport *transport = NULL;

extern "C" _EXPORT void exit_transport()
{
	DBGMSG(("> exit_transport\n"));
	if (transport) {
		delete transport;
		transport = NULL;
	}
	DBGMSG(("< exit_transport\n"));
}

extern "C" _EXPORT BDataIO *init_transport(BMessage *msg)
{
	DBGMSG(("> init_transport\n"));

	transport = new IppTransport(msg);

	if (transport->fail()) {
		exit_transport();
	}

	DBGMSG(("< init_transport\n"));
	return transport;
}
