#include "beserved_transport.h"


BeServedTransport *transport = NULL;

extern "C" _EXPORT void exit_transport()
{
	if (transport)
	{
		delete transport;
		transport = NULL;
	}
}

extern "C" _EXPORT BDataIO *init_transport(BMessage *msg)
{
	transport = new BeServedTransport(msg);

	if (transport->fail())
		exit_transport();

	return transport;
}
