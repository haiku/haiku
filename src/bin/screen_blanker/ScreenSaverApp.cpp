/*
 * Copyright 2003-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 */
#include <Debug.h>
#include <stdio.h>
#include <Screen.h>
#include <image.h>
#include <StorageDefs.h>
#include <FindDirectory.h>
#include <SupportDefs.h>
#include <File.h>
#include <Path.h>
#include <string.h>
#include <Beep.h>

#include "ScreenSaverApp.h"

const static int32 RESUME_SAVER = 'RSSV';


// Start the server application. Set pulse to fire once per second.
// Run until the application quits.
int main(int, char**) 
{	
	ScreenSaverApp myApplication;
	myApplication.Run();
	return(0);
}

// Construct the server app. Doesn't do much, at this point.
ScreenSaverApp::ScreenSaverApp()
	: BApplication(SCREEN_BLANKER_SIG),
	fWin(NULL),
	fSaver(NULL),
	fThrd(NULL),
	fPww(NULL),
	fThreadID(-1),
	fRunner(NULL)
{
	fBlankTime = system_time();
}


void 
ScreenSaverApp::ReadyToRun() 
{
	if (!fPref.LoadSettings()) {
		fprintf(stderr, "could not load settings\n");
		exit(1);
	} else { // If everything works OK, create a BDirectWindow and start the render thread.
		BScreen theScreen(B_MAIN_SCREEN_ID);
		fWin = new SSAwindow(theScreen.Frame());
		fPww = new PasswordWindow();
		fThrd = new ScreenSaverThread(fWin ,fWin->fView, &fPref);

		fSaver = fThrd->LoadAddOn();
		if (fSaver) {
			fWin->SetSaver(fSaver);
			fThreadID = spawn_thread(ScreenSaverThread::ThreadFunc,"ScreenSaverRenderer", B_LOW_PRIORITY,fThrd);
			resume_thread(fThreadID);
		} else {
			fprintf(stderr, "could not load the screensaver addon\n");
		}
		fWin->SetFullScreen(true);
		fWin->Show();
		HideCursor();
	}
}


void 
ScreenSaverApp::ShowPW() 
{
	if (fWin->Lock()) {
		if (fThreadID > -1)
			suspend_thread(fThreadID);
		if (B_OK==fWin->SetFullScreen(false)) {
			fWin->Sync();
			ShowCursor();
			fPww->Show();
			fPww->Sync();
		}
		fWin->Unlock();
	}
	delete fRunner;
	fRunner = new BMessageRunner(BMessenger(this), new BMessage(RESUME_SAVER), fPref.BlankTime(), 1);
	if (fRunner->InitCheck() != B_OK) {
		fprintf(stderr, "fRunner init failed\n");
	}
}


void 
ScreenSaverApp::MessageReceived(BMessage *message) 
{
	switch(message->what) {
		case UNLOCK_MESSAGE:
		{
			if (strcmp(fPref.Password(), crypt(fPww->GetPassword(), fPref.Password())) != 0) {
				beep();
				fPww->SetPassword("");
				delete fRunner;
				fRunner = new BMessageRunner(BMessenger(this), new BMessage(RESUME_SAVER), fPref.BlankTime(), 1);
				if (fRunner->InitCheck()!=B_OK) {
					fprintf(stderr, "fRunner init failed\n");
				}
			} else  {
				PRINT(("Quitting!\n"));
				Shutdown();
			}
			break;
		}
		case RESUME_SAVER:
			if (fWin->Lock()) {
				if (B_OK==fWin->SetFullScreen(true)) {
					HideCursor();
					fPww->Hide();
				}
				if (fThreadID > -1)
					resume_thread(fThreadID);
				fWin->Unlock();
			}
		default:
			BApplication::MessageReceived(message);
 			break;
	}
}


bool 
ScreenSaverApp::QuitRequested()
{
	if (fPref.LockEnable() && (system_time()-fBlankTime > (fPref.PasswordTime()-fPref.BlankTime()))) {
		ShowPW();
		return false;
	} else
		Shutdown();
	return BApplication::QuitRequested();
}


void 
ScreenSaverApp::Shutdown(void) 
{
	if (fWin)
		fWin->Hide();
	if (fThreadID > -1)
		kill_thread(fThreadID);
	if (fThrd)
		delete fThrd;
	Quit();
}
