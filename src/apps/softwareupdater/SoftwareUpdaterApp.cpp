/*
 * Copyright 2016-2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 *		Brian Hill <supernova@warpmail.net>
 */

#include "SoftwareUpdaterApp.h"

#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SoftwareUpdaterApp"


static const char* const kUsage = B_TRANSLATE_COMMENT(
	"Usage:  SoftwareUpdater <command>\n"
	"Updates installed packages.\n"
	"\n"
	"Commands:\n"
	"  update     - Search repositories for updates on all packages.\n"
	"  check      - Check for available updates but only display a "
		"notification with results.\n"
	"  full-sync  - Synchronize the installed packages with the "
		"repositories.\n", "Command line usage help")
;


SoftwareUpdaterApp::SoftwareUpdaterApp()
	:
	BApplication(kAppSignature),
	fWorker(NULL),
	fFinalQuitFlag(false),
	fActionRequested(UPDATE)
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
	fWorker = new WorkingLooper(fActionRequested);
}


void
SoftwareUpdaterApp::ArgvReceived(int32 argc, char **argv)
{
	// Only one command allowed
	if (argc > 2) {
		fprintf(stderr, kUsage);
		PostMessage(B_QUIT_REQUESTED);
		return;
	} else if (argc == 2) {
		fActionRequested = USER_SELECTION_NEEDED;
		const char* command = argv[1];
		if (strcmp("update", command) == 0)
			fActionRequested = UPDATE;
		else if (strcmp("check", command) == 0)
			fActionRequested = UPDATE_CHECK_ONLY;
		else if (strcmp("full-sync", command) == 0)
			fActionRequested = FULLSYNC;
		else
			fprintf(stderr, kUsage);
	}
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
main(int argc, const char* const* argv)
{
	if (argc > 2) {
		fprintf(stderr, kUsage);
		return 1;
	}
	
	SoftwareUpdaterApp app;
	return app.Run();
}
