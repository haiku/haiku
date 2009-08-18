#include "PrintTransportAddOn.h"

// We don't support multiple instances of the same transport add-on
static BDataIO* gTransport = NULL;

extern "C" _EXPORT BDataIO *init_transport(BMessage *msg)
{
	if (msg == NULL || gTransport != NULL)
		return NULL;
		
	const char *spool_path = msg->FindString("printer_file");

	if (spool_path && *spool_path != '\0') {
		BDirectory printer(spool_path);

		if (printer.InitCheck() == B_OK) {
			gTransport = instantiate_transport(&printer, msg);
			return gTransport;
		};
	};
	
	return NULL;
}

extern "C" _EXPORT void exit_transport()
{
	if (gTransport) {
		delete gTransport;
		gTransport = NULL;	
	}
}

