//--------------------------------------------------------------------
//	
//	PeopleWindow.cpp
//
//	Written by: Robert Polic
//	
//--------------------------------------------------------------------
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "PeopleWindow.h"

#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <Clipboard.h>
#include <ControlLook.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <String.h>
#include <TextView.h>
#include <Volume.h>

#include "PeopleApp.h"
#include "PeopleView.h"


TPeopleWindow::TPeopleWindow(BRect frame, const char *title, entry_ref *ref)
	:
	BWindow(frame, title, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE
		| B_AUTO_UPDATE_SIZE_LIMITS),
	fPanel(NULL)
{
	BMenu* menu;
	BMenuItem* item;

	BMenuBar* menuBar = new BMenuBar("");
	menu = new BMenu("File");
	menu->AddItem(item = new BMenuItem("New person" B_UTF8_ELLIPSIS,
		new BMessage(M_NEW), 'N'));
	item->SetTarget(NULL, be_app);
	menu->AddItem(new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED), 'W'));
	menu->AddSeparatorItem();
	menu->AddItem(fSave = new BMenuItem("Save", new BMessage(M_SAVE), 'S'));
	fSave->SetEnabled(FALSE);
	menu->AddItem(new BMenuItem("Save as"B_UTF8_ELLIPSIS,
		new BMessage(M_SAVE_AS)));
	menu->AddItem(fRevert = new BMenuItem("Revert",
		new BMessage(M_REVERT), 'R'));
	fRevert->SetEnabled(FALSE);
	menu->AddSeparatorItem();
	item = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q');
	item->SetTarget(NULL, be_app);
	menu->AddItem(item);
	menuBar->AddItem(menu);

	menu = new BMenu("Edit");
	menu->AddItem(fUndo = new BMenuItem("Undo", new BMessage(B_UNDO), 'Z'));
	fUndo->SetTarget(NULL, this);
	fUndo->SetEnabled(false);
	menu->AddSeparatorItem();
	menu->AddItem(fCut = new BMenuItem("Cut", new BMessage(B_CUT), 'X'));
	fCut->SetTarget(NULL, this);
	menu->AddItem(fCopy = new BMenuItem("Copy", new BMessage(B_COPY), 'C'));
	fCopy->SetTarget(NULL, this);
	menu->AddItem(fPaste = new BMenuItem("Paste", new BMessage(B_PASTE), 'V'));
	fPaste->SetTarget(NULL, this);
	menu->AddItem(item = new BMenuItem("Select all",
		new BMessage(M_SELECT), 'A'));
	item->SetTarget(NULL, this);
	menuBar->AddItem(menu);

	if (ref) {
		fRef = new entry_ref(*ref);
		SetTitle(ref->name);
		WatchChanges(true);
	} else
		fRef = NULL;

	fView = new TPeopleView("PeopleView", fRef);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.SetInsets(0, 0, 0, 0)
		.Add(menuBar)
		.Add(fView);
}


TPeopleWindow::~TPeopleWindow(void)
{
	if (fRef)
		WatchChanges(false);

	delete fRef;
}


void
TPeopleWindow::MenusBeginning(void)
{
	bool enabled;
	bool isRedo;

	enabled = fView->CheckSave();
	fSave->SetEnabled(enabled);
	fRevert->SetEnabled(enabled);

	undo_state state = ((BTextView *)CurrentFocus())->UndoState(&isRedo);
	fUndo->SetEnabled(state != B_UNDO_UNAVAILABLE);

	if (isRedo)
		fUndo->SetLabel("Redo");
	else
		fUndo->SetLabel("Undo");

	enabled = fView->TextSelected();
	fCut->SetEnabled(enabled);
	fCopy->SetEnabled(enabled);

	be_clipboard->Lock();
	fPaste->SetEnabled(be_clipboard->Data()->HasData("text/plain", B_MIME_TYPE));
	be_clipboard->Unlock();

	fView->BuildGroupMenu();
}


void
TPeopleWindow::MessageReceived(BMessage* msg)
{
	char			str[256];
	entry_ref		dir;
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
		case M_REVERT:
		case M_SELECT:
			fView->MessageReceived(msg);
			break;

		case M_SAVE_AS:
			SaveAs();
			break;

		case M_GROUP_MENU:
		{
			char *name = NULL;
			msg->FindString("group", (const char **)&name);
			fView->SetField(F_GROUP, name, FALSE);
			break;
		}
		case B_SAVE_REQUESTED:
			if (msg->FindRef("directory", &dir) == B_NO_ERROR) {
				const char *name = NULL;
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
						if (fRef) {
							WatchChanges(false);
							delete fRef;
						}
						fRef = new entry_ref(dir);
						WatchChanges(true);
						SetTitle(fRef->name);
						fView->NewFile(fRef);
					}
					else {
						sprintf(str, "Could not create %s.", name);
						(new BAlert("", str, "Sorry"))->Go();
					}
				}
			}
			break;
			
		case B_NODE_MONITOR:
		{
			int32 opcode;
			if (msg->FindInt32("opcode", &opcode) == B_OK) {
				switch (opcode) {
					case B_ENTRY_REMOVED:
						// We lost our file. Close the window by quiting its
						// looper.
						delete fRef;
						fRef = NULL;
						
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
						if (folder == trash) {
							delete fRef;
							fRef = NULL;
							PostMessage(B_QUIT_REQUESTED);
						}

						break;
					}
					
					case B_ATTR_CHANGED:
					{
						// An attribute was updated.
						BString attr;
						if (msg->FindString("attr", &attr) == B_OK) {
							for (int32 i = 0; gFields[i].attribute; i++) {
								if (attr == gFields[i].attribute) {
									fView->SetField(i, true);
								}
							}
						}
						break;
					}							
				}
			}
			break;
		}

		default:
			BWindow::MessageReceived(msg);
	}
}


bool
TPeopleWindow::QuitRequested(void)
{
	status_t result;

	if (fView->CheckSave()) {
		result = (new BAlert("", "Save changes before quitting?",
							"Cancel", "Quit", "Save"))->Go();
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
TPeopleWindow::DefaultName(char *name)
{
	strncpy(name, fView->GetField(F_NAME), B_FILE_NAME_LENGTH);
	while (*name) {
		if (*name == '/')
			*name = '-';
		name++;
	}
}


void
TPeopleWindow::SetField(int32 index, char *text)
{
	fView->SetField(index, text, true);
}


void
TPeopleWindow::SaveAs(void)
{
	char		name[B_FILE_NAME_LENGTH];
	BDirectory	dir;
	BEntry		entry;
	BMessenger	window(this);
	BPath		path;

	DefaultName(name);
	if (!fPanel) {
		fPanel = new BFilePanel(B_SAVE_PANEL, &window);
		fPanel->SetSaveText(name);
		find_directory(B_USER_DIRECTORY, &path, true);
		dir.SetTo(path.Path());
		if (dir.FindEntry("people", &entry) == B_NO_ERROR)
			fPanel->SetPanelDirectory(&entry);
		else if (dir.CreateDirectory("people", &dir) == B_NO_ERROR) {
			dir.GetEntry(&entry);
			fPanel->SetPanelDirectory(&entry);
		}
	}
	else if (fPanel->Window()->Lock()) {
		if (!fPanel->Window()->IsHidden())
			fPanel->Window()->Activate();
		else
			fPanel->SetSaveText(name);
		fPanel->Window()->Unlock();
	}

	if (fPanel->Window()->Lock()) {
		if (fPanel->Window()->IsHidden())
			fPanel->Window()->Show();
		fPanel->Window()->Unlock();
	}	
}


void
TPeopleWindow::WatchChanges(bool enable)
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
