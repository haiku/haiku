// Sun, 18 Jun 2000
// Y.Takagi

#include "LprTransport.h"
#include "DbgMsg.h"


static LprTransport *gTransport = NULL;


extern "C" void
exit_transport()
{
	DBGMSG(("> exit_transport\n"));
	delete gTransport;
	gTransport = NULL;
	DBGMSG(("< exit_transport\n"));
}


extern "C" BDataIO *
init_transport(BMessage *msg)
{
	DBGMSG(("> init_transport\n"));

	gTransport = new LprTransport(msg);

	if (gTransport->fail()) {
		exit_transport();
	}

	if (msg)
		msg->what = 'okok';

	DBGMSG(("< init_transport\n"));
	return gTransport;
}
