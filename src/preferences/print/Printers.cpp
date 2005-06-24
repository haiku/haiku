/*****************************************************************************/
// Printers Preference Application.
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2001-2003 OpenBeOS Project
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

#include "Printers.h"

#ifndef PRINTERSWINDOW_H
	#include "PrintersWindow.h"
#endif

int main()
{
	PrintersApp app;
	app.Run();	
	return 0;
}

PrintersApp::PrintersApp()
	: Inherited(PRINTERS_SIGNATURE)
{
}

void PrintersApp::ReadyToRun()
{
	PrintersWindow* win = new PrintersWindow(BRect(78.0, 71.0, 561.0, 409.0));
	win->Show();
}

void PrintersApp::MessageReceived(BMessage* msg) {
	if (msg->what == B_PRINTER_CHANGED) {
			// broadcast message
		BWindow* w;
		for (int32 i = 0; (w = WindowAt(i)) != NULL; i ++) {
			BMessenger msgr(NULL, w);
			msgr.SendMessage(B_PRINTER_CHANGED);
		}
	} else {
		BApplication::MessageReceived(msg);
	}
}
