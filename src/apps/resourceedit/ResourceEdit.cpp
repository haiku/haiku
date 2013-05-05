/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ResourceEdit.h"

#include "AboutWindow.h"
#include "Constants.h"
#include "MainWindow.h"
#include "SettingsFile.h"
#include "SettingsWindow.h"

#include <Entry.h>
#include <FilePanel.h>


ResourceEdit::ResourceEdit()
	:
	BApplication("application/x-vnd.Haiku-ResourceEdit")
{
	fCascade = BRect(100, 100, 700, 400);
	fCascadeCount = 0;

	fOpenPanel = new BFilePanel(B_OPEN_PANEL, &be_app_messenger, NULL, 0, true,
		new BMessage(MSG_OPEN_DONE));

	fSettings = new SettingsFile("resourceedit_settings");
	fSettings->Load();

	fSettingsWindow = NULL;
}


ResourceEdit::~ResourceEdit()
{

}


void
ResourceEdit::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MSG_NEW:
			_CreateWindow(NULL);
			break;

		case MSG_OPEN:
			fOpenPanel->Show();
			break;

		case MSG_OPEN_DONE:
		{
			entry_ref ref;

			while (fOpenPanel->GetNextSelectedRef(&ref) == B_OK)
				_CreateWindow(new BEntry(&ref));

			fOpenPanel->Rewind();

			break;
		}
		case MSG_CLOSE:
		{
			MainWindow* window;
			msg->FindPointer("window", (void**)&window);
			fWindowList.RemoveItem(window);

			if (fWindowList.CountItems() == 0)
				Quit();

			break;
		}
		case MSG_SAVEALL:
		{
			for (int32 i = 0; i < fWindowList.CountItems(); i++) {
				MainWindow* window = ((MainWindow*)fWindowList.ItemAt(i));
				window->PostMessage(MSG_SAVE);
			}

			break;
		}
		case MSG_SETTINGS:
		{
			if (fSettingsWindow != NULL)
				fSettingsWindow->Activate();
			else
				fSettingsWindow = new SettingsWindow(fSettings);

			break;
		}
		case MSG_SETTINGS_APPLY:
		{
			for (int32 i = 0; i < fWindowList.CountItems(); i++) {
				MainWindow* window = ((MainWindow*)fWindowList.ItemAt(i));
				window->PostMessage(MSG_SETTINGS_APPLY);
			}

			break;
		}
		case MSG_SETTINGS_CLOSED:
			fSettingsWindow = NULL;
			break;

		default:
			BApplication::MessageReceived(msg);
	}
}


void
ResourceEdit::ArgvReceived(int32 argc, char* argv[])
{
	for (int32 i = 1; i < argc; i++)
		_CreateWindow(new BEntry(argv[i]));
}


void
ResourceEdit::ReadyToRun()
{
	if (fWindowList.CountItems() <= 0)
		_CreateWindow(NULL);
}


void
ResourceEdit::_CreateWindow(BEntry* assocEntry)
{
	MainWindow* window = new MainWindow(_Cascade(), assocEntry, fSettings);

	fWindowList.AddItem(window);

	window->Show();
}


BRect
ResourceEdit::_Cascade()
{
	if (fCascadeCount == 8) {
		fCascade.OffsetBy(-20 * 8, -20 * 8);
		fCascadeCount = 0;
	} else {
		fCascade.OffsetBy(20, 20);
		fCascadeCount++;
	}

	return fCascade;
}
