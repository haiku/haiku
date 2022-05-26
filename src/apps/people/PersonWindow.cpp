/*
 * Copyright 2005-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Robert Polic
 *		Stephan Aßmus <superstippi@gmx.de>
 *
 * Copyright 1999, Be Incorporated.   All Rights Reserved.
 * This file may be used under the terms of the Be Sample Code License.
 */

#include "PersonWindow.h"

#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <Catalog.h>
#include <Clipboard.h>
#include <ControlLook.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <Locale.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Screen.h>
#include <ScrollView.h>
#include <String.h>
#include <TextView.h>
#include <Volume.h>

#include "PeopleApp.h"
#include "PersonView.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "People"


PersonWindow::PersonWindow(BRect frame, const char* title,
		const char* nameAttribute, const char* categoryAttribute,
		const entry_ref* ref)
	:
	BWindow(frame, title, B_TITLED_WINDOW, B_NOT_ZOOMABLE
		| B_AUTO_UPDATE_SIZE_LIMITS),
	fRef(NULL),
	fPanel(NULL),
	fNameAttribute(nameAttribute)
{
	BMenu* menu;
	BMenuItem* item;

	BMenuBar* menuBar = new BMenuBar("");
	menu = new BMenu(B_TRANSLATE("File"));
	menu->AddItem(item = new BMenuItem(
		B_TRANSLATE("New person" B_UTF8_ELLIPSIS),
		new BMessage(M_NEW), 'N'));
	item->SetTarget(be_app);
	menu->AddItem(new BMenuItem(B_TRANSLATE("Close"),
		new BMessage(B_QUIT_REQUESTED), 'W'));
	menu->AddSeparatorItem();
	menu->AddItem(fSave = new BMenuItem(B_TRANSLATE("Save"),
		new BMessage(M_SAVE), 'S'));
	fSave->SetEnabled(FALSE);
	menu->AddItem(new BMenuItem(
		B_TRANSLATE("Save as" B_UTF8_ELLIPSIS),
		new BMessage(M_SAVE_AS)));
	menu->AddItem(fRevert = new BMenuItem(B_TRANSLATE("Revert"),
		new BMessage(M_REVERT), 'R'));
	fRevert->SetEnabled(FALSE);
	menu->AddSeparatorItem();
	item = new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(B_QUIT_REQUESTED), 'Q');
	item->SetTarget(be_app);
	menu->AddItem(item);
	menuBar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Edit"));
	menu->AddItem(fUndo = new BMenuItem(B_TRANSLATE("Undo"),
		new BMessage(B_UNDO), 'Z'));
	fUndo->SetEnabled(false);
	menu->AddSeparatorItem();
	menu->AddItem(fCut = new BMenuItem(B_TRANSLATE("Cut"),
		new BMessage(B_CUT), 'X'));
	menu->AddItem(fCopy = new BMenuItem(B_TRANSLATE("Copy"),
		new BMessage(B_COPY), 'C'));
	menu->AddItem(fPaste = new BMenuItem(B_TRANSLATE("Paste"),
		new BMessage(B_PASTE), 'V'));
	BMenuItem* selectAllItem = new BMenuItem(B_TRANSLATE("Select all"),
		new BMessage(M_SELECT), 'A');
	menu->AddItem(selectAllItem);
	menu->AddSeparatorItem();
	menu->AddItem(item = new BMenuItem(B_TRANSLATE("Configure attributes"),
		new BMessage(M_CONFIGURE_ATTRIBUTES), 'F'));
	item->SetTarget(be_app);
	menuBar->AddItem(menu);

	if (ref != NULL) {
		SetTitle(ref->name);
		_SetToRef(new entry_ref(*ref));
	} else
		_SetToRef(NULL);

	fView = new PersonView("PeopleView", categoryAttribute, fRef);

	BScrollView* scrollView = new BScrollView("PeopleScrollView", fView, 0,
		false, true, B_NO_BORDER);
	scrollView->SetExplicitMinSize(BSize(scrollView->MinSize().width, 0));

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(0, 0, -1, 0)
		.Add(menuBar)
		.Add(scrollView);

	fRevert->SetTarget(fView);
	selectAllItem->SetTarget(fView);
}


PersonWindow::~PersonWindow()
{
	_SetToRef(NULL);
}


void
PersonWindow::MenusBeginning()
{
	bool enabled = !fView->IsSaved();
	fSave->SetEnabled(enabled);
	fRevert->SetEnabled(enabled);

	bool isRedo = false;
	bool undoEnabled = false;
	bool cutAndCopyEnabled = false;

	BTextView* textView = dynamic_cast<BTextView*>(CurrentFocus());
	if (textView != NULL) {
		undo_state state = textView->UndoState(&isRedo);
		undoEnabled = state != B_UNDO_UNAVAILABLE;

		cutAndCopyEnabled = fView->IsTextSelected();
	}

	if (isRedo)
		fUndo->SetLabel(B_TRANSLATE("Redo"));
	else
		fUndo->SetLabel(B_TRANSLATE("Undo"));
	fUndo->SetEnabled(undoEnabled);
	fCut->SetEnabled(cutAndCopyEnabled);
	fCopy->SetEnabled(cutAndCopyEnabled);

	be_clipboard->Lock();
	fPaste->SetEnabled(be_clipboard->Data()->HasData("text/plain", B_MIME_TYPE));
	be_clipboard->Unlock();

	fView->BuildGroupMenu();
}


void
PersonWindow::MessageReceived(BMessage* msg)
{
	char			str[256];
	BDirectory		directory;
	BEntry			entry;
	BFile			file;
	BNodeInfo		*node;

	switch (msg->what) {
		case M_SAVE:
			if (!fRef) {
				SaveAs();
				break;
			}
			// supposed to fall through
		case M_REVERT:
		case M_SELECT:
			fView->MessageReceived(msg);
			break;

		case M_SAVE_AS:
			SaveAs();
			break;

		case B_UNDO: // fall through
		case B_CUT:
		case B_COPY:
		case B_PASTE:
		{
			BView* view = CurrentFocus();
			if (view != NULL)
				view->MessageReceived(msg);
			break;
		}

		case B_SAVE_REQUESTED:
		{
			entry_ref dir;
			if (msg->FindRef("directory", &dir) == B_OK) {
				const char* name = NULL;
				msg->FindString("name", &name);
				directory.SetTo(&dir);
				if (directory.InitCheck() == B_NO_ERROR) {
					directory.CreateFile(name, &file);
					if (file.InitCheck() == B_NO_ERROR) {
						node = new BNodeInfo(&file);
						node->SetType("application/x-person");
						delete node;

						directory.FindEntry(name, &entry);
						entry.GetRef(&dir);
						_SetToRef(new entry_ref(dir));
						SetTitle(fRef->name);
						fView->CreateFile(fRef);
					}
					else {
						sprintf(str, B_TRANSLATE("Could not create %s."), name);
						BAlert* alert = new BAlert("", str, B_TRANSLATE("Sorry"));
						alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
						alert->Go();
					}
				}
			}
			break;
		}

		case B_NODE_MONITOR:
		{
			int32 opcode;
			if (msg->FindInt32("opcode", &opcode) == B_OK) {
				switch (opcode) {
					case B_ENTRY_REMOVED:
						// We lost our file. Close the window.
						PostMessage(B_QUIT_REQUESTED);
						break;

					case B_ENTRY_MOVED:
					{
						// We may have renamed our entry. Obtain relevant data
						// from message.
						BString name;
						msg->FindString("name", &name);

						int64 directory;
						msg->FindInt64("to directory", &directory);

						int32 device;
						msg->FindInt32("device", &device);

						// Update our ref.
						delete fRef;
						fRef = new entry_ref(device, directory, name.String());

						// And our window title.
						SetTitle(name);

						// If moved to Trash, close window.
						BVolume volume(device);
						BPath trash;
						find_directory(B_TRASH_DIRECTORY, &trash, false,
							&volume);
						BPath folder(fRef);
						folder.GetParent(&folder);
						if (folder == trash)
							PostMessage(B_QUIT_REQUESTED);

						break;
					}

					case B_ATTR_CHANGED:
					{
						// An attribute was updated.
						BString attr;
						if (msg->FindString("attr", &attr) == B_OK)
							fView->SetAttribute(attr.String(), true);
						break;
					}
					case B_STAT_CHANGED:
						fView->UpdatePicture(fRef);
						break;
				}
			}
			break;
		}

		default:
			BWindow::MessageReceived(msg);
	}
}


bool
PersonWindow::QuitRequested()
{
	status_t result;

	if (!fView->IsSaved()) {
		BAlert* alert = new BAlert("",
			B_TRANSLATE("Save changes before closing?"), B_TRANSLATE("Cancel"),
			B_TRANSLATE("Don't save"), B_TRANSLATE("Save"),
			B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
		alert->SetShortcut(0, B_ESCAPE);
		alert->SetShortcut(1, 'd');
		alert->SetShortcut(2, 's');
		result = alert->Go();

		if (result == 2) {
			if (fRef)
				fView->Save();
			else {
				SaveAs();
				return false;
			}
		} else if (result == 0)
			return false;
	}

	delete fPanel;

	BMessage message(M_WINDOW_QUITS);
	message.AddRect("frame", Frame());
	if (be_app->Lock()) {
		be_app->PostMessage(&message);
		be_app->Unlock();
	}

	return true;
}


void
PersonWindow::Show()
{
	BRect screenFrame = BScreen(this).Frame();
	if (Frame().bottom > screenFrame.bottom)
		ResizeBy(0, screenFrame.bottom - Frame().bottom - 10);
	fView->MakeFocus();
	BWindow::Show();
}


void
PersonWindow::AddAttribute(const char* label, const char* attribute)
{
	fView->AddAttribute(label, attribute);
}


void
PersonWindow::SaveAs()
{
	char name[B_FILE_NAME_LENGTH];
	_GetDefaultFileName(name);

	if (fPanel == NULL) {
		BMessenger target(this);
		fPanel = new BFilePanel(B_SAVE_PANEL, &target);

		BPath path;
		find_directory(B_USER_DIRECTORY, &path, true);

		BDirectory dir;
		dir.SetTo(path.Path());

		BEntry entry;
		if (dir.FindEntry("people", &entry) == B_OK
			|| (dir.CreateDirectory("people", &dir) == B_OK
					&& dir.GetEntry(&entry) == B_OK)) {
			fPanel->SetPanelDirectory(&entry);
		}
	}

	if (fPanel->Window()->Lock()) {
		fPanel->SetSaveText(name);
		if (fPanel->Window()->IsHidden())
			fPanel->Window()->Show();
		else
			fPanel->Window()->Activate();
		fPanel->Window()->Unlock();
	}
}


bool
PersonWindow::RefersPersonFile(const entry_ref& ref) const
{
	if (fRef == NULL)
		return false;
	return *fRef == ref;
}


void
PersonWindow::_GetDefaultFileName(char* name)
{
	strncpy(name, fView->AttributeValue(fNameAttribute), B_FILE_NAME_LENGTH);
	while (*name) {
		if (*name == '/')
			*name = '-';
		name++;
	}
}


void
PersonWindow::_SetToRef(entry_ref* ref)
{
	if (fRef != NULL) {
		_WatchChanges(false);
		delete fRef;
	}

	fRef = ref;

	_WatchChanges(true);
}


void
PersonWindow::_WatchChanges(bool enable)
{
	if (fRef == NULL)
		return;

	node_ref nodeRef;

	BNode node(fRef);
	node.GetNodeRef(&nodeRef);

	uint32 flags;
	BString action;

	if (enable) {
		// Start watching.
		flags = B_WATCH_ALL;
		action = "starting";
	} else {
		// Stop watching.
		flags = B_STOP_WATCHING;
		action = "stoping";
	}

	if (watch_node(&nodeRef, flags, this) != B_OK) {
		printf("Error %s node monitor.\n", action.String());
	}
}
