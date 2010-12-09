/*
 * Copyright 2003-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Phipps
 *		Jérôme Duval, jerome.duval@free.fr
 *		Axel Dörfler, axeld@pinc-software.de
 *		Ryan Leavengood, leavengood@gmail.com
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
	if (!fSettings.Load()) {
		fprintf(stderr, "could not load settings, using defaults\n");
	}

	// create a BDirectWindow and start the render thread.
	// TODO: we need a window per screen...
	BScreen screen(B_MAIN_SCREEN_ID);
	fWindow = new ScreenSaverWindow(screen.Frame());
	fPasswordWindow = new PasswordWindow();
	fRunner = new ScreenSaverRunner(fWindow, fWindow->ChildAt(0), false, fSettings);

	fSaver = fRunner->ScreenSaver();
	if (fSaver) {
		fWindow->SetSaver(fSaver);
		fRunner->Run();
	} else {
		fprintf(stderr, "could not load the screensaver addon\n");
		fWindow->ChildAt(0)->SetViewColor(0, 0, 0);
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

		fWindow->Sync();
			// TODO: is that needed?
		ShowCursor();
		if (fPasswordWindow->IsHidden())
			fPasswordWindow->Show();

		fWindow->Unlock();
	}

	_QueueResumeScreenSaver();
}


void
ScreenBlanker::_QueueResumeScreenSaver()
{
	delete fResumeRunner;

	BMessage resume(kMsgResumeSaver);
	fResumeRunner = new BMessageRunner(BMessenger(this), &resume,
		fSettings.BlankTime(), 1);
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

	uint32 flags = fSettings.TimeFlags();
	BScreen screen;
	uint32 capabilities = screen.DPMSCapabilites();
	if ((capabilities & B_DPMS_OFF) == 0)
		flags &= ~ENABLE_DPMS_OFF;
	if ((capabilities & B_DPMS_SUSPEND) == 0)
		flags &= ~ENABLE_DPMS_SUSPEND;
	if ((capabilities & B_DPMS_STAND_BY) == 0)
		flags &= ~ENABLE_DPMS_STAND_BY;

	if ((flags & ENABLE_DPMS_MASK) == 0)
		return;

	if (fSettings.OffTime() == fSettings.SuspendTime()
		&& (flags & (ENABLE_DPMS_OFF | ENABLE_DPMS_SUSPEND)) == (ENABLE_DPMS_OFF | ENABLE_DPMS_SUSPEND))
		flags &= ~ENABLE_DPMS_SUSPEND;
	if (fSettings.SuspendTime() == fSettings.StandByTime()
		&& (flags & (ENABLE_DPMS_SUSPEND | ENABLE_DPMS_STAND_BY)) == (ENABLE_DPMS_SUSPEND | ENABLE_DPMS_STAND_BY))
		flags &= ~ENABLE_DPMS_STAND_BY;

	// start them off again

	if (flags & ENABLE_DPMS_STAND_BY) {
		BMessage dpms(kMsgStandByScreen);
		fStandByScreenRunner = new BMessageRunner(BMessenger(this), &dpms,
			fSettings.StandByTime(), 1);
		if (fStandByScreenRunner->InitCheck() != B_OK)
			syslog(LOG_ERR, "standby screen saver runner failed\n");
	}

	if (flags & ENABLE_DPMS_SUSPEND) {
		BMessage dpms(kMsgSuspendScreen);
		fSuspendScreenRunner = new BMessageRunner(BMessenger(this), &dpms,
			fSettings.SuspendTime(), 1);
		if (fSuspendScreenRunner->InitCheck() != B_OK)
			syslog(LOG_ERR, "suspend screen saver runner failed\n");
	}

	if (flags & ENABLE_DPMS_OFF) {
		BMessage dpms(kMsgTurnOffScreen);
		fTurnOffScreenRunner = new BMessageRunner(BMessenger(this), &dpms,
			fSettings.OffTime(), 1);
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
			if (strcmp(fSettings.Password(), crypt(fPasswordWindow->Password(),
					fSettings.Password())) != 0) {
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
		{
			if (fWindow->Lock()) {
				HideCursor();
				fPasswordWindow->SetPassword("");
				fPasswordWindow->Hide();

				fRunner->Resume();
				fWindow->Unlock();
			}

			// Turn on the message filter again
			BMessage enable(kMsgEnableFilter);
			fWindow->PostMessage(&enable);

			_QueueTurnOffScreen();
			break;
		}

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
	if (fSettings.LockEnable()
		&& system_time() - fBlankTime > fSettings.PasswordTime() - fSettings.BlankTime()) {
		_ShowPasswordWindow();
		return false;
	}

	_Shutdown();
	return true;
}


void
ScreenBlanker::_Shutdown()
{
	if (fWindow) {
		fWindow->Hide();

		if (fWindow->Lock())
			fWindow->Quit();
	}

	delete fRunner;
}


//	#pragma mark -


int
main(int, char**)
{
	ScreenBlanker app;
	app.Run();
	return 0;
}
