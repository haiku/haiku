/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "FileTypes.h"
#include "FileTypesWindow.h"

#include <Application.h>
#include <MenuBar.h>
#include <MenuItem.h>

#include <be_apps/Tracker/RecentItems.h>


FileTypesWindow::FileTypesWindow(BPoint position)
	: BWindow(BRect(position.x, position.y, position.x + 300, position.y + 200),
		"FileTypes", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	// add the menu

	BMenuBar* menuBar = new BMenuBar(BRect(0, 0, 0, 0), NULL);
	AddChild(menuBar);

	BMenu* menu = new BMenu("File");
	menu->AddItem(new BMenuItem("New Resource File" B_UTF8_ELLIPSIS,
		NULL, 'N', B_COMMAND_KEY));

	BMenu* recentsMenu = BRecentFilesList::NewFileListMenu("Open" B_UTF8_ELLIPSIS,
		NULL, NULL, be_app, 10, false, NULL, kSignature);
	BMenuItem* item = new BMenuItem(recentsMenu, new BMessage(kMsgOpenFilePanel));
	item->SetShortcut('O', B_COMMAND_KEY);
	menu->AddItem(item);
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("About FileTypes" B_UTF8_ELLIPSIS,
		new BMessage(B_ABOUT_REQUESTED)));
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED),
		'Q', B_COMMAND_KEY));
	menu->SetTargetForItems(be_app);
	item->SetTarget(this);
	menuBar->AddItem(menu);
	
	BRect rect = Bounds();
	rect.top = menuBar->Bounds().Height() + 1;
	BView* topView = new BView(rect, NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	topView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(topView);
}


FileTypesWindow::~FileTypesWindow()
{
}


void
FileTypesWindow::MessageReceived(BMessage* message)
{
	BWindow::MessageReceived(message);
}


bool
FileTypesWindow::QuitRequested()
{
	be_app->PostMessage(kMsgTypesWindowClosed);
	return true;
}


