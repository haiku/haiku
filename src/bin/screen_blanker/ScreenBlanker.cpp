/*
 * Copyright 2003-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 *		Axel Dörfler, axeld@pinc-software.de
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
#include <syslog.h>


const static uint32 kMsgResumeSaver = 'RSSV';
const static uint32 kMsgTurnOffScreen = 'tofs';
const static uint32 kMsgSuspendScreen = 'suss';
const static uint32 kMsgStandByScreen = 'stbs';


ScreenBlanker::ScreenBlanker()
	: BApplication(SCREEN_BLANKER_SIG),
	fWindow(NULL),
	fSaver(NULL),
	fRunner(NULL),
	fPasswordWindow(NULL),
	fResumeRunner(NULL),
	fStandByScreenRunner(NULL),
	fSuspendScreenRunner(NULL),
	fTurnOffScreenRunner(NULL)
{
	fBlankTime = system_time();
}


ScreenBlanker::~ScreenBlanker()
{
	delete fResumeRunner;
	_TurnOnScreen();
}


void
ScreenBlanker::ReadyToRun() 
{
	if (!fPrefs.LoadSettings()) {
		fprintf(stderr, "could not load settings\n");
		exit(1);
	}
	
	// create a BDirectWindow and start the render thread.
	// TODO: we need a window per screen...
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

	_QueueTurnOffScreen();
}


void
ScreenBlanker::_TurnOnScreen()
{
	delete fStandByScreenRunner;
	delete fSuspendScreenRunner;
	delete fTurnOffScreenRunner;

	fStandByScreenRunner = fSuspendScreenRunner = fTurnOffScreenRunner = NULL;

	BScreen screen;
	screen.SetDPMS(B_DPMS_ON);
}


void
ScreenBlanker::_SetDPMSMode(uint32 mode)
{
	BScreen screen;
	screen.SetDPMS(mode);

	if (fWindow->Lock()) {
		fRunner->Suspend();
		fWindow->Unlock();
	}
}


void
ScreenBlanker::_ShowPasswordWindow() 
{
	_TurnOnScreen();

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

	_QueueResumeScreenSaver();
}


void
ScreenBlanker::_QueueResumeScreenSaver()
{
	delete fResumeRunner;
	fResumeRunner = new BMessageRunner(BMessenger(this), new BMessage(kMsgResumeSaver),
		fPrefs.BlankTime(), 1);
	if (fResumeRunner->InitCheck() != B_OK)
		syslog(LOG_ERR, "resume screen saver runner failed\n");
}


void
ScreenBlanker::_QueueTurnOffScreen()
{
	// stop running notifiers

	delete fStandByScreenRunner;
	delete fSuspendScreenRunner;
	delete fTurnOffScreenRunner;
	
	fStandByScreenRunner = fSuspendScreenRunner = fTurnOffScreenRunner = NULL;

	// figure out which notifiers we need and which of them are supported

	uint32 flags = fPrefs.TimeFlags();
	BScreen screen;
	uint32 capabilities = screen.DPMSCapabilites();
	if ((capabilities & B_DPMS_OFF) == 0)
		flags &= ENABLE_DPMS_OFF;
	if ((capabilities & B_DPMS_SUSPEND) == 0)
		flags &= ENABLE_DPMS_SUSPEND;
	if ((capabilities & B_DPMS_STAND_BY) == 0)
		flags &= ENABLE_DPMS_STAND_BY;

	if ((flags & ENABLE_DPMS_MASK) == 0)
		return;

	if (fPrefs.OffTime() == fPrefs.SuspendTime())
		flags &= ENABLE_DPMS_SUSPEND;
	if (fPrefs.SuspendTime() == fPrefs.StandByTime())
		flags &= ENABLE_DPMS_STAND_BY;

	// start them off again

	if (flags & ENABLE_DPMS_STAND_BY) {
		fStandByScreenRunner = new BMessageRunner(BMessenger(this),
			new BMessage(kMsgStandByScreen), fPrefs.StandByTime(), 1);
		if (fStandByScreenRunner->InitCheck() != B_OK)
			syslog(LOG_ERR, "standby screen saver runner failed\n");
	}

	if (flags & ENABLE_DPMS_SUSPEND) {
		fSuspendScreenRunner = new BMessageRunner(BMessenger(this),
			new BMessage(kMsgSuspendScreen), fPrefs.SuspendTime(), 1);
		if (fSuspendScreenRunner->InitCheck() != B_OK)
			syslog(LOG_ERR, "turn off screen saver runner failed\n");
	}

	if (flags & ENABLE_DPMS_OFF) {
		fTurnOffScreenRunner = new BMessageRunner(BMessenger(this),
			new BMessage(kMsgTurnOffScreen), fPrefs.OffTime(), 1);
		if (fTurnOffScreenRunner->InitCheck() != B_OK)
			syslog(LOG_ERR, "turn off screen saver runner failed\n");
	}
}


void
ScreenBlanker::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgUnlock:
		{
			if (strcmp(fPrefs.Password(), crypt(fPasswordWindow->Password(),
					fPrefs.Password())) != 0) {
				beep();
				fPasswordWindow->SetPassword("");
				_QueueResumeScreenSaver();
			} else  {
				PRINT(("Quitting!\n"));
				_Shutdown();
				Quit();
			}
			break;
		}

		case kMsgResumeSaver:
			if (fWindow->Lock()) {
				if (fWindow->SetFullScreen(true) == B_OK) {
					HideCursor();
					fPasswordWindow->Hide();
				}

				fRunner->Resume();
				fWindow->Unlock();
			}

			_QueueTurnOffScreen();
			break;

		case kMsgTurnOffScreen:
			_SetDPMSMode(B_DPMS_OFF);
			break;
		case kMsgSuspendScreen:
			_SetDPMSMode(B_DPMS_SUSPEND);
			break;
		case kMsgStandByScreen:
			_SetDPMSMode(B_DPMS_STAND_BY);
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
