/*
 * Copyright 2016 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 */

#include "SoftwareUpdaterApp.h"

#include <stdio.h>
#include <AppDefs.h>

#include "SoftwareUpdaterWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "SoftwareUpdater"


SoftwareUpdaterApp::SoftwareUpdaterApp()
	:
	BApplication("application/x-vnd.haiku-softwareupdater")
{
}


SoftwareUpdaterApp::~SoftwareUpdaterApp()
{
}


void
SoftwareUpdaterApp::ReadyToRun()
{
	SoftwareUpdaterWindow* window = new SoftwareUpdaterWindow();
	if (window)
		window->Show();
}


int
main()
{
	SoftwareUpdaterApp app;
	return app.Run();
}
