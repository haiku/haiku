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

#include <MenuBar.h>
#include <MenuItem.h>
#include <FilePanel.h>
#include <NodeInfo.h>
#include <Alert.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Font.h>
#include <Clipboard.h>

#include "PeopleApp.h"
#include "PeopleView.h"
#include "PeopleWindow.h"

#include <stdio.h>
#include <string.h>


TPeopleWindow::TPeopleWindow(BRect frame, char *title, entry_ref *ref)
	: BWindow(frame, title, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE),
	fPanel(NULL)
{
	BMenu* menu;
	BMenuItem* item;

	BRect rect(0, 0, 32767, 15);
	BMenuBar* menuBar = new BMenuBar(rect, "");
	menu = new BMenu("File");
	menu->AddItem(item = new BMenuItem("New Person", new BMessage(M_NEW), 'N'));
	item->SetTarget(NULL, be_app);
	menu->AddItem(new BMenuItem("Close", new BMessage(B_CLOSE_REQUESTED), 'W'));
	menu->AddSeparatorItem();
	menu->AddItem(fSave = new BMenuItem("Save", new BMessage(M_SAVE), 'S'));
	fSave->SetEnabled(FALSE);
	menu->AddItem(new BMenuItem("Save As"B_UTF8_ELLIPSIS, new BMessage(M_SAVE_AS)));
	menu->AddItem(fRevert = new BMenuItem("Revert", new BMessage(M_REVERT), 'R'));
	fRevert->SetEnabled(FALSE);
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
	menuBar->AddItem(menu);

	menu = new BMenu("Edit");
	menu->AddItem(fUndo = new BMenuItem("Undo", new BMessage(M_UNDO), 'Z'));
	menu->AddSeparatorItem();
	menu->AddItem(fCut = new BMenuItem("Cut", new BMessage(B_CUT), 'X'));
	fCut->SetTarget(NULL, this);
	menu->AddItem(fCopy = new BMenuItem("Copy", new BMessage(B_COPY), 'C'));
	fCopy->SetTarget(NULL, this);
	menu->AddItem(fPaste = new BMenuItem("Paste", new BMessage(B_PASTE), 'V'));
	fPaste->SetTarget(NULL, this);
	menu->AddItem(item = new BMenuItem("Select All", new BMessage(M_SELECT), 'A'));
	item->SetTarget(NULL, this);
	menuBar->AddItem(menu);
	AddChild(menuBar);

	if (ref) {
		fRef = new entry_ref(*ref);
		SetTitle(ref->name);
	} else
		fRef = NULL;

	rect = Frame();
	rect.OffsetTo(0, menuBar->Bounds().bottom + 1);
	fView = new TPeopleView(rect, "PeopleView", fRef);

	AddChild(fView);
	ResizeTo(fView->Frame().right, fView->Frame().bottom);
}


TPeopleWindow::~TPeopleWindow(void)
{
	delete fRef;
	delete fPanel;
}


void
TPeopleWindow::MenusBeginning(void)
{
	bool enabled;

	enabled = fView->CheckSave();
	fSave->SetEnabled(enabled);
	fRevert->SetEnabled(enabled);

	fUndo->SetEnabled(false);
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
						if (fRef)
							delete fRef;
						fRef = new entry_ref(dir);
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

		default:
			BWindow::MessageReceived(msg);
	}
}


bool
TPeopleWindow::QuitRequested(void)
{
	int32			count = 0;
	int32			index = 0;
	BPoint			pos;
	BRect			r;
	status_t		result;
	TPeopleWindow	*window;

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

	while ((window = (TPeopleWindow *)be_app->WindowAt(index++))) {
		if (window->FindView("PeopleView"))
			count++;
	}

	if (count == 1) {
		r = Frame();
		pos = r.LeftTop();
		if (((TPeopleApp*)be_app)->fPrefs) {
			((TPeopleApp*)be_app)->fPrefs->Seek(0, 0);
			((TPeopleApp*)be_app)->fPrefs->Write(&pos, sizeof(BPoint));
		}
		be_app->PostMessage(B_QUIT_REQUESTED);
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
