/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "MainWindow.h"

#include "Constants.h"
#include "ImageButton.h"
#include "ResourceListView.h"
#include "ResourceRow.h"

#include <Application.h>
#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <Entry.h>
#include <FilePanel.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Resources.h>
#include <StatusBar.h>
#include <String.h>
#include <TextControl.h>
#include <TranslationUtils.h>

#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fstream>
using namespace std;


MainWindow::MainWindow(BRect frame, BEntry* assocEntry)
	:
	BWindow(frame, NULL, B_DOCUMENT_WINDOW, 0)
{
	fAssocEntry = assocEntry;

	fSavePanel = NULL;

	fMenuBar = new BMenuBar(BRect(0, 0, 600, 1), "fMenuBar");

	fFileMenu = new BMenu("File", B_ITEMS_IN_COLUMN);
	fNewItem = new BMenuItem("New", new BMessage(MSG_NEW), 'N');
	fOpenItem = new BMenuItem("Open", new BMessage(MSG_OPEN), 'O');
	fCloseItem = new BMenuItem("Close", new BMessage(MSG_CLOSE), 'W');
	fSaveItem = new BMenuItem("Save", new BMessage(MSG_SAVE), 'S');
	fSaveAsItem = new BMenuItem("Save As" B_UTF8_ELLIPSIS, new
		BMessage(MSG_SAVEAS));
	fSaveAllItem = new BMenuItem("Save All", new BMessage(MSG_SAVEALL), 'S',
		B_SHIFT_KEY);
	fMergeWithItem = new BMenuItem("Merge With" B_UTF8_ELLIPSIS,
		new BMessage(MSG_MERGEWITH), 'M');
	fQuitItem = new BMenuItem("Quit", new BMessage(MSG_QUIT), 'Q');

	fEditMenu = new BMenu("Edit", B_ITEMS_IN_COLUMN);
	fUndoItem = new BMenuItem("Undo", new BMessage(MSG_UNDO), 'Z');
	fRedoItem = new BMenuItem("Redo", new BMessage(MSG_REDO), 'Y');
	fCutItem = new BMenuItem("Cut", new BMessage(MSG_CUT), 'X');
	fCopyItem = new BMenuItem("Copy", new BMessage(MSG_COPY), 'C');
	fPasteItem = new BMenuItem("Paste", new BMessage(MSG_PASTE), 'V');
	fClearItem = new BMenuItem("Clear", new BMessage(MSG_CLEAR));
	fSelectAllItem = new BMenuItem("Select All",
		new BMessage(MSG_SELECTALL), 'A');

	fHelpMenu = new BMenu("Help", B_ITEMS_IN_COLUMN);

	fResourceIDText = new BTextControl(BRect(0, 0, 47, 23).OffsetBySelf(8, 24),
		"fResourceIDText", NULL, "0", NULL);
	fResourceTypePopUp = new BPopUpMenu("(Type)");
	fResourceTypeMenu = new BMenuField(BRect(0, 0, 1, 1).OffsetBySelf(60, 24),
		"fResourceTypeMenu", NULL, fResourceTypePopUp);

	for (int32 i = 0; kDefaultTypes[i].type != NULL; i++) {
		if (kDefaultTypes[i].size == ~(uint32)0)
			fResourceTypePopUp->AddSeparatorItem();
		else
			fResourceTypePopUp->AddItem(
				new BMenuItem(kDefaultTypes[i].type, NULL));
	}

	fResourceTypePopUp->ItemAt(kDefaultTypeSelected)->SetMarked(true);

	fToolbarView = new BView(BRect(0, 0, 600, 51), "fToolbarView",
		B_FOLLOW_LEFT_RIGHT, 0);

	fToolbarView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BRect toolRect = BRect(0, 0, 23, 23).OffsetBySelf(8, 24);

	fAddButton = new ImageButton(toolRect.OffsetBySelf(200, 0),
		"fAddButton", BTranslationUtils::GetBitmap('PNG ', "add.png"),
		new BMessage(MSG_ADD), B_FOLLOW_NONE);
	fAddButton->ResizeTo(23, 23);

	fRemoveButton = new ImageButton(toolRect.OffsetBySelf(30, 0),
		"fRemoveButton", BTranslationUtils::GetBitmap('PNG ', "remove.png"),
		new BMessage(MSG_REMOVE), B_FOLLOW_NONE);
	fRemoveButton->ResizeTo(23, 23);
	fRemoveButton->SetEnabled(false);

	fMoveUpButton = new ImageButton(toolRect.OffsetBySelf(30, 0),
		"fMoveUpButton", BTranslationUtils::GetBitmap('PNG ', "moveup.png"),
		new BMessage(MSG_MOVEUP), B_FOLLOW_NONE);
	fMoveUpButton->ResizeTo(23, 23);
	fMoveUpButton->SetEnabled(false);

	fMoveDownButton = new ImageButton(toolRect.OffsetBySelf(30, 0),
		"fMoveDownButton", BTranslationUtils::GetBitmap('PNG ', "movedown.png"),
		new BMessage(MSG_MOVEDOWN), B_FOLLOW_NONE);
	fMoveDownButton->ResizeTo(23, 23);
	fMoveDownButton->SetEnabled(false);

	BRect listRect = Bounds();
	listRect.SetLeftTop(BPoint(0, 52));
	listRect.InsetBy(-1, -1);

	fResourceList = new ResourceListView(listRect, "fResourceList",
		B_FOLLOW_ALL, 0);

	fResourceList->AddColumn(new BIntegerColumn("ID", 48, 1, 999,
		B_ALIGN_RIGHT), 0);
	fResourceList->AddColumn(new BStringColumn("Name", 156, 1, 999,
		B_TRUNCATE_END, B_ALIGN_LEFT), 1);
	fResourceList->AddColumn(new BStringColumn("Type", 56, 1, 999,
		B_TRUNCATE_END, B_ALIGN_CENTER), 2);
	fResourceList->AddColumn(new BStringColumn("Code", 56, 1, 999,
		B_TRUNCATE_END, B_ALIGN_CENTER), 3);
	fResourceList->AddColumn(new BStringColumn("Data", 156, 1, 999,
		B_TRUNCATE_END, B_ALIGN_LEFT), 4);
	fResourceList->AddColumn(new BSizeColumn("Size", 74, 1, 999,
		B_ALIGN_RIGHT), 5);

	fResourceList->SetLatchWidth(0);

	fResourceList->SetTarget(this);
	fResourceList->SetSelectionMessage(new BMessage(MSG_SELECTION));

	AddChild(fMenuBar);

	fMenuBar->AddItem(fFileMenu);
		fFileMenu->AddItem(fNewItem);
		fFileMenu->AddItem(fOpenItem);
		fFileMenu->AddItem(fCloseItem);
		fFileMenu->AddSeparatorItem();
		fFileMenu->AddItem(fSaveItem);
		fFileMenu->AddItem(fSaveAsItem);
		fFileMenu->AddItem(fSaveAllItem);
		fFileMenu->AddItem(fMergeWithItem);
		fFileMenu->AddSeparatorItem();
		fFileMenu->AddItem(fQuitItem);

	fMenuBar->AddItem(fEditMenu);
		fEditMenu->AddItem(fUndoItem);
		fEditMenu->AddItem(fRedoItem);
		fEditMenu->AddSeparatorItem();
		fEditMenu->AddItem(fCutItem);
		fEditMenu->AddItem(fCopyItem);
		fEditMenu->AddItem(fPasteItem);
		fEditMenu->AddItem(fClearItem);
		fEditMenu->AddSeparatorItem();
		fEditMenu->AddItem(fSelectAllItem);

	fMenuBar->AddItem(fHelpMenu);

	fToolbarView->AddChild(fResourceIDText);
	fToolbarView->AddChild(fResourceTypeMenu);

	fToolbarView->AddChild(fAddButton);
	fToolbarView->AddChild(fRemoveButton);
	fToolbarView->AddChild(fMoveUpButton);
	fToolbarView->AddChild(fMoveDownButton);

	AddChild(fToolbarView);

	AddChild(fResourceList);

	if (assocEntry != NULL) {
		_SetTitleFromEntry();
		_Load();
	} else {
		SetTitle("ResourceEdit | " B_UTF8_ELLIPSIS);
	}
}


MainWindow::~MainWindow()
{

}


bool
MainWindow::QuitRequested()
{
	// TODO: Check if file is saved.

	BMessage* msg = new BMessage(MSG_CLOSE);
	msg->AddPointer("window", (void*)this);
	be_app->PostMessage(msg);
	return true;
}


void
MainWindow::SelectionChanged()
{
	if (fResourceList->CurrentSelection(NULL) == NULL) {
		fMoveUpButton->SetEnabled(false);
		fMoveDownButton->SetEnabled(false);
		fRemoveButton->SetEnabled(false);
	} else {
		fRemoveButton->SetEnabled(true);
		fMoveUpButton->SetEnabled(!fResourceList->RowAt(0)->IsSelected());
		fMoveDownButton->SetEnabled(!fResourceList->RowAt(
			fResourceList->CountRows() - 1)->IsSelected());
	}
}


void
MainWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MSG_NEW:
			be_app->PostMessage(MSG_NEW);
			break;

		case MSG_OPEN:
			be_app->PostMessage(MSG_OPEN);
			break;

		case MSG_CLOSE:
			PostMessage(B_QUIT_REQUESTED);
			break;

		case MSG_SAVE:
			_Save();
			break;

		case MSG_SAVEAS:
			_SaveAs();
			break;

		case MSG_SAVEAS_DONE:
		{
			entry_ref ref;
			const char* leaf;

			if (msg->FindRef("directory", &ref) != B_OK)
				break;

			if (msg->FindString("name", &leaf) != B_OK)
				break;

			BDirectory dir(&ref);
			_Save(new BEntry(&dir, leaf));

			break;
		}
		case MSG_SAVEALL:
			be_app->PostMessage(MSG_SAVEALL);
			break;

		case MSG_MERGEWITH:
			PRINT(("[MSG_MERGEWITH]: Not yet implemented."));
			// TODO: Implement.
			// "Merge from..." might be a better idea actually.
			break;

		case MSG_QUIT:
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;

		case MSG_UNDO:
			// TODO: Implement.
			PRINT(("[MSG_UNDO]: Not yet implemented."));
			break;

		case MSG_REDO:
			// TODO: Implement.
			PRINT(("[MSG_REDO]: Not yet implemented."));
			break;

		case MSG_CUT:
			// TODO: Implement.
			PRINT(("[MSG_CUT]: Not yet implemented."));
			break;

		case MSG_COPY:
			// TODO: Implement.
			PRINT(("[MSG_COPY]: Not yet implemented."));
			break;

		case MSG_PASTE:
			// TODO: Implement.
			PRINT(("[MSG_PASTE]: Not yet implemented."));
			break;

		case MSG_CLEAR:
			fResourceList->Clear();
			SelectionChanged();
			break;

		case MSG_SELECTALL:
		{
			for (int32 i = 0; i < fResourceList->CountRows(); i++)
				fResourceList->AddToSelection(fResourceList->RowAt(i));

			SelectionChanged();
			break;
		}
		case MSG_ADD:
		{
			// Thank you, François Claus :D Merry Christmas!

			int32 ix = fResourceTypePopUp->FindMarkedIndex();

			if (ix != -1) {
				ResourceRow* row = new ResourceRow();
				row->SetResourceID(_NextResourceID());
				row->SetResourceType(kDefaultTypes[ix].type);
				row->SetResourceTypeCode(kDefaultTypes[ix].typeCode);
				row->SetResourceRawData(kDefaultData);
				row->SetResourceSize(kDefaultTypes[ix].size);
				fResourceList->AddRow(row);
			}

			break;
		}
		case MSG_REMOVE:
		{
			for (int i = 0; i < fResourceList->CountRows(); i++) {
				BRow* row = fResourceList->RowAt(i);

				if (row->IsSelected()) {
					fResourceList->RemoveRow(row);
					i--;
				}
			}

			break;
		}
		case MSG_MOVEUP:
		{
			for (int i = 1; i < fResourceList->CountRows(); i++) {
				BRow* row = fResourceList->RowAt(i);

				if (row->IsSelected())
					fResourceList->SwapRows(i, i - 1);

			}

			fResourceList->ClearSortColumns();
			SelectionChanged();
			break;
		}
		case MSG_MOVEDOWN:
		{
			for (int i = fResourceList->CountRows() - 1 - 1; i >= 0; i--) {
				BRow* row = fResourceList->RowAt(i);

				if (row->IsSelected())
					fResourceList->SwapRows(i, i + 1);
			}

			fResourceList->ClearSortColumns();
			SelectionChanged();
			break;
		}
		case MSG_SELECTION:
			SelectionChanged();
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}


void
MainWindow::_SetTitleFromEntry()
{
	char nameBuffer[B_FILE_NAME_LENGTH];

	fAssocEntry->GetName(nameBuffer);

	BString title;
	title << "ResourceEdit | ";
	title << nameBuffer;
	SetTitle(title);
}


void
MainWindow::_SaveAs()
{
	if (fSavePanel == NULL)
		fSavePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this), NULL,
			B_FILE_NODE, false, new BMessage(MSG_SAVEAS_DONE));

	fSavePanel->Rewind();
	fSavePanel->Show();
}


void
MainWindow::_Save(BEntry* entry)
{
	if (entry == NULL) {
		if (fAssocEntry == NULL) {
			_SaveAs();
			return;
		} else
			entry = fAssocEntry;
	} else {
		if (fAssocEntry == NULL) {
			fAssocEntry = entry;
			_SetTitleFromEntry();
		}
	}

	/*BPath path;
	entry->GetPath(&path);

	// I wouldn't use std:: if BFile had cooler stuff.
	// Not very comfortable here.

	fstream out(path.Path(), ios::out);

	time_t timeNow = time(0);


	out << "/-" << endl;
	out << " - This file is auto-generated by Haiku ResourceEdit." << endl;
	out << " - Time: " << ctime((const time_t*)&timeNow);
	out << " -\" << endl;

	for (int32 i = 0; i < fResourceList->CountRows(); i++) {
		ResourceRow* row = (ResourceRow*)fResourceList->RowAt(i);

		out << endl;
		out << "resource";

		if (true) {
			// TODO: Implement no-ID cases.

			out << "(" << row->ResourceID();

			if (row->ResourceName()[0] != '\0') {
				out << ", \"" << row->ResourceName() << "\"" << endl;
			}

			out << ") ";
		}

		if (row->ResourceTypeCode()[0] != '\0')
			out << "#\'" << row->ResourceTypeCode() << "\' ";

		if (strcmp(row->ResourceType(), "raw") != 0)
			out << row->ResourceType() << ' ' << row->ResourceData();
		else {
			// TODO: Implement hexdump and import.
			out << "array {" << endl;
			out << "\t\"Not yet implemented.\"" << endl;
			out << "}";
		}

		out << endl;
	}

	out.close();*/

	// Commented out whole output section. Switching to .rsrc files to
	// close Part 1 of GCI task.

	// TODO: Implement exporting to .rdef and/or other formats.


	BFile* file = new BFile(entry, B_READ_WRITE | B_CREATE_FILE);
	BResources output(file, true);
	delete file;

	for (int32 i = 0; i < fResourceList->CountRows(); i++) {
		ResourceRow* row = (ResourceRow*)fResourceList->RowAt(i);
		output.AddResource(row->ResourceTypeCode(), row->ResourceID(),
			row->ResourceRawData(), row->ResourceSize(), row->ResourceName());
	}

	output.Sync();
}


void
MainWindow::_Load()
{
	/*BPath path;
	struct stat st;

	fAssocEntry->GetPath(&path);
	fAssocEntry->GetStat(&st);

	int fd = open(path.Path(), 0);
	const char* in = (const char*)mmap(NULL, st.st_size, PROT_READ,
		MAP_SHARED, fd, 0);

	// TODO: Fix stucking bug.
	// TODO: Parse data.

	for (int32 i = 0; i < st.st_size - 1; i++) {
		//...
	}*/

	// Commented out input section. Same reason as above.

	// TODO: Implement importing from .rdef and/or other formats.

	BFile* file = new BFile(fAssocEntry, B_READ_ONLY);
	BResources input(file);
	delete file;

	type_code code;
	int32 id;
	const char* name;
	size_t size;

	for (int32 i = 0; input.GetResourceInfo(i, &code, &id, &name, &size); i++) {
		ResourceRow* row = new ResourceRow();
		row->SetResourceID(id);
		row->SetResourceName(name);
		row->SetResourceSize(size);
		row->SetResourceTypeCode(code);
		row->SetResourceRawData(input.LoadResource(code, id, &size));
		fResourceList->AddRow(row);
	}
}


int32
MainWindow::_NextResourceID()
{
	int32 currentID = atoi(fResourceIDText->Text());
	int32 nextID = currentID + 1;
	BString text;

	// Should I check if the ID is already present in the list?
	// Hmmm...

	text << nextID;
	fResourceIDText->SetText(text);

	return currentID;
}
