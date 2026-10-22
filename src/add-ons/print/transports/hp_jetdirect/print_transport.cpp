/*
 * Copyright 2001-2026, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *      Philippe Houdoin
 */

#include <Message.h>
#include <Directory.h>
#include <SupportKit.h>

#include "HPJetDirectTransport.h"

static BDataIO* sTransport = NULL;


// Implementation of transport add-on interface


extern "C" _EXPORT BDataIO*
init_transport(BMessage* msg)
{
	if (msg == NULL)
		return NULL;

	if (sTransport != NULL)
		return NULL;

	const char* printerName = msg->FindString("printer_file");

	if (printerName != NULL && *printerName != '\0') {
		BDirectory printer(printerName);

		if (printer.InitCheck() == B_OK) {
			HPJetDirectPort * transport = new HPJetDirectPort(&printer, msg);

			if (transport->InitCheck() == B_OK) {
				sTransport = transport;
				if (msg != NULL)
					msg->what = 'okok';
				return sTransport;
			};

			delete transport;
		};
	};
	return NULL;
}


extern "C" _EXPORT void
exit_transport()
{
	delete sTransport;
	sTransport = NULL;
}
