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

ShowImageApp::ShowImageApp()
	: BApplication(APP_SIG), m_pOpenPanel(0)
{ }

void ShowImageApp::AboutRequested()
{
	BAlert* pAlert = new BAlert( "About ShowImage", 
								 "OBOS ShowImage\n\nRecreated by Fernando F.Oliveira", "OK");
	pAlert->Go();
}

void ShowImageApp::ReadyToRun()
{
	int32 count = ShowImageWindow::CountWindows();
	if (count < 1)
		OnOpen();
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
	if (message) {
		RefsReceived(message);
	}
}

void ShowImageApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
	case MSG_FILE_OPEN:
		OnOpen();
		break;
	case B_CANCEL:
		if (ShowImageWindow::CountWindows() < 1)
			PostMessage(B_QUIT_REQUESTED);
		break;
	default:
		BApplication::MessageReceived(message);
		break;
	}
}

bool ShowImageApp::QuitRequested()
{
	// Attempt to close all the document windows.
	bool ok = QuitDudeWinLoop();
	if (ok)
		// Everything's been saved, and only unimportant windows should remain.
		// Now we can forcibly blow those away.
		CloseAllWindows();
	
	return ok;
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

void ShowImageApp::OnOpen()
{
	if (! m_pOpenPanel) {
		m_pOpenPanel = new BFilePanel;
		m_pOpenPanel->Window()->SetTitle("Open Image File");
	}
	m_pOpenPanel->Show();
}

bool ShowImageApp::QuitDudeWinLoop()
{
	bool ok = true;
	status_t err;
	int32 i=0;
	while (ok) {
		BWindow* pWin = WindowAt(i++);
		if (! pWin)
			break;
			
		ShowImageWindow* pShowImageWindow = dynamic_cast<ShowImageWindow*>(pWin);
		if (pShowImageWindow && pShowImageWindow->Lock()) {
			BMessage quitMsg(B_QUIT_REQUESTED);
			BMessage reply;
			BMessenger winMsgr(pShowImageWindow);
			pShowImageWindow->Unlock();
			err = winMsgr.SendMessage(&quitMsg, &reply);
			if (err == B_OK) {
				bool result;
				err = reply.FindBool("result", &result);
				if (err == B_OK) {
					ok = result;
				}
			}
		}
	}
	return ok;
}

void ShowImageApp::CloseAllWindows()
{
	int32 i = 0;
	BWindow* pWin;
	for (pWin = WindowAt(i++); pWin && pWin->Lock(); pWin = WindowAt(i++)) {
		// don't take no for an answer
		pWin->Quit();
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
