/*
 * Copyright 2016-2017 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 *		Brian Hill <supernova@tycho.email>
 */

#include "SoftwareUpdaterApp.h"

#include <getopt.h>
#include <stdlib.h>

#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SoftwareUpdaterApp"


static const char* const kUsage = B_TRANSLATE_COMMENT(
	"Usage:  SoftwareUpdater <command> [ <option> ]\n"
	"Updates installed packages.\n"
	"\n"
	"Commands:\n"
	"  update     - Search repositories for updates on all packages.\n"
	"  check      - Check for available updates but only display a "
		"notification with results.\n"
	"  full-sync  - Synchronize the installed packages with the "
		"repositories.\n"
	"\n"
	"Options:\n"
	"  -h or --help       Print this usage help\n"
	"  -v or --verbose    Output verbose information\n",
	"Command line usage help")
;

static struct option const kLongOptions[] = {
	{"verbose", no_argument, 0, 'v'},
	{"help", no_argument, 0, 'h'},
	{NULL}
};


SoftwareUpdaterApp::SoftwareUpdaterApp()
	:
	BApplication(kAppSignature),
	fWorker(NULL),
	fFinalQuitFlag(false),
	fActionRequested(UPDATE),
	fVerbose(false),
	fArgvsAccepted(true)
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
	if (fWindowMessenger.IsValid())
		fWindowMessenger.SendMessage(kMsgCancel);
	return true;
}


void
SoftwareUpdaterApp::ReadyToRun()
{
	// Argvs no longer accepted once the process starts
	fArgvsAccepted = false;
	
	fWorker = new WorkingLooper(fActionRequested, fVerbose);
}


void
SoftwareUpdaterApp::ArgvReceived(int32 argc, char **argv)
{
	if (!fArgvsAccepted) {
		fputs(B_TRANSLATE("Argument variables are no longer accepted\n"),
			stderr);
		return;
	}
	
	int c;
	while ((c = getopt_long(argc, argv, "hv", kLongOptions, NULL)) != -1) {
		switch (c) {
			case 0:
				break;
			case 'h':
				fputs(kUsage, stdout);
				exit(0);
				break;
			case 'v':
				fVerbose = true;
				break;
			default:
				fputs(kUsage, stderr);
				exit(1);
				break;
		}
	}
	
	const char* command = "";
	int32 argCount = argc - optind;
	if (argCount == 0)
		return;
	else if (argCount > 1) {
		fputs(kUsage, stderr);
		exit(1);
	} else
		command = argv[optind];
	
	fActionRequested = USER_SELECTION_NEEDED;
	if (strcmp("update", command) == 0)
		fActionRequested = UPDATE;
	else if (strcmp("check", command) == 0)
		fActionRequested = UPDATE_CHECK_ONLY;
	else if (strcmp("full-sync", command) == 0)
		fActionRequested = FULLSYNC;
	else {
		fputs(B_TRANSLATE_COMMENT("Unrecognized argument", "Error message"),
			stderr);
		fprintf(stderr, " \"%s\"\n", command);
		fputs(kUsage, stderr);
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
	SoftwareUpdaterApp app;
	return app.Run();
}
