/*
 * Copyright 2004, pinc Software. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */


#include "UpdateWindow.h"
#include "NetworkTime.h"

#include <Application.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <StatusBar.h>
#include <Button.h>
#include <Alert.h>
#include <Screen.h>

#include <stdio.h>

static const uint32 kMsgAutoMode = 'aumd';
static const uint32 kMsgTryAllServers = 'tasr';
static const uint32 kMsgChooseDefaultServer = 'cdsr';
static const uint32 kMsgDefaultServer = 'dsrv';


UpdateWindow::UpdateWindow(BRect position, const BMessage &settings)
	: BWindow(position, "Network Time", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS),
	fSettings(settings)
{
	bool autoMode;
	if (settings.FindBool("auto mode", &autoMode) != B_OK)
		autoMode = false;

	// pressing the control key will avoid auto quit
	fAutoQuit = modifiers() & B_CONTROL_KEY ? false : autoMode;
	bool showMenu = !fAutoQuit;

	BMenuBar *bar = new BMenuBar(Bounds(), "");
	BMenu *menu;
	BMenuItem *item;

	menu = new BMenu("Project");

	menu->AddItem(item = new BMenuItem("About" B_UTF8_ELLIPSIS, new BMessage(B_ABOUT_REQUESTED), '?'));
	item->SetTarget(be_app);
	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
	bar->AddItem(menu);

	fServerMenu = new BMenu("Servers");
	fServerMenu->SetRadioMode(true);
	BuildServerMenu();

	menu = new BMenu("Options");
	menu->AddItem(fTryAllServersItem = new BMenuItem("Try All Servers",
		new BMessage(kMsgTryAllServers)));
	menu->AddItem(fChooseDefaultServerItem = new BMenuItem("Choose Default Server Automatically",
		new BMessage(kMsgChooseDefaultServer)));
	menu->AddItem(fAutoModeItem = new BMenuItem("Auto Mode", new BMessage(kMsgAutoMode)));
	menu->AddSeparatorItem();
	menu->AddItem(fServerMenu);
	menu->AddItem(new BMenuItem("Edit Server List" B_UTF8_ELLIPSIS, new BMessage(kMsgEditServerList)));
	bar->AddItem(menu);

	if (showMenu)
		AddChild(bar);

	BRect rect = Bounds();
	if (showMenu)
		rect.top = bar->Bounds().Height();

	BView *view = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(view);

	rect = view->Bounds().InsetByCopy(5, 5);
	fStatusBar = new BStatusBar(rect, "status", NULL, NULL);
	float width, height;
	fStatusBar->GetPreferredSize(&width, &height);
	fStatusBar->ResizeTo(rect.Width(), height);
	fStatusBar->SetResizingMode(B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT);
	view->AddChild(fStatusBar);

	BMessage *buttonMessage = new BMessage(fAutoQuit ? kMsgStopTimeUpdate : kMsgUpdateTime);
	buttonMessage->AddMessenger("monitor", this);

	if (showMenu) {
		rect.top += height + 4;
		fButton = new BButton(rect, NULL, "Update", buttonMessage,
			B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT);
		fButton->ResizeToPreferred();
		if (autoMode)
			fButton->SetLabel("Stop");
		fButton->MoveBy((rect.Width() - fButton->Bounds().Width()) / 2, 0);
		view->AddChild(fButton);

		ResizeTo(Bounds().Width(),
			bar->Bounds().Height() + fButton->Frame().bottom + 5);
	} else {
		fButton = NULL;
		ResizeTo(Bounds().Width(), height + 9);
	}

	SetSizeLimits(200, 16384, Bounds().Height(), Bounds().Height());

	if (Frame().LeftTop() == B_ORIGIN) {
		// center on screen
		BScreen screen;
		MoveTo((screen.Frame().Width() - Bounds().Width()) / 2,
			(screen.Frame().Height() - Bounds().Height()) / 2);
	}

	// Update views
	BMessage update = fSettings;
	update.what = kMsgSettingsUpdated;
	PostMessage(&update);

	// Automatically start update in "auto mode"
	if (fAutoQuit) {
		BMessage update(kMsgUpdateTime);
		update.AddMessenger("monitor", this);

		be_app->PostMessage(&update);
	}
}


UpdateWindow::~UpdateWindow()
{
}


bool
UpdateWindow::QuitRequested()
{
	BMessage update(kMsgUpdateSettings);
	update.AddRect("status frame", Frame());
	be_app_messenger.SendMessage(&update);

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
UpdateWindow::BuildServerMenu()
{
	// remove old entries
	BMenuItem *item;
	while ((item = fServerMenu->RemoveItem(0L)) != NULL)
		delete item;

	int32 defaultServer;
	if (fSettings.FindInt32("default server", &defaultServer) != B_OK)
		defaultServer = 0;

	const char *server;
	for (int32 i = 0; fSettings.FindString("server", i, &server) == B_OK; i++) {
		fServerMenu->AddItem(item = new BMenuItem(server, new BMessage(kMsgDefaultServer)));
		if (i == defaultServer)
			item->SetMarked(true);
	}
}


void
UpdateWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgSettingsUpdated:
		{
			if (message->HasString("server"))
				BuildServerMenu();
			else {
				int32 index;
				if (message->FindInt32("default server", &index) == B_OK) {
					BMenuItem *item = fServerMenu->ItemAt(index);
					if (item != NULL)
						item->SetMarked(true);
				}
			}

			bool boolean;
			if (message->FindBool("auto mode", &boolean) == B_OK)
				fAutoModeItem->SetMarked(boolean);
			if (message->FindBool("try all servers", &boolean) == B_OK)
				fTryAllServersItem->SetMarked(boolean);
			if (message->FindBool("choose default server", &boolean) == B_OK)
				fChooseDefaultServerItem->SetMarked(boolean);
			break;
		}

		case kMsgAutoMode:
		{
			fAutoModeItem->SetMarked(!fAutoModeItem->IsMarked());

			if (!fSettings.FindBool("knows auto mode")) {
				// we should alert the user about the changed behaviour
				(new BAlert("notice",
					"Attention!\n\n"
					"Activating this mode let's the user interface only show the "
					"bare minimum, and you will not be able to reach the menu "
					"anymore.\n"
					"Also, the application will quit immediately after the time has "
					"been updated successfully; it will only stay if updating fails, "
					"so that you can catch the reason for the error.\n\n"
					"If you want to get the full interface again, you have to "
					"press the control key while starting this application.\n\n"
					"This warning will appear only the first time you select this mode.",
					"Ok"))->Go();
			}

			BMessage update(kMsgUpdateSettings);
			update.AddBool("auto mode", fAutoModeItem->IsMarked());
			update.AddBool("knows auto mode", true);
			be_app->PostMessage(&update);
			break;
		}

		case kMsgTryAllServers:
		{
			fTryAllServersItem->SetMarked(!fTryAllServersItem->IsMarked());

			BMessage update(kMsgUpdateSettings);
			update.AddBool("try all servers", fTryAllServersItem->IsMarked());
			be_app->PostMessage(&update);
			break;
		}

		case kMsgChooseDefaultServer:
		{
			fChooseDefaultServerItem->SetMarked(!fChooseDefaultServerItem->IsMarked());

			BMessage update(kMsgUpdateSettings);
			update.AddBool("choose default server", fChooseDefaultServerItem->IsMarked());
			be_app->PostMessage(&update);
			break;
		}

		case kMsgDefaultServer:
		{
			int32 index;
			if (message->FindInt32("index", &index) != B_OK)
				break;

			BMessage update(kMsgUpdateSettings);
			update.AddInt32("default server", index);
			be_app->PostMessage(&update);
			break;
		}

		case kMsgEditServerList:
			be_app->PostMessage(kMsgEditServerList);
			break;

		case kMsgStopTimeUpdate:
			if (fButton != NULL) {
				fButton->SetLabel("Update");
				fButton->Message()->what = kMsgUpdateTime;
			}

			BButton *source;
			if (message->FindPointer("source", (void **)&source) == B_OK
				&& fButton == source)
				fStatusBar->SetText("Stopped.");

			be_app->PostMessage(message);
			break;

		case kMsgUpdateTime:
			if (fButton != NULL) {
				fButton->SetLabel("Stop");
				fButton->Message()->what = kMsgStopTimeUpdate;
			}

			be_app->PostMessage(message);
			break;

		case kMsgStatusUpdate:
			float progress;
			if (message->FindFloat("progress", &progress) == B_OK)
				fStatusBar->Update(progress - fStatusBar->CurrentValue());

			const char *text;
			if (message->FindString("message", &text) == B_OK)
				fStatusBar->SetText(text);

			status_t status;
			if (message->FindInt32("status", (int32 *)&status) == B_OK) {
				if (status == B_OK && fAutoQuit)
					QuitRequested();
				else if (fButton != NULL) {
					fButton->SetLabel("Update");
					fButton->Message()->what = kMsgUpdateTime;
				}
			}
			break;

		default:
			BWindow::MessageReceived(message);
	}
}

