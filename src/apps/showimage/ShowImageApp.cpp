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
#include <String.h>
#include <Clipboard.h>

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
	fPulseStarted = false;
	fOpenPanel = new BFilePanel(B_OPEN_PANEL);
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
		fOpenPanel->Show();
	else
		// If image windows are already open
		// (paths supplied on the command line)
		// start checking the number of open windows
		StartPulse();

	be_clipboard->StartWatching(be_app_messenger);
		// tell the clipboard to notify this app when its contents change
}

void
ShowImageApp::StartPulse()
{
	if (!fPulseStarted) {
		// Tell the app to begin checking
		// for the number of open windows
		fPulseStarted = true;
		SetPulseRate(250000);
			// Set pulse to every 1/4 second
	}
}

void
ShowImageApp::Pulse()
{
	// Bug: The BFilePanel is automatically closed if the volume that
	// is displayed is unmounted.
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
	if (pmsg) {
		RefsReceived(pmsg);
		delete pmsg;
	}
}

void
ShowImageApp::MessageReceived(BMessage *pmsg)
{
	switch (pmsg->what) {
		case MSG_FILE_OPEN:
			fOpenPanel->Show();
			break;
			
		case MSG_WINDOW_QUIT:
			break;
			
		case B_CANCEL:
			// File open panel was closed,
			// start checking count of open windows
			StartPulse();
			break;
		
		case B_CLIPBOARD_CHANGED:
			CheckClipboard();
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
ShowImageApp::BroadcastToWindows(BMessage *pmsg)
{
	const int32 n = CountWindows();
	for (int32 i = 0; i < n ; i ++) {
		// BMessenger checks for us if BWindow is still a valid object
		BMessenger msgr(WindowAt(i));
		msgr.SendMessage(pmsg);
	}
}

void
ShowImageApp::CheckClipboard()
{
	// Determines if the contents of the clipboard contain
	// data that is useful to this application. 
	// After checking the clipboard, a message is sent to
	// all windows indicating that the clipboard has changed
	// and whether or not the clipboard contains useful data.
	bool bdata = false;
 
	if (be_clipboard->Lock()) {	
		BMessage *pclip;
		if ((pclip = be_clipboard->Data()) != NULL) {
			BString strClass;
			if (pclip->FindString("class", &strClass) == B_OK) {
				if (strClass == "BBitmap")
					bdata = true;
			}
		}
		
		be_clipboard->Unlock(); 
	}
	
	BMessage msg(MSG_CLIPBOARD_CHANGED);
	msg.AddBool("data_available", bdata);
	BroadcastToWindows(&msg);
}

void
ShowImageApp::Quit()
{
	be_clipboard->StopWatching(be_app_messenger);
		// tell clipboard we don't want anymore notification
	
	BApplication::Quit();
}

