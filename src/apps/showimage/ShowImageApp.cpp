/*****************************************************************************/
// ShowImageApp
// Written by Fernando Francisco de Oliveira, Michael Wilber, Michael Pfeiffer
//
// ShowImageApp.cpp
//
//
// Copyright (c) 2003 OpenBeOS Project
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

#include <stdio.h>
#include <Alert.h>
#include <FilePanel.h>

#include "ShowImageConstants.h"
#include "ShowImageApp.h"
#include "ShowImageWindow.h"

int main(int, char **)
{
	ShowImageApp theApp;
	theApp.Run();
	return 0;
}

#define WINDOWS_TO_IGNORE 1

ShowImageApp::ShowImageApp()
	: BApplication(APP_SIG)
{
	fbPulseStarted = false;
	fpOpenPanel = new BFilePanel(B_OPEN_PANEL);
}

void
ShowImageApp::AboutRequested()
{
	BAlert* pAlert = new BAlert("About ShowImage",
		"OBOS ShowImage\n\nby Fernando F. Oliveira, Michael Wilber and Michael Pfeiffer", "OK");
	pAlert->Go();
}

void
ShowImageApp::ReadyToRun()
{
	if (CountWindows() == WINDOWS_TO_IGNORE)
		fpOpenPanel->Show();
	else
		// If image windows are already open
		// (paths supplied on the command line)
		// start checking the number of open windows
		StartPulse();
}

void
ShowImageApp::StartPulse()
{
	if (!fbPulseStarted) {
		// Tell the app to begin checking
		// for the number of open windows
		fbPulseStarted = true;
		SetPulseRate(250000);
			// Set pulse to every 1/4 second
	}
}

void
ShowImageApp::Pulse()
{
	if (!IsLaunching() && CountWindows() <= WINDOWS_TO_IGNORE)
		// If the application is not launching and
		// all windows are closed except for the file open panel,
		// quit the application
		PostMessage(B_QUIT_REQUESTED);
}

void
ShowImageApp::ArgvReceived(int32 argc, char **argv)
{
	BMessage *pmsg = NULL;
	for (int32 i = 1; i < argc; i++) {
		entry_ref ref;
		status_t err = get_ref_for_path(argv[i], &ref);
		if (err == B_OK) {
			if (!pmsg) {
				pmsg = new BMessage;
				pmsg->what = B_REFS_RECEIVED;
			}
			pmsg->AddRef("refs", &ref);
		}
	}
	if (pmsg)
		RefsReceived(pmsg);
}

void
ShowImageApp::MessageReceived(BMessage *pmsg)
{
	switch (pmsg->what) {
		case MSG_FILE_OPEN:
			fpOpenPanel->Show();
			break;
			
		case MSG_WINDOW_QUIT:
			break;
			
		case B_CANCEL:
			// File open panel was closed,
			// start checking count of open windows
			StartPulse();
			break;
		
		case MSG_UPDATE_RECENT_DOCUMENTS:
			BroadcastToWindows(MSG_UPDATE_RECENT_DOCUMENTS);
			break;

		default:
			BApplication::MessageReceived(pmsg);
			break;
	}
}

void
ShowImageApp::RefsReceived(BMessage *pmsg)
{
	uint32 type;
	int32 count;
	status_t ret = pmsg->GetInfo("refs", &type, &count);
	if (ret != B_OK || type != B_REF_TYPE)
		return;

	entry_ref ref;
	for (int32 i = 0; i < count; i++) {
   		if (pmsg->FindRef("refs", i, &ref) == B_OK)
   			Open(&ref);
   	}
}

void
ShowImageApp::Open(const entry_ref *pref)
{
	new ShowImageWindow(pref);
}

void
ShowImageApp::BroadcastToWindows(uint32 what)
{
	const int32 n = CountWindows();
	for (int32 i = 0; i < n ; i ++) {
		// BMessenger checks for us if BWindow is still a valid object
		BMessenger msgr(WindowAt(i));
		msgr.SendMessage(what);
	}
}
