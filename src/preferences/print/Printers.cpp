/*
 * Copyright 2001-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */


#include "Printers.h"

#include <Locale.h>

#include "pr_server.h"
#include "Messages.h"
#include "PrintersWindow.h"


int
main()
{
	PrintersApp app;
	app.Run();	
	return 0;
}


PrintersApp::PrintersApp()
	: Inherited(PRINTERS_SIGNATURE)
{
}


void
PrintersApp::ReadyToRun()
{
	PrintersWindow* win = new PrintersWindow(BRect(78, 71, 561, 409));
	win->Show();
}


void
PrintersApp::MessageReceived(BMessage* msg)
{
	if (msg->what == B_PRINTER_CHANGED || msg->what == PRINTERS_ADD_PRINTER) {
			// broadcast message
		uint32 what = msg->what;
		if (what == PRINTERS_ADD_PRINTER)
			what = kMsgAddPrinter;

		BWindow* w;
		for (int32 i = 0; (w = WindowAt(i)) != NULL; i++) {
			BMessenger msgr(NULL, w);
			msgr.SendMessage(what);
		}
	} else {
		BApplication::MessageReceived(msg);
	}
}


void
PrintersApp::ArgvReceived(int32 argc, char** argv)
{
	for (int i = 1; i < argc; i++) {
		// TODO: show a pre-filled add printer dialog here
	}
}

