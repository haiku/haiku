/*
    OBOS ShowImage 0.1 - 17/02/2002 - 22:22 - Fernando Francisco de Oliveira
*/

#include <stdio.h>
#include <Alert.h>
#include <FilePanel.h>

#include "ShowImageConstants.h"
#include "ShowImageApp.h"
#include "ShowImageWindow.h"

int main(int, char**)
{
	ShowImageApp theApp;
	theApp.Run();
	return 0;
}

#define WINDOWS_TO_IGNORE 1

ShowImageApp::ShowImageApp()
	: BApplication(APP_SIG)
{
	fbpulseStarted = false;
	m_pOpenPanel = new BFilePanel(B_OPEN_PANEL);
}

void ShowImageApp::AboutRequested()
{
	BAlert* pAlert = new BAlert("About ShowImage",
		"OBOS ShowImage\n\nby Fernando F. Oliveira and Michael Wilber", "OK");
	pAlert->Go();
}

void ShowImageApp::ReadyToRun()
{
	if (CountWindows() == WINDOWS_TO_IGNORE)
		m_pOpenPanel->Show();
	else
		// If image windows are already open
		// (paths supplied on the command line)
		// start checking the number of open windows
		StartPulse();
}

void
ShowImageApp::StartPulse()
{
	if (!fbpulseStarted) {
		// Tell the app to begin checking
		// for the number of open windows
		fbpulseStarted = true;
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

void ShowImageApp::ArgvReceived(int32 argc, char** argv)
{
	BMessage* message = 0;
	for (int32 i=1; i<argc; i++) {
		entry_ref ref;
		status_t err = get_ref_for_path(argv[i], &ref);
		if (err == B_OK) {
			if (! message) {
				message = new BMessage;
				message->what = B_REFS_RECEIVED;
			}
			message->AddRef("refs", &ref);
		}
	}
	if (message)
		RefsReceived(message);
}

void ShowImageApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_FILE_OPEN:
			m_pOpenPanel->Show();
			break;
		case MSG_WINDOW_QUIT:
			break;
			
		case B_CANCEL:
			// File open panel was closed,
			// start checking count of open windows
			StartPulse();
			break;

		default:
			BApplication::MessageReceived(message);
			break;
	}
}

void ShowImageApp::RefsReceived(BMessage* message)
{
	uint32 type;
	int32 count;
	entry_ref ref;
	
	message->GetInfo("refs", &type, &count);
	if (type != B_REF_TYPE)
		return;
	
	for (int32 i = --count; i >= 0; --i) {
   		if (message->FindRef("refs", i, &ref) == B_OK) {
   			Open(&ref);
   		}
   	}
}

void ShowImageApp::Open(const entry_ref* ref)
{
	if (ShowImageWindow::NewWindow(ref) != B_OK) {
		char errStr[B_FILE_NAME_LENGTH + 50];
		sprintf(errStr, "Couldn't open file: %s",
			(ref && ref->name) ? ref->name : "???");
		BAlert* pAlert = new BAlert("file i/o error", errStr, "OK");
		pAlert->Go();
	}
}
