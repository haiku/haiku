/*
 * Copyright 2003-2008, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jérôme Duval
 *		Oliver Ruiz Dorantes
 *		Atsushi Takamatsu
 */


#include "HWindow.h"
#include "HEventList.h"

#include <stdio.h>

#include <Alert.h>
#include <Application.h>
#include <Beep.h>
#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <ControlLook.h>
#include <FindDirectory.h>
#include <fs_attr.h>
#include <GroupLayoutBuilder.h>
#include <Locale.h>
#include <MediaFiles.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>
#include <ScrollView.h>
#include <StringView.h>
#include <Sound.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "HWindow"

static const char kSettingsFile[] = "Sounds_Settings";


HWindow::HWindow(BRect rect, const char* name)
	:
	BWindow(rect, name, B_TITLED_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS),
	fFilePanel(NULL),
	fPlayer(NULL)
{
	InitGUI();

	fFilePanel = new BFilePanel();
	fFilePanel->SetTarget(this);

	BPath path;
	BMessage msg;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(kSettingsFile);
		BFile file(path.Path(), B_READ_ONLY);

		if (file.InitCheck() == B_OK && msg.Unflatten(&file) == B_OK
			&& msg.FindRect("frame", &fFrame) == B_OK) {
			MoveTo(fFrame.LeftTop());
			ResizeTo(fFrame.Width(), fFrame.Height());
		}
	}
}


HWindow::~HWindow()
{
	delete fFilePanel;
	delete fPlayer;

	BPath path;
	BMessage msg;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) == B_OK) {
		path.Append(kSettingsFile);
		BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE);

		if (file.InitCheck() == B_OK) {
			msg.AddRect("frame", fFrame);
			msg.Flatten(&file);
		}
	}
}


void
HWindow::InitGUI()
{
	fEventList = new HEventList();
	fEventList->SetType(BMediaFiles::B_SOUNDS);
	fEventList->SetSelectionMode(B_SINGLE_SELECTION_LIST);

	BGroupView* view = new BGroupView();
	BBox* box = new BBox("", B_WILL_DRAW | B_FRAME_EVENTS
		| B_NAVIGABLE_JUMP | B_PULSE_NEEDED);

	BMenu* menu = new BMenu("file");
	menu->SetRadioMode(true);
	menu->SetLabelFromMarked(true);
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("<none>"),
		new BMessage(M_NONE_MESSAGE)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Other" B_UTF8_ELLIPSIS),
		new BMessage(M_OTHER_MESSAGE)));
	BMenuField* menuField = new BMenuField("filemenu",
		B_TRANSLATE("Sound File:"), menu);
	menuField->SetDivider(menuField->StringWidth(B_TRANSLATE("Sound File:"))
		+ 10);

	BButton* stopbutton = new BButton("stop", B_TRANSLATE("Stop"),
		new BMessage(M_STOP_MESSAGE));
	stopbutton->SetEnabled(false);

	BButton* playbutton = new BButton("play", B_TRANSLATE("Play"),
		new BMessage(M_PLAY_MESSAGE));
	playbutton->SetEnabled(false);

	const float kInset = be_control_look->DefaultItemSpacing();
	view->SetLayout(new BGroupLayout(B_HORIZONTAL));
	view->AddChild(BGroupLayoutBuilder(B_VERTICAL, kInset)
		.AddGroup(B_HORIZONTAL)
			.Add(menuField)
			.AddGlue()
		.End()
		.AddGroup(B_HORIZONTAL, kInset)
			.AddGlue()
			.Add(playbutton)
			.Add(stopbutton)
		.End()
		.SetInsets(kInset, kInset, kInset, kInset)
	);

	box->AddChild(view);

	SetLayout(new BGroupLayout(B_HORIZONTAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.AddGroup(B_VERTICAL, kInset)
			.Add(fEventList)
			.Add(box)
		.End()
		.SetInsets(kInset, kInset, kInset, kInset)
	);

	// setup file menu
	SetupMenuField();
	BMenuItem* noneItem = menu->FindItem(B_TRANSLATE("<none>"));
	if (noneItem != NULL)
		noneItem->SetMarked(true);
}


void
HWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case M_OTHER_MESSAGE:
		{
			BMenuField* menufield
				= dynamic_cast<BMenuField*>(FindView("filemenu"));
			if (menufield == NULL)
				return;
			BMenu* menu = menufield->Menu();

			HEventRow* row = (HEventRow*)fEventList->CurrentSelection();
			if (row != NULL) {
				BPath path(row->Path());
				if (path.InitCheck() != B_OK) {
					BMenuItem* item = menu->FindItem(B_TRANSLATE("<none>"));
					if (item != NULL)
						item->SetMarked(true);
				} else {
					BMenuItem* item = menu->FindItem(path.Leaf());
					if (item != NULL)
						item->SetMarked(true);
				}
			}
			fFilePanel->Show();
			break;
		}

		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
		{
			entry_ref ref;
			HEventRow* row = (HEventRow*)fEventList->CurrentSelection();
			if (message->FindRef("refs", &ref) == B_OK && row != NULL) {
				BMenuField* menufield
					= dynamic_cast<BMenuField*>(FindView("filemenu"));
				if (menufield == NULL)
					return;
				BMenu* menu = menufield->Menu();

				// check audio file
				BNode node(&ref);
				BNodeInfo ninfo(&node);
				char type[B_MIME_TYPE_LENGTH + 1];
				ninfo.GetType(type);
				BMimeType mtype(type);
				BMimeType superType;
				mtype.GetSupertype(&superType);
				if (superType.Type() == NULL
					|| strcmp(superType.Type(), "audio") != 0) {
					beep();
					BAlert* alert = new BAlert("",
						B_TRANSLATE("This is not an audio file."),
						B_TRANSLATE("OK"), NULL, NULL,
						B_WIDTH_AS_USUAL, B_STOP_ALERT);
					alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
					alert->Go();
					break;
				}

				// add file item
				BMessage* msg = new BMessage(M_ITEM_MESSAGE);
				BPath path(&ref);
				msg->AddRef("refs", &ref);
				BMenuItem* menuitem = menu->FindItem(path.Leaf());
				if (menuitem == NULL)
					menu->AddItem(menuitem = new BMenuItem(path.Leaf(), msg), 0);
				// refresh item
				fEventList->SetPath(BPath(&ref).Path());
				// check file menu
				if (menuitem != NULL)
					menuitem->SetMarked(true);
			}
			break;
		}

		case M_PLAY_MESSAGE:
		{
			HEventRow* row = (HEventRow*)fEventList->CurrentSelection();
			if (row != NULL) {
				const char* path = row->Path();
				if (path != NULL) {
					entry_ref ref;
					::get_ref_for_path(path, &ref);
					delete fPlayer;
					fPlayer = new BFileGameSound(&ref, false);
					fPlayer->StartPlaying();
				}
			}
			break;
		}

		case M_STOP_MESSAGE:
		{
			if (fPlayer == NULL)
				break;
			if (fPlayer->IsPlaying()) {
				fPlayer->StopPlaying();
				delete fPlayer;
				fPlayer = NULL;
			}
			break;
		}

		case M_EVENT_CHANGED:
		{
			const char* path;
			BMenuField* menufield
				= dynamic_cast<BMenuField*>(FindView("filemenu"));
			if (menufield == NULL)
				return;
			BMenu* menu = menufield->Menu();

			if (message->FindString("path", &path) == B_OK) {
				BPath path(path);
				if (path.InitCheck() != B_OK) {
					BMenuItem* item = menu->FindItem(B_TRANSLATE("<none>"));
					if (item != NULL)
						item->SetMarked(true);
				} else {
					BMenuItem* item = menu->FindItem(path.Leaf());
					if (item != NULL)
						item->SetMarked(true);
				}
			}
			break;
		}

		case M_ITEM_MESSAGE:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK)
				fEventList->SetPath(BPath(&ref).Path());
			break;
		}

		case M_NONE_MESSAGE:
		{
			fEventList->SetPath(NULL);
			break;
		}

		default:
			BWindow::MessageReceived(message);
	}
}


void
HWindow::SetupMenuField()
{
	BMenuField* menufield = dynamic_cast<BMenuField*>(FindView("filemenu"));
	if (menufield == NULL)
		return;
	BMenu* menu = menufield->Menu();
	int32 count = fEventList->CountRows();
	for (int32 i = 0; i < count; i++) {
		HEventRow* row = (HEventRow*)fEventList->RowAt(i);
		if (row == NULL)
			continue;

		BPath path(row->Path());
		if (path.InitCheck() != B_OK)
			continue;
		if (menu->FindItem(path.Leaf()))
			continue;

		BMessage* msg = new BMessage(M_ITEM_MESSAGE);
		entry_ref ref;
		::get_ref_for_path(path.Path(), &ref);
		msg->AddRef("refs", &ref);
		menu->AddItem(new BMenuItem(path.Leaf(), msg), 0);
	}

	BPath path;
	BDirectory dir;
	BEntry entry;
	BPath item_path;

	status_t err = find_directory(B_BEOS_SOUNDS_DIRECTORY, &path);
	if (err == B_OK)
		err = dir.SetTo(path.Path());
	while (err == B_OK) {
		err = dir.GetNextEntry(&entry, true);
		if (entry.InitCheck() != B_NO_ERROR)
			break;

		entry.GetPath(&item_path);

		if (menu->FindItem(item_path.Leaf()))
			continue;

		BMessage* msg = new BMessage(M_ITEM_MESSAGE);
		entry_ref ref;
		::get_ref_for_path(item_path.Path(), &ref);
		msg->AddRef("refs", &ref);
		menu->AddItem(new BMenuItem(item_path.Leaf(), msg), 0);
	}

	err = find_directory(B_USER_SOUNDS_DIRECTORY, &path);
	if (err == B_OK)
		err = dir.SetTo(path.Path());
	while (err == B_OK) {
		err = dir.GetNextEntry(&entry, true);
		if (entry.InitCheck() != B_NO_ERROR)
			break;

		entry.GetPath(&item_path);

		if (menu->FindItem(item_path.Leaf()))
			continue;

		BMessage* msg = new BMessage(M_ITEM_MESSAGE);
		entry_ref ref;

		::get_ref_for_path(item_path.Path(), &ref);
		msg->AddRef("refs", &ref);
		menu->AddItem(new BMenuItem(item_path.Leaf(), msg), 0);
	}

	err = find_directory(B_COMMON_SOUNDS_DIRECTORY, &path);
	if (err == B_OK)
		err = dir.SetTo(path.Path());
	while (err == B_OK) {
		err = dir.GetNextEntry(&entry, true);
		if (entry.InitCheck() != B_NO_ERROR)
			break;

		entry.GetPath(&item_path);

		if (menu->FindItem(item_path.Leaf()))
			continue;

		BMessage* msg = new BMessage(M_ITEM_MESSAGE);
		entry_ref ref;

		::get_ref_for_path(item_path.Path(), &ref);
		msg->AddRef("refs", &ref);
		menu->AddItem(new BMenuItem(item_path.Leaf(), msg), 0);
	}

}


void
HWindow::Pulse()
{
	HEventRow* row = (HEventRow*)fEventList->CurrentSelection();
	BMenuField* menufield = dynamic_cast<BMenuField*>(FindView("filemenu"));
	BButton* button = dynamic_cast<BButton*>(FindView("play"));
	BButton* stop = dynamic_cast<BButton*>(FindView("stop"));

	if (menufield == NULL || button == NULL || stop == NULL)
		return;

	if (row != NULL) {
		menufield->SetEnabled(true);

		const char* path = row->Path();
		if (path != NULL && strcmp(path, ""))
			button->SetEnabled(true);
		else
			button->SetEnabled(false);
	} else {
		menufield->SetEnabled(false);
		button->SetEnabled(false);
	}

	if (fPlayer != NULL) {
		if (fPlayer->IsPlaying())
			stop->SetEnabled(true);
		else
			stop->SetEnabled(false);
	} else
		stop->SetEnabled(false);
}


void
HWindow::DispatchMessage(BMessage* message, BHandler* handler)
{
	if (message->what == B_PULSE)
		Pulse();
	BWindow::DispatchMessage(message, handler);
}


bool
HWindow::QuitRequested()
{
	fFrame = Frame();

	fEventList->RemoveAll();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}
