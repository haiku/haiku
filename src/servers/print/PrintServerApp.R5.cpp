/*****************************************************************************/
// print_server Background Application.
//
// Version: 1.0.0d1
//
// The print_server manages the communication between applications and the
// printer and transport drivers.
//
// Authors
//   Ithamar R. Adema
//   Michael Pfeiffer
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2001, 2002 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#include "PrintServerApp.h"

#include "pr_server.h"
#include "Printer.h"
#include "ConfigWindow.h"

	// BeOS API
#include <Alert.h>
#include <Autolock.h>
#include <PrintJob.h>

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

status_t PrintServerApp::async_thread(void* data)
{
	AsyncThreadParams* p = (AsyncThreadParams*)data;
	
	Printer* printer = p->printer;
	BMessage* msg = p->AcquireMessage();

	{
		AutoReply sender(msg, 'stop');
		switch (msg->what) {
				// Handle showing the page config dialog
			case PSRV_SHOW_PAGE_SETUP: {
					BMessage reply(*msg);
					if (printer != NULL) {
						if (p->app->fUseConfigWindow) {
							ConfigWindow* w = new ConfigWindow(kPageSetup, printer, msg, &sender);
							w->Go();
						} else if (printer->ConfigurePage(reply) == B_OK) {
							sender.SetReply(&reply);
						}
					} else {
							// If no default printer, give user choice of aborting or setting up a printer
						BAlert* alert = new BAlert("Info", "Hang on there! You don't have any printers set up!\nYou'll need to do that before trying to print\n\nWould you like to set up a printer now?", "No thanks", "Sure!");
						if (alert->Go() == 1) {
							run_add_printer_panel();
						}						
					}
				}
				break;
	
				// Handle showing the print config dialog
			case PSRV_SHOW_PRINT_SETUP: {
					if (printer == NULL) break;
					if (p->app->fUseConfigWindow) {
						ConfigWindow* w = new ConfigWindow(kJobSetup, printer, msg, &sender);
						w->Go();
					} else {
						BMessage reply(*msg);
						if (printer->ConfigureJob(reply) == B_OK) {
							sender.SetReply(&reply);
						}
					}
				}
				break;
	
				// Retrieve default configuration message from printer add-on
			case PSRV_GET_DEFAULT_SETTINGS:
				if (printer != NULL) {
					BMessage reply;
					if (printer->GetDefaultSettings(reply) == B_OK) {
						sender.SetReply(&reply);
						break;
					}
				}
				break;

				// Create a new printer
			case PSRV_MAKE_PRINTER: {
					BString driverName, transportName, transportPath;
					BString printerName, connection;
					
					if (msg->FindString("driver", &driverName) == B_OK &&
						msg->FindString("transport", &transportName) == B_OK &&
						msg->FindString("transport path", &transportPath) == B_OK &&
						msg->FindString("printer name", &printerName) == B_OK
						) {
	
						if (msg->FindString("connection", &connection) != B_OK)
							connection = "Local";
	
							// then create the actual printer
						if (p->app->CreatePrinter(printerName.String(), driverName.String(),
							connection.String(),
							transportName.String(), transportPath.String()) == B_OK) {				
								// If printer was created ok, ask if it needs to be the default
							char buffer[256];
							::sprintf(buffer, "Would you like to make %s the default\nprinter?",
								printerName.String());
		
							BAlert* alert = new BAlert("", buffer, "No", "Yes");
							if (alert->Go() == 1) {
								p->app->SelectPrinter(printerName.String());
							}
						}
					}
				}
				break;
		}	
	}

	delete p;

	return B_OK;
}


// Async. processing of received message
void PrintServerApp::AsyncHandleMessage(BMessage* msg)
{
	AsyncThreadParams* data = new AsyncThreadParams(this, fDefaultPrinter, msg);

	thread_id tid = spawn_thread(async_thread, "async", B_NORMAL_PRIORITY, (void*)data);

	if (tid > 0) {
		resume_thread(tid);	
	} else {
		delete data;
	}
}

void PrintServerApp::Handle_BeOSR5_Message(BMessage* msg)
{
	switch(msg->what) {
			// Get currently selected printer
		case PSRV_GET_ACTIVE_PRINTER: {
				BMessage reply('okok');
				BString printerName = fDefaultPrinter ? fDefaultPrinter->Name() : "";
				BString mime;
				if (fUseConfigWindow && MimeTypeForSender(msg, mime)) {
					BAutolock lock(gLock);
					if (lock.IsLocked()) {
							// override with printer for application
						PrinterSettings* p = fSettings->FindPrinterSettings(mime.String());
						if (p) printerName = p->GetPrinter();
					}
				}
				reply.AddString("printer_name", printerName);
				reply.AddInt32("color", BPrintJob::B_COLOR_PRINTER);	// BeOS knows not if color or not, so always color
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

#if 0
			// Create a new printer
		case PSRV_MAKE_PRINTER: {
				BString driverName, transportName, transportPath;
				BString printerName, connection;
				
				if (msg->FindString("driver", &driverName) == B_OK &&
					msg->FindString("transport", &transportName) == B_OK &&
					msg->FindString("transport path", &transportPath) == B_OK &&
					msg->FindString("printer name", &printerName) == B_OK
					) {

					if (msg->FindString("connection", &connection) != B_OK)
						connection = "Local";

						// then create the actual printer
					if (CreatePrinter(printerName.String(), driverName.String(),
						connection.String(),
						transportName.String(), transportPath.String()) == B_OK) {				
							// If printer was created ok, ask if it needs to be the default
						char buffer[256];
						::sprintf(buffer, "Would you like to make %s the default\nprinter?",
							printerName.String());
	
						BAlert* alert = new BAlert("", buffer, "No", "Yes");
						if (alert->Go() == 1) {
							SelectPrinter(printerName.String());
						}
					}
				}
			}
			break;
#endif

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
