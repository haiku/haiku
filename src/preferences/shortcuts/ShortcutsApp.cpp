/*
 * Copyright 1999-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		Fredrik Modéen
 */
 

#include "ShortcutsApp.h"

#include <Alert.h>
#include <Catalog.h>
#include <Locale.h>

#include "ShortcutsWindow.h"


#define APPLICATION_SIGNATURE "application/x-vnd.Haiku-Shortcuts"

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "ShortcutsApp"


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
	BAlert* alert = new BAlert(B_TRANSLATE("About Shortcuts"), 
		B_TRANSLATE("Shortcuts\n\n"
		"Based on SpicyKeys for BeOS made by Jeremy Friesner."),
		B_TRANSLATE("OK"));
	alert->Go();
}

