/*
 * Copyright 2003, Michael Phipps. All rights reserved.
 * Distributed under the terms of the MIT License.
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
	fWin(NULL) 
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
		if (!fSaver) {
			fprintf(stderr, "could not load the screensaver addon\n");
			exit(1);
		}
		fWin->SetSaver(fSaver);
		fWin->SetFullScreen(true);
		fWin->Show();
		fThreadID = spawn_thread(ScreenSaverThread::ThreadFunc,"ScreenSaverRenderer", B_LOW_PRIORITY,fThrd);
		resume_thread(fThreadID);
		HideCursor();
	}
}


void 
ScreenSaverApp::ShowPW() 
{
	fWin->Lock();
	suspend_thread(fThreadID);
	if (B_OK==fWin->SetFullScreen(false)) {
		fWin->Sync();
		ShowCursor();
		fPww->Show();
		fPww->Sync();
	}
	fWin->Unlock();
}


void 
ScreenSaverApp::MessageReceived(BMessage *message) 
{
	switch(message->what) {
		case UNLOCK_MESSAGE:
		{
			char salt[3] = "";
			strncpy(salt, fPref.Password(), 2);
			if (strcmp(crypt(fPww->GetPassword(), salt),fPref.Password())) {
				beep();
				fPww->Hide();
				fWin->SetFullScreen(true);
				if (fThreadID)
					resume_thread(fThreadID);
				}
				else  {
					PRINT(("Quitting!\n"));
					Shutdown();
				}
			break;
		}
		default:
			BApplication::MessageReceived(message);
 			break;
	}
}


bool 
ScreenSaverApp::QuitRequested()
{
	if (system_time()-fBlankTime>fPref.PasswordTime()) {
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
	if (fThreadID)
		kill_thread(fThreadID);
	if (fThrd)
		delete fThrd;
	Quit();
}
