/* PatchApp.cpp
 * ------------
 * Implements the PatchBay application class and main().
 *
 * Copyright 2013-2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Revisions by:
 * 		Pete Goodeve
 *		Philippe Houdoin
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */
 
#include "PatchApp.h"

#include <Roster.h>
#include <Catalog.h>

#include "PatchWin.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "App"

const char * kApplicationName = B_TRANSLATE_SYSTEM_NAME("PatchBay");
const char * kApplicationSignature = "application/x-vnd.Haiku.PatchBay";


PatchApp::PatchApp()
	:
	BApplication(kApplicationSignature)
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

