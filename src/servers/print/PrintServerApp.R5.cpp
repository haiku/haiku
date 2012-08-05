/*
 * Copyright 2001-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ithamar R. Adema
 *		Michael Pfeiffer
 */
#include "PrintServerApp.h"

#include "pr_server.h"
#include "Printer.h"
#include "ConfigWindow.h"

	// BeOS API
#include <Alert.h>
#include <Autolock.h>
#include <Catalog.h>
#include <Locale.h>
#include <PrintJob.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PrintServerApp"


struct AsyncThreadParams {
	PrintServerApp* app;
	Printer* printer;
	BMessage* message;

	AsyncThreadParams(PrintServerApp* app, Printer* p, BMessage* m)
		: app(app)
		, printer(p)
		, message(m)
	{
		app->Acquire();
		if (printer) printer->Acquire();
	}

	~AsyncThreadParams() {
		if (printer) printer->Release();
		delete message;
		app->Release();
	}

	BMessage* AcquireMessage() {
		BMessage* m = message; message = NULL; return m;
	}
};


status_t 
PrintServerApp::async_thread(void* data)
{
	AsyncThreadParams* p = (AsyncThreadParams*)data;

	Printer* printer = p->printer;
	BMessage* msg = p->AcquireMessage();
	{
		AutoReply sender(msg, 'stop');
		switch (msg->what) {
			// Handle showing the config dialog
			case PSRV_SHOW_PAGE_SETUP: {
			case PSRV_SHOW_PRINT_SETUP:
				if (printer) {
					if (p->app->fUseConfigWindow) {
						config_setup_kind kind = kJobSetup;
						if (msg->what == PSRV_SHOW_PAGE_SETUP)
							kind = kPageSetup;
						ConfigWindow* w = new ConfigWindow(kind, printer, msg,
							&sender);
						w->Go();
					} else {
						BMessage reply(*msg);
						status_t status = B_ERROR;
						if (msg->what == PSRV_SHOW_PAGE_SETUP)
							status = printer->ConfigurePage(reply);
						else
							status = printer->ConfigureJob(reply);

						if (status == B_OK)
							sender.SetReply(&reply);
					}
				} else {
					// If no default printer is set, give user
					// choice of aborting or setting up a printer
					int32 count = Printer::CountPrinters();
					BString alertText(
						B_TRANSLATE("There are no printers set up."));
					if (count > 0)
						alertText.SetTo(B_TRANSLATE(
							"There is no default printer set up."));

					alertText.Append(" ");
					alertText.Append(
						B_TRANSLATE("Would you like to set one up now?"));
					BAlert* alert = new BAlert("Info", alertText.String(),
						B_TRANSLATE("No"), B_TRANSLATE("Yes"));
					alert->SetShortcut(0, B_ESCAPE);
					if (alert->Go() == 1) {
						if (count == 0)
							run_add_printer_panel();
						else
							run_select_printer_panel();
					}
				}
			}	break;

			// Retrieve default configuration message from printer add-on
			case PSRV_GET_DEFAULT_SETTINGS: {
				if (printer) {
					BMessage reply;
					if (printer->GetDefaultSettings(reply) == B_OK) {
						sender.SetReply(&reply);
						break;
					}
				}
			}	break;

			// Create a new printer
			case PSRV_MAKE_PRINTER: {
				BString driverName;
				BString printerName;
				BString transportName;
				BString transportPath;
				if (msg->FindString("driver", &driverName) == B_OK
					&& msg->FindString("transport", &transportName) == B_OK
					&& msg->FindString("transport path", &transportPath) == B_OK
					&& msg->FindString("printer name", &printerName) == B_OK) {
					BString connection;
					if (msg->FindString("connection", &connection) != B_OK)
						connection = "Local";

					// then create the actual printer
					if (p->app->CreatePrinter(printerName.String(),
						driverName.String(), connection.String(),
						transportName.String(),
						transportPath.String()) == B_OK) {
						// If printer was created ok,
						// ask if it needs to be the default
						BString text(B_TRANSLATE("Would you like to make @ "
							"the default printer?"));
						text.ReplaceFirst("@", printerName.String());
						BAlert* alert = new BAlert("", text.String(),
							B_TRANSLATE("No"), B_TRANSLATE("Yes"));
						alert->SetShortcut(0, B_ESCAPE);
						if (alert->Go() == 1)
							p->app->SelectPrinter(printerName.String());
					}
				}
			}	break;
		}
	}
	delete p;
	return B_OK;
}


// Async. processing of received message
void 
PrintServerApp::AsyncHandleMessage(BMessage* msg)
{
	AsyncThreadParams* data = new AsyncThreadParams(this, fDefaultPrinter, msg);

	thread_id tid = spawn_thread(async_thread, "async", B_NORMAL_PRIORITY,
		(void*)data);

	if (tid > 0) {
		resume_thread(tid);
	} else {
		delete data;
	}
}


void 
PrintServerApp::Handle_BeOSR5_Message(BMessage* msg)
{
	switch(msg->what) {
			// Get currently selected printer
		case PSRV_GET_ACTIVE_PRINTER: {
				BMessage reply('okok');
				BString printerName;
				if (fDefaultPrinter)
					printerName = fDefaultPrinter->Name();
				BString mime;
				if (fUseConfigWindow && MimeTypeForSender(msg, mime)) {
					BAutolock lock(gLock);
					if (lock.IsLocked()) {
							// override with printer for application
						PrinterSettings* p = fSettings->FindPrinterSettings(
							mime.String());
						if (p)
							printerName = p->GetPrinter();
					}
				}
				reply.AddString("printer_name", printerName);
				// BeOS knows not if color or not, so always color
				reply.AddInt32("color", BPrintJob::B_COLOR_PRINTER);
				msg->SendReply(&reply);
			}
			break;

			//make printer active (currently always quietly :))
		case PSRV_MAKE_PRINTER_ACTIVE_QUIETLY:
			//make printer active quietly
		case PSRV_MAKE_PRINTER_ACTIVE: {
				BString newActivePrinter;
				if (msg->FindString("printer",&newActivePrinter) == B_OK) {
					SelectPrinter(newActivePrinter.String());
				}
			}
			break;

		case PSRV_SHOW_PAGE_SETUP:
		case PSRV_SHOW_PRINT_SETUP:
		case PSRV_GET_DEFAULT_SETTINGS:
		case PSRV_MAKE_PRINTER:
			AsyncHandleMessage(DetachCurrentMessage());
			break;

			// Tell printer addon to print a spooled job
		case PSRV_PRINT_SPOOLED_JOB:
			HandleSpooledJobs();
			break;
	}
}

