/*
 * Copyright 1999-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		Fredrik Modéen
 */
 
#include "ShortcutsApp.h"


#include "Alert.h"


#include "ShortcutsWindow.h"

#define APPLICATION_SIGNATURE "application/x-vnd.ShortcutsKeys"

ShortcutsApp::ShortcutsApp()
	:
	BApplication(APPLICATION_SIGNATURE)
{
}


void
ShortcutsApp::ReadyToRun()
{
	ShortcutsWindow* window = new ShortcutsWindow();
	window->Show();
}


ShortcutsApp::~ShortcutsApp()
{ 

}


void
ShortcutsApp::AboutRequested()
{
	BAlert* alert = new BAlert("About Shortcuts", 
		"Shortcuts v1.28(SpicyKeys v1.28)\nby Jeremy Friesner"
		, "Ok");
	alert->Go();
}
