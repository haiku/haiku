/*****************************************************************************/
// print_server Background Application.
//
// Version: 1.0.0d1
//
// The print_server manages the communication between applications and the
// printer and transport drivers.
//
//
//
// TODO:
//	 Handle spooled jobs at startup
//	 Handle printer status (asked to print a spooled job but printer busy)
//   
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2001 OpenBeOS Project
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

	// BeOS API
#include <PrintJob.h>
#include <Alert.h>

void PrintServerApp::Handle_BeOSR5_Message(BMessage* msg)
{
	switch(msg->what) {
			// Get currently selected printer
		case PSRV_GET_ACTIVE_PRINTER: {
				BMessage reply('okok');
				reply.AddString("printer_name", fDefaultPrinter ? fDefaultPrinter->Name() : "");
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

			// Create a new printer
		case PSRV_MAKE_PRINTER: {
				BString driverName, transportName, transportPath;
				BString printerName, connection;
				
				if (msg->FindString("driver", &driverName) == B_OK &&
					msg->FindString("transport", &transportName) == B_OK &&
					msg->FindString("transport path", &transportPath) == B_OK &&
					msg->FindString("printer name", &printerName) == B_OK &&
					msg->FindString("connection", &connection) == B_OK) {
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

			// Handle showing the page config dialog
		case PSRV_SHOW_PAGE_SETUP: {
				if (fDefaultPrinter != NULL) {
					BMessage reply(*msg);
					if (fDefaultPrinter->ConfigurePage(reply) == B_OK) {
						msg->SendReply(&reply);
					}
				}
				else {
						// If no default printer, give user choice of aborting or setting up a printer
					BAlert* alert = new BAlert("Info", "Hang on there! You don't have any printers set up!\nYou'll need to do that before trying to print\n\nWould you like to set up a printer now?", "No thanks", "Sure!");
					if (alert->Go() == 1) {
						run_add_printer_panel();
					}
					
						// Always stop dialog flow	
					BMessage reply('stop');
					msg->SendReply(&reply);
				}
			}
			break;

			// Handle showing the print config dialog
		case PSRV_SHOW_PRINT_SETUP: {
				if (fDefaultPrinter != NULL) {
					BMessage reply(*msg);
					if (fDefaultPrinter->ConfigureJob(reply) == B_OK) {
						msg->SendReply(&reply);
					}
				}
				else {
					BMessage reply('stop');
					msg->SendReply(&reply);
				}
			}
			break;

			// Tell printer addon to print a spooled job
		case PSRV_PRINT_SPOOLED_JOB: {
				BString name, jobpath;
				if (msg->FindString("Spool File",&jobpath) == B_OK &&
					msg->FindString("JobName", &name) == B_OK) {
						// Handle the spooled job
					HandleSpooledJob(name.String(), jobpath.String());
				}
			}
			break;
	}
}
