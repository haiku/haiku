/*
 * Copyright 2014 Freeman Lou <freemanlou2430@yahoo.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "VirtualKeyboardWindow.h"

#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <GroupLayoutBuilder.h>
#include <ListView.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Path.h>
#include <Screen.h>

#include "KeyboardLayoutView.h"
#include "KeymapListItem.h"

static const uint32 kMsgMenuFontChange = 'mMFC';

static int
compare_key_list_items(const void* a, const void* b)
{
	KeymapListItem* item1 = *(KeymapListItem**)a;
	KeymapListItem* item2 = *(KeymapListItem**)b;

	return BLocale::Default()->StringCompare(item1->Text(), item2->Text());
}


VirtualKeyboardWindow::VirtualKeyboardWindow(BInputServerDevice* dev)
	:
	BWindow(BRect(0,0,0,0),"Virtual Keyboard",
	B_NO_BORDER_WINDOW_LOOK, B_FLOATING_ALL_WINDOW_FEEL,
	B_WILL_ACCEPT_FIRST_CLICK | B_AVOID_FOCUS),
	fDevice(dev)
{
	BScreen screen;
	BRect screenRect(screen.Frame());

	ResizeTo(screenRect.Width(), screenRect.Height() / 3);
	MoveTo(0,screenRect.Height() - screenRect.Height() / 3);

	SetLayout(new BGroupLayout(B_VERTICAL));

	//Add to an options window later, use as list for now
	fMapListView = new BListView("Maps");
	fFontMenu = new BMenu("Font");
	fLayoutMenu = new BMenu("Layout");

	_LoadMaps();
	_LoadLayouts(fLayoutMenu);
	_LoadFonts();

	KeymapListItem* current =
		static_cast<KeymapListItem*>(fMapListView->LastItem());
	fCurrentKeymap.Load(current->EntryRef());


	fKeyboardView = new KeyboardLayoutView("Keyboard",fDevice);
	fKeyboardView->GetKeyboardLayout()->SetDefault();
	fKeyboardView->SetEditable(false);
	fKeyboardView->SetKeymap(&fCurrentKeymap);

	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(fKeyboardView));
}


void
VirtualKeyboardWindow::_LoadLayouts(BMenu* menu)
{
	directory_which dataDirectories[] = {
		B_USER_NONPACKAGED_DATA_DIRECTORY,
		B_USER_DATA_DIRECTORY,
		B_SYSTEM_NONPACKAGED_DIRECTORY,
		B_SYSTEM_DATA_DIRECTORY,
	};

	for (uint i = 0; i < sizeof(dataDirectories)/sizeof(directory_which); i++) {
		BPath path;
		if (find_directory(dataDirectories[i], &path) != B_OK)
			continue;

		path.Append("KeyboardLayouts");

		BDirectory directory;
		if (directory.SetTo(path.Path()) == B_OK)
			_LoadLayoutMenu(menu, directory);
	}
}


void
VirtualKeyboardWindow::_LoadLayoutMenu(BMenu* menu, BDirectory directory)
{
	entry_ref ref;

	while (directory.GetNextRef(&ref) == B_OK) {
		if (menu->FindItem(ref.name) != NULL)
			continue;

		BDirectory subdirectory;
		subdirectory.SetTo(&ref);
		if (subdirectory.InitCheck() == B_OK) {
			BMenu* submenu = new BMenu(ref.name);
			_LoadLayoutMenu(submenu, subdirectory);
			menu->AddItem(submenu);
		} else {
			//BMessage* message = new BMessage(kChangeKeyboardLayout);
			//message->AddRed("ref",&ref);
			menu->AddItem(new BMenuItem(ref.name, NULL));
		}
	}
}


void
VirtualKeyboardWindow::_LoadMaps()
{
	BPath path;
	if (find_directory(B_SYSTEM_DATA_DIRECTORY, &path) != B_OK)
		return;

	path.Append("Keymaps");
	BDirectory directory;
	entry_ref ref;

	if (directory.SetTo(path.Path()) == B_OK) {
		while (directory.GetNextRef(&ref) == B_OK) {
			fMapListView->AddItem(new KeymapListItem(ref));	
		}
	}
	fMapListView->SortItems(&compare_key_list_items);
}


void
VirtualKeyboardWindow::_LoadFonts()
{
	int32 numFamilies = count_font_families();
	font_family family, currentFamily;
	font_style currentStyle;
	uint32 flags;

	be_plain_font->GetFamilyAndStyle(&currentFamily,&currentStyle);

	for (int32 i = 0; i< numFamilies; i++) {
		if (get_font_family(i, &family, &flags) == B_OK) {
			BMenuItem* item = new BMenuItem(family, NULL);	//new BMessage(kMsgMenuFontChanged));
			fFontMenu->AddItem(item);
			if (!strcmp(family, currentFamily))
				item->SetMarked(true);
		}
	}
}


void
VirtualKeyboardWindow::MessageReceived(BMessage* message)
{
	BWindow::MessageReceived(message);
}
