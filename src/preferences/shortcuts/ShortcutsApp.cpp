/*
 * Copyright 1999-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		Fredrik Modéen
 */
 

#include "ShortcutsApp.h"

#include <Catalog.h>

#include "ShortcutsWindow.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ShortcutsApp"


ShortcutsApp::ShortcutsApp()
	:
	BApplication("application/x-vnd.Haiku-Shortcuts")
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
