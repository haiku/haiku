/*
 * Copyright 2003-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 */


#include "ScreenBlanker.h"

#include <Beep.h>
#include <Debug.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Screen.h>
#include <StorageDefs.h>
#include <SupportDefs.h>
#include <image.h>

#include <stdio.h>
#include <string.h>


const static int32 RESUME_SAVER = 'RSSV';


ScreenBlanker::ScreenBlanker()
	: BApplication(SCREEN_BLANKER_SIG),
	fWindow(NULL),
	fSaver(NULL),
	fRunner(NULL),
	fPasswordWindow(NULL),
	fResumeRunner(NULL)
{
	fBlankTime = system_time();
}


ScreenBlanker::~ScreenBlanker()
{
	delete fResumeRunner;
}


void
ScreenBlanker::ReadyToRun() 
{
	if (!fPrefs.LoadSettings()) {
		fprintf(stderr, "could not load settings\n");
		exit(1);
	}
	
	// create a BDirectWindow and start the render thread.
	BScreen screen(B_MAIN_SCREEN_ID);
	fWindow = new ScreenSaverWindow(screen.Frame());
	fPasswordWindow = new PasswordWindow();
	fRunner = new ScreenSaverRunner(fWindow, fWindow->ChildAt(0), false, fPrefs);

	fSaver = fRunner->ScreenSaver();
	if (fSaver) {
		fWindow->SetSaver(fSaver);
		fRunner->Run();
	} else {
		fprintf(stderr, "could not load the screensaver addon\n");
	}

	fWindow->SetFullScreen(true);
	fWindow->Show();
	HideCursor();
}


void
ScreenBlanker::_ShowPasswordWindow() 
{
	if (fWindow->Lock()) {
		fRunner->Suspend();

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
ScreenBlanker::_ResumeScreenSaver()
{
	delete fResumeRunner;
	fResumeRunner = new BMessageRunner(BMessenger(this), new BMessage(RESUME_SAVER),
		fPrefs.BlankTime(), 1);
	if (fResumeRunner->InitCheck() != B_OK) {
		fprintf(stderr, "fRunner init failed\n");
	}
}


void
ScreenBlanker::MessageReceived(BMessage *message) 
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

				fRunner->Resume();
				fWindow->Unlock();
			}
			break;

		default:
			BApplication::MessageReceived(message);
 			break;
	}
}


bool
ScreenBlanker::QuitRequested()
{
	if (fPrefs.LockEnable()
		&& system_time() - fBlankTime > fPrefs.PasswordTime() - fPrefs.BlankTime()) {
		_ShowPasswordWindow();
		return false;
	}

	_Shutdown();
	return true;
}


void
ScreenBlanker::_Shutdown() 
{
	delete fRunner;

	if (fWindow)
		fWindow->Hide();
}


//	#pragma mark -


int
main(int, char**) 
{
	ScreenBlanker app;
	app.Run();
	return 0;
}
