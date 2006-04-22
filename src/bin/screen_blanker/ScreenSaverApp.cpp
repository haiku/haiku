/*
 * Copyright 2003-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 */


#include "ScreenSaverApp.h"

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

const static int32 RESUME_SAVER = 'RSSV';


ScreenSaverApp::ScreenSaverApp()
	: BApplication(SCREEN_BLANKER_SIG),
	fWindow(NULL),
	fSaver(NULL),
	fThread(NULL),
	fPasswordWindow(NULL),
	fThreadID(-1),
	fRunner(NULL)
{
	fBlankTime = system_time();
}


void
ScreenSaverApp::ReadyToRun() 
{
	if (!fPrefs.LoadSettings()) {
		fprintf(stderr, "could not load settings\n");
		exit(1);
	}
	
	// create a BDirectWindow and start the render thread.
	BScreen screen(B_MAIN_SCREEN_ID);
	fWindow = new ScreenSaverWindow(screen.Frame());
	fPasswordWindow = new PasswordWindow();
	fThread = new ScreenSaverThread(fWindow, fWindow->ChildAt(0), &fPrefs);

	fSaver = fThread->LoadAddOn();
	if (fSaver) {
		fWindow->SetSaver(fSaver);
		fThreadID = spawn_thread(ScreenSaverThread::ThreadFunc,
			"ScreenSaverRenderer", B_LOW_PRIORITY, fThread);
		resume_thread(fThreadID);
	} else {
		fprintf(stderr, "could not load the screensaver addon\n");
	}

	fWindow->SetFullScreen(true);
	fWindow->Show();
	HideCursor();
}


void
ScreenSaverApp::_ShowPasswordWindow() 
{
	if (fWindow->Lock()) {
		if (fThreadID > -1)
			suspend_thread(fThreadID);

		if (fWindow->SetFullScreen(false) == B_OK) {
			fWindow->Sync();
				// TODO: is that needed?
			ShowCursor();
			fPasswordWindow->Show();
		}
		fWindow->Unlock();
	}

	_ResumeScreenSaver();
}


void
ScreenSaverApp::_ResumeScreenSaver()
{
	delete fRunner;
	fRunner = new BMessageRunner(BMessenger(this), new BMessage(RESUME_SAVER),
		fPrefs.BlankTime(), 1);
	if (fRunner->InitCheck() != B_OK) {
		fprintf(stderr, "fRunner init failed\n");
	}
}


void
ScreenSaverApp::MessageReceived(BMessage *message) 
{
	switch (message->what) {
		case UNLOCK_MESSAGE:
		{
			if (strcmp(fPrefs.Password(), crypt(fPasswordWindow->GetPassword(),
					fPrefs.Password())) != 0) {
				beep();
				fPasswordWindow->SetPassword("");
				_ResumeScreenSaver();
			} else  {
				PRINT(("Quitting!\n"));
				_Shutdown();
				Quit();
			}
			break;
		}

		case RESUME_SAVER:
			if (fWindow->Lock()) {
				if (fWindow->SetFullScreen(true) == B_OK) {
					HideCursor();
					fPasswordWindow->Hide();
				}
				if (fThreadID > -1)
					resume_thread(fThreadID);
				fWindow->Unlock();
			}
			break;

		default:
			BApplication::MessageReceived(message);
 			break;
	}
}


bool
ScreenSaverApp::QuitRequested()
{
	if (fPrefs.LockEnable()
		&& (system_time() - fBlankTime > (fPrefs.PasswordTime() - fPrefs.BlankTime()))) {
		_ShowPasswordWindow();
		return false;
	}

	_Shutdown();
	return true;
}


void
ScreenSaverApp::_Shutdown() 
{
	if (fThread) {
		fThread->Quit(fThreadID);
		delete fThread;
	} else if (fThreadID > -1) {
		// ?!?
		kill_thread(fThreadID);
	}

	fThread = NULL;
	fThreadID = -1;

	if (fWindow)
		fWindow->Hide();
}


//	#pragma mark -


int
main(int, char**) 
{
	ScreenSaverApp app;
	app.Run();
	return 0;
}
