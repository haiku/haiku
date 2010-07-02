/*
 * Copyright 2010 Wim van der Meer <WPJvanderMeer@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Wim van der Meer
 */


#include "ScreenshotApp.h"

#include <stdlib.h>

#include <Bitmap.h>
#include <Catalog.h>
#include <Locale.h>
#include <Roster.h>

#include "ScreenshotWindow.h"
#include "Utility.h"


ScreenshotApp::ScreenshotApp()
	:
	BApplication("application/x-vnd.haiku-screenshot"),
	fUtility(new Utility),
	fSilent(false),
	fClipboard(false)
{
}


ScreenshotApp::~ScreenshotApp()
{
	delete fUtility;
}


void
ScreenshotApp::MessageReceived(BMessage* message)
{
	status_t status = B_OK;
	switch (message->what) {
		case SS_UTILITY_DATA:
		{
			BMessage bitmap;
			status = message->FindMessage("wholeScreen", &bitmap);
			if (status != B_OK)
				break;

			fUtility->wholeScreen = new BBitmap(&bitmap);

			status = message->FindMessage("cursorBitmap", &bitmap);
			if (status != B_OK)
				break;

			fUtility->cursorBitmap = new BBitmap(&bitmap);

			status = message->FindMessage("cursorAreaBitmap", &bitmap);
			if (status != B_OK)
				break;

			fUtility->cursorAreaBitmap = new BBitmap(&bitmap);

			status = message->FindPoint("cursorPosition",
				&fUtility->cursorPosition);
			if (status != B_OK)
				break;

			status = message->FindRect("activeWindowFrame",
				&fUtility->activeWindowFrame);
			if (status != B_OK)
				break;

			status = message->FindRect("tabFrame", &fUtility->tabFrame);
			if (status != B_OK)
				break;

			status = message->FindFloat("borderSize", &fUtility->borderSize);
			if (status != B_OK)
				break;

			break;
		}

		default:
			BApplication::MessageReceived(message);
			break;
	}
	
	if (status != B_OK)
		be_app->PostMessage(B_QUIT_REQUESTED);
}


void
ScreenshotApp::ArgvReceived(int32 argc, char** argv)
{
	for (int32 i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-s") == 0 
			|| strcmp(argv[i], "--silent") == 0)
			fSilent = true;
		else if (strcmp(argv[i], "-c") == 0 
			|| strcmp(argv[i], "--clipboard") == 0)
			fClipboard = true;			
	}
}


void
ScreenshotApp::ReadyToRun()
{
	new ScreenshotWindow(*fUtility, fSilent, fClipboard);
}


int
main()
{
	ScreenshotApp app;
	return app.Run();
}
