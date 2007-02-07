/*
 * Copyright (c) 2005-2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Author:
 *		DarkWyrm <darkwyrm@earthlink.net>
 */
#include "App.h"
#include "ResWindow.h"
#include <Entry.h>

int
main(void)
{
	App app;
	app.Run();
	return 0;
}

App::App(void)
  :	BApplication("application/x-vnd.Haiku-ResEdit"),
  	fWindowCount(0)
{
	fOpenPanel = new BFilePanel();
	fSavePanel = new BFilePanel(B_SAVE_PANEL);
}


App::~App(void)
{
	delete fOpenPanel;
	delete fSavePanel;
}


void
App::ReadyToRun(void)
{
/*
	if (fWindowCount < 1) {
		ResWindow *win = new ResWindow(BRect(50,100,600,400));
		win->Show();
	}
*/
	if (fWindowCount < 1) {
		BEntry entry("/boot/develop/projects/ResEdit/CapitalBe.rsrc");
		entry_ref ref;
		entry.GetRef(&ref);
		ResWindow *win = new ResWindow(BRect(50,100,600,400),&ref);
		win->Show();
	}
}


void
App::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case M_REGISTER_WINDOW: {
			fWindowCount++;
			break;
		}
		case M_UNREGISTER_WINDOW: {
			fWindowCount--;
			if (fWindowCount == 0)
				PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case M_SHOW_OPEN_PANEL: {
			// Don't do anything if it's already open
			if (fOpenPanel->IsShowing())
				break;
			fOpenPanel->Show();
			break;
		}
		default:
			BApplication::MessageReceived(msg);
	}
}


void
App::RefsReceived(BMessage *msg)
{
	entry_ref ref;
	int32 i=0;
	while (msg->FindRef("refs",i,&ref) == B_OK) {
		ResWindow *win = new ResWindow(BRect(50,100,600,400),&ref);
		win->Show();
		i++;
	}
}


bool
App::QuitRequested(void)
{
	for (int32 i = 0; i < CountWindows(); i++) {
		BWindow *win = WindowAt(i);
		if (fOpenPanel->Window() == win || fSavePanel->Window() == win)
			continue;
		
		if (!win->QuitRequested())
			return false;
	}
	
	return true;
}

