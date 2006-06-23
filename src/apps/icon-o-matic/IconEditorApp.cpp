/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "IconEditorApp.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include "Document.h"
#include "MainWindow.h"

// testing:
#include "ColorPickerPanel.h"

using std::nothrow;

// constructor
IconEditorApp::IconEditorApp()
	: BApplication("application/x-vnd.Haiku-Icon-O-Matic"),
	  fMainWindow(NULL),
	  fDocument(new Document("test"))
{
}

// destructor
IconEditorApp::~IconEditorApp()
{
	// NOTE: it is important that the GUI has been deleted
	// at this point, so that all the listener/observer
	// stuff is properly detached
	delete fDocument;
}

// #pragma mark -

// QuitRequested
bool
IconEditorApp::QuitRequested()
{
	// TODO: ask main window if quitting is ok
	fMainWindow->Lock();
	fMainWindow->Quit();
	fMainWindow = NULL;

	return true;
}

// MessageReceived
void
IconEditorApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
//		case MSG_OPEN:
//			break;

		default:
			BApplication::MessageReceived(message);
			break;
	}
}

// ReadyToRun
void
IconEditorApp::ReadyToRun()
{
	fMainWindow = new MainWindow(this, fDocument);
	fMainWindow->Show();

	ColorPickerPanel* panel = new ColorPickerPanel(
		BRect(80, 80, 200, 200),
		(rgb_color){ 100, 200, 150, 255 });
	panel->Show();
}


