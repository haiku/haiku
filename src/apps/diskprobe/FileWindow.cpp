/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "FileWindow.h"
#include "OpenWindow.h"
#include "DiskProbe.h"
#include "ProbeView.h"

#include <Application.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <be_apps/Tracker/RecentItems.h>


FileWindow::FileWindow(BRect rect, entry_ref *ref, const BMessage *settings)
	: ProbeWindow(rect, ref)
{
	// Set alternative window title for devices

	BEntry entry(ref);
	struct stat stat;
	if (entry.GetStat(&stat) == B_OK && (stat.st_mode & (S_IFBLK | S_IFCHR)) != 0) {
		BPath path(ref);
		SetTitle(path.Path());
	}

	// add the menu

	BMenuBar *menuBar = new BMenuBar(BRect(0, 0, 0, 0), NULL);
	AddChild(menuBar);

	BMenu *menu = new BMenu("File");
	menu->AddItem(new BMenuItem("New" B_UTF8_ELLIPSIS,
					new BMessage(kMsgOpenOpenWindow), 'N', B_COMMAND_KEY));

	BMenu *devicesMenu = new BMenu("Open Device");
	OpenWindow::CollectDevices(devicesMenu);
	devicesMenu->SetTargetForItems(be_app);
	menu->AddItem(new BMenuItem(devicesMenu));

	BMenu *recentsMenu = BRecentFilesList::NewFileListMenu("Open File" B_UTF8_ELLIPSIS,
							NULL, NULL, be_app, 10, false, NULL, kSignature);
	BMenuItem *item;
	menu->AddItem(item = new BMenuItem(recentsMenu, new BMessage(kMsgOpenFilePanel)));
	item->SetShortcut('O', B_COMMAND_KEY);
	menu->AddSeparatorItem();

	// the ProbeView save menu items will be inserted here
	item = new BMenuItem("Close", new BMessage(B_CLOSE_REQUESTED), 'W', B_COMMAND_KEY);
	menu->AddItem(item);
	menu->AddSeparatorItem();

	// the ProbeView print menu items will be inserted here
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("About DiskProbe" B_UTF8_ELLIPSIS, new BMessage(B_ABOUT_REQUESTED)));
	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q', B_COMMAND_KEY));
	menu->SetTargetForItems(be_app);
	item->SetTarget(this);
	menuBar->AddItem(menu);

	// add our interface widgets

	BRect rect = Bounds();
	rect.top = menuBar->Bounds().Height() + 1;
	fProbeView = new ProbeView(rect, ref, NULL, settings);
	AddChild(fProbeView);

	fProbeView->AddSaveMenuItems(menu, 4);
	fProbeView->AddPrintMenuItems(menu, menu->CountItems() - 4);

	fProbeView->UpdateSizeLimits();
}


bool 
FileWindow::QuitRequested()
{
	bool quit = fProbeView->QuitRequested();
	if (!quit)
		return false;

	return ProbeWindow::QuitRequested();
}


bool
FileWindow::Contains(const entry_ref &ref, const char *attribute)
{
	if (attribute != NULL)
		return false;

	return ref == Ref();
}

