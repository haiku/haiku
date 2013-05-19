/* PatchApp.cpp
 * ------------
 * Implements the PatchBay application class and main().
 *
 * Copyright 2013, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Revisions by Pete Goodeve
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
 
#include "PatchApp.h"

#include <Roster.h>

#include "PatchWin.h"

PatchApp::PatchApp()
	:
	BApplication("application/x-vnd.Haiku.PatchBay")
{}


void
PatchApp::ReadyToRun()
{
	new PatchWin;
}


void
PatchApp::MessageReceived(BMessage* msg)
{
	BApplication::MessageReceived(msg);
}


int
main(void)
{
	PatchApp app;
	app.Run();
	return 0;
}

