#include "PrintTransportAddOn.h"

static BDataIO* gTransport = NULL;

extern "C" _EXPORT BDataIO *init_transport(BMessage *msg) {
	if (msg != NULL && gTransport == NULL) {
		const char* printer_name = msg->FindString("printer_name");
		if (printer_name && *printer_name != '\0') {
			BDirectory printer(printer_name);
			if (printer.InitCheck() == B_OK) {
				gTransport = instanciate_transport(&printer, msg);
				return gTransport;
			}
		}
	}
	return NULL;
}

extern "C" _EXPORT void exit_transport() {
	if (gTransport) {
		delete gTransport; gTransport = NULL;	
	}
}

