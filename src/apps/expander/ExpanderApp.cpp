/*****************************************************************************/
// Expander
// Written by Jérôme Duval
//
// ExpanderApp.cpp
//
//
// Copyright (c) 2004 OpenBeOS Project
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

#include <Alert.h>
#include "ExpanderApp.h"
#include "ExpanderWindow.h"

const char *APP_SIG		= "application/x-vnd.obos-Expander";

int main(int, char **)
{
	ExpanderApp theApp;
	theApp.Run();
	return 0;
}

ExpanderApp::ExpanderApp()
	: BApplication(APP_SIG)
{
	BPoint windowPosition = fSettings.Message().FindPoint("window_position");
	BRect windowFrame(0,0,450,120);
	windowFrame.OffsetBy(windowPosition);
	BMessage settings(fSettings.Message());
	fWindow = new ExpanderWindow(windowFrame, NULL, &settings);
}

void
ExpanderApp::AboutRequested()
{
	BAlert *alert = new BAlert("about", "Expand-O-Matic\n"
		"\twritten by Jérôme Duval\n"
		"\tCopyright 2004, OpenBeOS.\n\n"
		"original Be version by \n"
		"Dominic, Hiroshi, Peter, Pavel and Robert\n", "Ok");
	BTextView *view = alert->TextView();
	BFont font;

	view->SetStylable(true);

	view->GetFont(&font);
	font.SetSize(18);
	font.SetFace(B_BOLD_FACE); 			
	view->SetFontAndColor(0, 14, &font);

	alert->Go();

}


void
ExpanderApp::ReadyToRun()
{
	
}


void
ExpanderApp::ArgvReceived(int32 argc, char **argv)
{
	BMessage *msg = NULL;
	for (int32 i = 1; i < argc; i++) {
		entry_ref ref;
		status_t err = get_ref_for_path(argv[i], &ref);
		if (err == B_OK) {
			if (!msg) {
				msg = new BMessage;
				msg->what = B_REFS_RECEIVED;
			}
			msg->AddRef("refs", &ref);
		}
	}
	if (msg)
		RefsReceived(msg);
}


void
ExpanderApp::RefsReceived(BMessage *msg)
{   	
   	BMessenger messenger(fWindow);
   	msg->AddBool("fromApp", true);
   	messenger.SendMessage(msg);
}


void 
ExpanderApp::UpdateSettingsFrom(BMessage *message)
{
	fSettings.UpdateFrom(message);
}
