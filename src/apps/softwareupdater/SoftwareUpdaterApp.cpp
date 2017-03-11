/*
 * Copyright 2016-2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 *		Brian Hill <supernova@warpmail.net>
 */

#include "SoftwareUpdaterApp.h"

#include "constants.h"


SoftwareUpdaterApp::SoftwareUpdaterApp()
	:
	BApplication("application/x-vnd.haiku-softwareupdater"),
	fWorker(NULL),
	fFinalQuitFlag(false)
{
}


SoftwareUpdaterApp::~SoftwareUpdaterApp()
{
	if (fWorker) {
		fWorker->Lock();
		fWorker->Quit();
	}
}


bool
SoftwareUpdaterApp::QuitRequested()
{
	if (fFinalQuitFlag)
		return true;
	
	// Simulate a cancel request from window- this gives the updater a chance
	// to quit cleanly
	if (fWindowMessenger.IsValid()) {
		fWindowMessenger.SendMessage(kMsgCancel);
		return false;
	}
	
	return true;
}


void
SoftwareUpdaterApp::ReadyToRun()
{
	fWorker = new WorkingLooper();
}


void
SoftwareUpdaterApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgRegister:
			message->FindMessenger(kKeyMessenger, &fWindowMessenger);
			break;
		
		case kMsgFinalQuit:
			fFinalQuitFlag = true;
			PostMessage(B_QUIT_REQUESTED);
			break;
		
		default:
			BApplication::MessageReceived(message);
	}
}


int
main()
{
	SoftwareUpdaterApp app;
	return app.Run();
}
