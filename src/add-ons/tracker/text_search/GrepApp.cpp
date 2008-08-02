/*
 * Copyright (c) 1998-2007 Matthijs Hollemans
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */
 
#include "GrepApp.h"

#include <stdio.h>

#include <Entry.h>

#include "GlobalDefs.h"
#include "GrepWindow.h"
#include "Model.h"


GrepApp::GrepApp()
	: BApplication(APP_SIGNATURE),
	  fGotArgvOnStartup(false),
	  fGotRefsOnStartup(false),
	  fQuitter(NULL)
{
}


GrepApp::~GrepApp()
{
	delete fQuitter;
}


void 
GrepApp::ArgvReceived(int32 argc, char** argv)
{
	fGotArgvOnStartup = true;

	BMessage message(B_REFS_RECEIVED);
	int32 refCount = 0;
	
	for (int32 i = 1; i < argc; i++) {
		BEntry entry(argv[i]);
		entry_ref ref;
		entry.GetRef(&ref);
		
		if (entry.Exists()) {
			message.AddRef("refs", &ref);
			refCount += 1;
		} else
			printf("%s: File not found: %s\n", argv[0], argv[i]);
	}

	if (refCount > 0)
		RefsReceived(&message);
}


void
GrepApp::RefsReceived(BMessage* message)
{ 
	if (IsLaunching())
		fGotRefsOnStartup = true;

	new GrepWindow(message);
}


void
GrepApp::ReadyToRun()
{
	if (!fGotArgvOnStartup && !fGotRefsOnStartup)
		_NewUnfocusedGrepWindow();

	// TODO: stippi: I don't understand what this is supposed to do:
	if (fGotArgvOnStartup && !fGotRefsOnStartup)
		PostMessage(B_QUIT_REQUESTED);
}


void
GrepApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_SILENT_RELAUNCH:
			_NewUnfocusedGrepWindow();
			break;

		case MSG_TRY_QUIT:
			_TryQuit();
			break;

		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
GrepApp::_TryQuit()
{ 
	if (CountWindows() == 0)
		PostMessage(B_QUIT_REQUESTED);
	
	if (CountWindows() == 1 && fQuitter == NULL) {
		fQuitter = new BMessageRunner(be_app_messenger, 
			new BMessage(MSG_TRY_QUIT), 200000, -1);
	}
}


void
GrepApp::_NewUnfocusedGrepWindow()
{
	BMessage emptyMessage;
	new GrepWindow(&emptyMessage);
}
