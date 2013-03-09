/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "MainWindow.h"

#include "Constants.h"
#include "EditWindow.h"
#include "ImageButton.h"
#include "ResourceListView.h"
#include "ResourceRow.h"
#include "SettingsFile.h"
#include "SettingsWindow.h"
#include "UndoContext.h"

#include <Alert.h>
#include <Application.h>
#include <Box.h>
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
#include <String.h>
#include <StringView.h>
#include <TextControl.h>
#include <TranslationUtils.h>

#include <stdio.h>
#include <stdlib.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fstream>
using namespace std;


MainWindow::MainWindow(BRect frame, BEntry* assocEntry, SettingsFile* settings)
	:
	BWindow(frame, NULL, B_DOCUMENT_WINDOW, B_OUTLINE_RESIZE)
{
	fAssocEntry = assocEntry;
	fSettings = settings;

	fUndoContext = new UndoContext();

	AdaptSettings();

	fSavePanel = NULL;
	fUnsavedChanges = false;

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
	fUndoItem->SetEnabled(false);
	fRedoItem = new BMenuItem("Redo", new BMessage(MSG_REDO), 'Y');
	fRedoItem->SetEnabled(false);
	fCutItem = new BMenuItem("Cut", new BMessage(MSG_CUT), 'X');
	fCutItem->SetEnabled(false);
	fCopyItem = new BMenuItem("Copy", new BMessage(MSG_COPY), 'C');
	fCopyItem->SetEnabled(false);
	fPasteItem = new BMenuItem("Paste", new BMessage(MSG_PASTE), 'V');
	fPasteItem->SetEnabled(false);
	fClearItem = new BMenuItem("Clear", new BMessage(MSG_CLEAR));
	fSelectAllItem = new BMenuItem("Select All",
		new BMessage(MSG_SELECTALL), 'A');

	fToolsMenu = new BMenu("Tools", B_ITEMS_IN_COLUMN);
	fAddAppResourcesItem = new BMenuItem("Add app resources",
		new BMessage(MSG_ADDAPPRES));
	fSettingsItem = new BMenuItem("Settings", new BMessage(MSG_SETTINGS));

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

	fResourceList->AddColumn(new BStringColumn("ID", 48, 1, 999,
		B_ALIGN_LEFT), 0);
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

	//fResourceList->SetLatchWidth(0);

	fResourceList->SetTarget(this);
	fResourceList->SetSelectionMessage(new BMessage(MSG_SELECTION));
	fResourceList->SetInvocationMessage(new BMessage(MSG_INVOCATION));

	fStatsString = new BStringView(BRect(3, 2, 150, 13), "fStatsString", "");
	fStatsString->SetFontSize(11.0f);

	fStatsBox = new BBox(BRect(0, 0, 150, 16), "fStatsBox");
	fStatsBox->SetBorder(B_PLAIN_BORDER);

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

	fMenuBar->AddItem(fToolsMenu);
		fToolsMenu->AddItem(fAddAppResourcesItem);
		fToolsMenu->AddSeparatorItem();
		fToolsMenu->AddItem(fSettingsItem);

	fMenuBar->AddItem(fHelpMenu);

	fToolbarView->AddChild(fResourceIDText);
	fToolbarView->AddChild(fResourceTypeMenu);

	fToolbarView->AddChild(fAddButton);
	fToolbarView->AddChild(fRemoveButton);
	fToolbarView->AddChild(fMoveUpButton);
	fToolbarView->AddChild(fMoveDownButton);

	AddChild(fToolbarView);

	AddChild(fResourceList);
	fResourceList->AddStatusView(fStatsBox);
	fStatsBox->AddChild(fStatsString);

	if (assocEntry != NULL) {
		_SetTitleFromEntry();
		_Load();
	} else {
		SetTitle("ResourceEdit | " B_UTF8_ELLIPSIS);
	}
}


MainWindow::~MainWindow()
{
	delete fUndoContext;
}


bool
MainWindow::QuitRequested()
{
	// CAUTION:	Do not read the body of this function if you
	//			are known to suffer from gotophobia.

	if (fUnsavedChanges) {
		Activate();

		char nameBuffer[B_FILE_NAME_LENGTH];

		if (fAssocEntry != NULL)
			fAssocEntry->GetName(nameBuffer);
		else
			strcpy(nameBuffer, "Untitled");

		BString warning = "";
		warning << "Save changse to'";
		warning << nameBuffer;
		warning << "' before closing?";

		BAlert* alert = new BAlert("Save", warning.String(),
			"Cancel", "Don't save", "Save",
			B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);

		alert->SetShortcut(0, B_ESCAPE);

		int32 result = alert->Go();

		switch (result) {
			case 0:
				return false;
			case 1:
				goto quit;
			case 2:
			{
				_Save();

				if (fUnsavedChanges)
					return false;
				else
					goto quit;
			}
		}
	}

	quit:
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

		fCutItem->SetEnabled(false);
		fCopyItem->SetEnabled(false);
	} else {
		fRemoveButton->SetEnabled(true);
		fMoveUpButton->SetEnabled(!fResourceList->RowAt(0)->IsSelected());
		fMoveDownButton->SetEnabled(!fResourceList->RowAt(
			fResourceList->CountRows() - 1)->IsSelected());

		fCutItem->SetEnabled(true);
		fCopyItem->SetEnabled(true);
	}
}


void
MainWindow::MouseDown(BPoint point)
{

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
			fUndoContext->Undo();
			_RefreshUndoRedo();
			break;

		case MSG_REDO:
			fUndoContext->Redo();
			_RefreshUndoRedo();
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
		{
			BList* rows = new BList();

			for (int32 i = 0; i < fResourceList->CountRows(); i++) {
				ResourceRow* row = (ResourceRow*)fResourceList->RowAt(i);
				row->ActionIndex = i;
				rows->AddItem(row);
			}

			_Do(new RemoveAction("Clear", fResourceList, rows));

			SelectionChanged();
			break;
		}
		case MSG_SELECTALL:
		{
			for (int32 i = 0; i < fResourceList->CountRows(); i++)
				fResourceList->AddToSelection(fResourceList->RowAt(i));

			SelectionChanged();
			break;
		}
		case MSG_SETTINGS:
			be_app->PostMessage(MSG_SETTINGS);
			break;

		case MSG_ADD:
		{
			// Thank you, François Claus :D Merry Christmas!

			int32 ix = fResourceTypePopUp->FindMarkedIndex();

			if (ix != -1) {
				BList* rows = new BList();

				ResourceRow* row = new ResourceRow();
				row->SetResourceID(_NextResourceID());
				row->SetResourceType(kDefaultTypes[ix].type);
				row->SetResourceStringCode(kDefaultTypes[ix].code);
				row->SetResourceData(kDefaultTypes[ix].data);
				row->SetResourceSize(kDefaultTypes[ix].size);
				row->ActionIndex = fResourceList->CountRows(NULL);

				rows->AddItem(row);

				_Do(new AddAction("Add", fResourceList, rows));
			}

			break;
		}
		case MSG_REMOVE:
		{
			BList* rows = new BList();

			for (int i = 0; i < fResourceList->CountRows(); i++) {
				ResourceRow* row = (ResourceRow*)fResourceList->RowAt(i);

				if (row->IsSelected()) {
					row->ActionIndex = i;
					rows->AddItem(row);
				}
			}

			if (!rows->IsEmpty())
				_Do(new RemoveAction("Remove", fResourceList, rows));
			else
				delete rows;

			break;
		}
		case MSG_MOVEUP:
		{
			BList* rows = new BList();

			for (int i = 1; i < fResourceList->CountRows(); i++) {
				ResourceRow* row = (ResourceRow*)fResourceList->RowAt(i);

				if (row->IsSelected()) {
					row->ActionIndex = i;
					rows->AddItem(row);
				}
			}

			_Do(new MoveUpAction("Move Up", fResourceList, rows));

			break;
		}
		case MSG_MOVEDOWN:
		{
			BList* rows = new BList();

			for (int i = 0; i < fResourceList->CountRows() - 1; i++) {
				ResourceRow* row = (ResourceRow*)fResourceList->RowAt(i);

				if (row->IsSelected()) {
					row->ActionIndex = i;
					rows->AddItem(row);
				}
			}

			_Do(new MoveDownAction("Move Down", fResourceList, rows));

			break;
		}
		case MSG_SELECTION:
			SelectionChanged();
			break;

		case MSG_INVOCATION:
		{
			BRect frame = Frame();

			new EditWindow(BRect(frame.left + 50, frame.top + 50,
				frame.left + 300, frame.top + 350),
				(ResourceRow*)fResourceList->CurrentSelection());

			break;
		}
		case MSG_SETTINGS_APPLY:
			AdaptSettings();
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}


void
MainWindow::AdaptSettings()
{
	fUndoContext->SetLimit(fSettings->UndoLimit);
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

	BPath path;
	entry->GetPath(&path);

	// I wouldn't use std:: if BFile had cooler stuff.
	// Not very comfortable here.

	fstream out(path.Path(), ios::out);

	time_t timeNow = time(0);


	out << "/*" << endl;
	out << " * This file is auto-generated by Haiku ResourceEdit." << endl;
	out << " * Time: " << ctime((const time_t*)&timeNow);
	out << " */" << endl;

	for (int32 i = 0; i < fResourceList->CountRows(); i++) {
		ResourceRow* row = (ResourceRow*)fResourceList->RowAt(i);

		out << endl;
		out << "resource";

		if (row->ResourceStringID()[0] != '\0') {
			out << "(" << row->ResourceID();

			if (row->ResourceName()[0] != '\0')
				out << ", \"" << row->ResourceName() << "\"";

			out << ")";
		} else {
			if (row->ResourceName()[0] != '\0')
				out << "(\"" << row->ResourceName() << "\")";
		}

		out << " ";

		if (row->ResourceStringCode()[0] != '\0')
			out << "#\'" << row->ResourceStringCode() << "\' ";

		if (strcmp(row->ResourceType(), "raw") != 0)
			out << row->ResourceType() << ' ' << row->ResourceData();
		else {
			// TODO: Implement hexdump and import.
			out << "array {" << endl;
			out << "\t\"Not yet implemented.\"" << endl;
			out << "}";
		}

		out << ";" << endl;
	}

	out.close();

	// Killed code: Export to .rsrc

	/*BFile* file = new BFile(entry, B_READ_WRITE | B_CREATE_FILE);
	BResources output(file, true);
	delete file;

	for (int32 i = 0; i < fResourceList->CountRows(); i++) {
		ResourceRow* row = (ResourceRow*)fResourceList->RowAt(i);

		output.AddResource(row->ResourceCode(), row->ResourceID(),
			row->ResourceRawData(), row->ResourceSize(), row->ResourceName());
	}

	output.Sync();*/

	fUnsavedChanges = false;
}


void
MainWindow::_Load()
{
	// CAUTION: Here be dragons.

	// [Initialization phase]

	BPath path;
	struct stat st;

	fAssocEntry->GetPath(&path);
	fAssocEntry->GetStat(&st);

	int fd = open(path.Path(), 0);

	const char* in = (const char*)mmap(NULL, st.st_size, PROT_READ,
		MAP_SHARED, fd, 0);

	// [Setup phase]

	enum {
		T_STATE_NORMAL,
		T_STATE_COMMENT,
		T_STATE_MULTILINE_COMMENT,
		T_STATE_STRING,
	} t_state = T_STATE_NORMAL;

	char quoteType = '\0';

	char escChars[256];

	for (int32 i = 0; i < 256; i++)
		escChars[i] = ~(char)0;

	escChars['n'] = '\n';
	escChars['t'] = '\t';
	escChars['r'] = '\r';
	escChars['0'] = '\0';
	escChars['"'] = '\"';
	escChars['\'']= '\'';

	// TODO: Add other escape sequences?
	// Multichar escape sequences cannot be resolved with this.

	BString buffer = "";
	BList* tokens = new BList();

	BList* errors = new BList();
		// Multiple errors are allowed, but currently unused.

	BList* rows = new BList();

	// [Tokenization phase]

	for (int32 i = 0; i < st.st_size - 1; i++) {
		if (t_state == T_STATE_NORMAL) {
			if (in[i] == ' ' || in[i] == '\t' || in[i] == '\n'
				 || in[i] == '\r') {

				if (buffer != "") {
					tokens->AddItem(new BString(buffer));
					buffer = "";
				}

				continue;
			}

			if (in[i] == ',' || in[i] == ';' || in[i] == '{' || in[i] == '}'
				|| in[i] == '(' || in[i] == ')') {
				if (buffer != "") {
					tokens->AddItem(new BString(buffer));
					buffer = in[i];
					tokens->AddItem(new BString(buffer));
					buffer = "";
				}

				continue;
			}

			if (in[i] == '"' || in[i] == '\'') {
				t_state = T_STATE_STRING;
				quoteType = in[i];
				buffer += in[i];
				continue;
			}

			if (in[i] == '/') {
				if (in[i + 1] == '/') {
					t_state = T_STATE_COMMENT;
					continue;
				}

				if (in[i + 1] == '*') {
					t_state = T_STATE_MULTILINE_COMMENT;
					continue;
				}
			}

			buffer += in[i];

		} else if (t_state == T_STATE_COMMENT) {
			if (in[i] == '\n') {
				t_state = T_STATE_NORMAL;
				continue;
			}
		} else if (t_state == T_STATE_MULTILINE_COMMENT) {
			if (in[i] == '*' && in[i + 1] == '/') {
				t_state = T_STATE_NORMAL;
				i++;
				continue;
			}
		} else if (t_state == T_STATE_STRING) {
			if (in[i] == '\\' && escChars[(int)in[i]] != ~(char)0) {
				buffer += escChars[(int)in[i]];
				i++;
				continue;
			}

			if (in[i] == quoteType) {
				buffer += in[i];
				tokens->AddItem(new BString(buffer));
				buffer = "";
				t_state = T_STATE_NORMAL;
				continue;
			}

			buffer += in[i];
		} else {
			// This can't even happen.
		}
	}

	if (t_state == T_STATE_MULTILINE_COMMENT)
		errors->AddItem(new BString(
			"Multi-line comment not closed by end of file."));
	else if (t_state == T_STATE_STRING)
		errors->AddItem(new BString("String not closed by end of file."));
	else {
		// [Parsing phase]

		for (int32 i = 0; i < tokens->CountItems(); i++) {
			BString& token = *(BString*)tokens->ItemAt(i);

			if (token == "resource") {
				ResourceRow* row = new ResourceRow();

				token = *(BString*)tokens->ItemAt(++i);

				if (token[0] == '(') {
					BList args;
					bool wantComma = false;

					while (true) {
						token = *(BString*)tokens->ItemAt(++i);

						if (token[0] == ')')
							break;

						if (wantComma) {
							if (token[0] != ',') {
								errors->AddItem(new BString("Expected ','."));
								break;
							}

							wantComma = false;
						} else {
							args.AddItem(tokens->ItemAt(i));
							wantComma = true;
						}
					}

					if (errors->CountItems() > 0)
						break;

					token = *(BString*)tokens->ItemAt(++i);

					if (args.CountItems() > 2) {
						errors->AddItem(new BString(
							"Too many arguments for resource identifier."));
						break;
					}

					if (args.CountItems() >= 1) {
						BString& arg = *(BString*)args.ItemAt(0);

						if (arg[0] == '"') {
							errors->AddItem(new BString(
								"Resource ID cannot be a string."));
							break;
						}

						row->SetResourceStringID(arg);
					}

					if (args.CountItems() == 2) {
						BString& arg = *(BString*)args.ItemAt(1);

						if (arg[0] != '"') {
							errors->AddItem(new BString(
								"Resource name must be a string."));
							break;
						}

						row->SetResourceName(arg);
					}
				}

				if (token[0] == '#') {
					if (token.Length() != 7
						|| token[1] != '\'' || token[6] != '\'') {
						errors->AddItem(new BString(
							"Invalid syntax for resource type-code."));
						break;
					}

					BString code;
					token.CopyInto(code, 2, 4);
					row->SetResourceStringCode(code);

					token = *(BString*)tokens->ItemAt(++i);
				}

				if (token[0] == '(') {
					BString& type = *(BString*)tokens->ItemAt(++i);
					token = *(BString*)tokens->ItemAt(++i);

					if (token[0] != ')') {
						errors->AddItem(new BString(
							"Expected ')'."));
						break;
					}

					row->SetResourceType(type);
				}

			} else if (token == "type") {
				// TODO: Implement.
			} else if (token == "enum") {
				// TODO: Implement.
			}
		}
	}

	// [Result phase]

	if (errors->CountItems() > 0) {
		for (int32 i = 0; i < errors->CountItems(); i++) {
			BString& error = *(BString*)errors->ItemAt(i);
			PRINT(("ERROR: %s\n", error.String()));
			delete (BString*)tokens->ItemAt(i);
		}

		delete errors;
	} else {
		for (int32 i = 0; i < rows->CountItems(); i++) {
			ResourceRow* row = (ResourceRow*)rows->ItemAt(i);
			PRINT(("[Resource]\n"));
			PRINT(("id   = %s\n", row->ResourceStringID()));
			PRINT(("name = %s\n", row->ResourceName()));
			// ...
			PRINT(("\n"));
		}
	}

	// [Cleanup phase]

	for (int32 i = 0; i < tokens->CountItems(); i++)
		delete (BString*)tokens->ItemAt(i);

	delete tokens;

	munmap((void*)in, st.st_size);

	// Killed code: Import from .rsrc

	/*BFile* file = new BFile(fAssocEntry, B_READ_ONLY);
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
		row->SetResourceCode(code);
		row->SetResourceRawData(input.LoadResource(code, id, &size));
		fResourceList->AddRow(row);
	}*/
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


void
MainWindow::_RefreshStats()
{
	const size_t kB = 1024;
	const size_t MB = 1024 * 1024;
	const size_t GB = 1024 * 1024 * 1024;

	size_t size = 0;

	BString text = "";

	text << fResourceList->CountRows();
	text << " resources, ";

	for (int32 i = 0; i < fResourceList->CountRows(); i++) {
		ResourceRow* row = (ResourceRow*)fResourceList->RowAt(i);
		size += row->ResourceSize();
	}

	if (size >= GB) {
		text << (double)size / GB;
		text << " GB";
	} else if (size >= MB) {
		text << (double)size / MB;
		text << " MB";
	} else if (size >= kB) {
		text << (double)size / kB;
		text << " kB";
	} else {
		text << size;
		text << " bytes";
	}

	fStatsString->SetText(text);
}


void
MainWindow::_RefreshUndoRedo()
{
	if (fUndoContext->CanUndo()) {
		fUndoItem->SetEnabled(true);
		fUndoItem->SetLabel(fUndoContext->UndoLabel().Prepend("Undo "));
	} else {
		fUndoItem->SetEnabled(false);
		fUndoItem->SetLabel("Undo");
	}

	if (fUndoContext->CanRedo()) {
		fRedoItem->SetEnabled(true);
		fRedoItem->SetLabel(fUndoContext->RedoLabel().Prepend("Redo "));
	} else {
		fRedoItem->SetEnabled(false);
		fRedoItem->SetLabel("Redo");
	}
}


void
MainWindow::_Do(UndoContext::Action* action)
{
	fUnsavedChanges = true;
	fUndoContext->Do(action);
	_RefreshStats();
	_RefreshUndoRedo();
}


MainWindow::AddAction::AddAction(const BString& label,
	ResourceListView* list, BList* rows)
	:
	UndoContext::Action(label)
{
	fList = list;
	fRows = rows;
	fAdded = false;
}


MainWindow::AddAction::~AddAction()
{
	if (!fAdded)
		for (int32 i = 0; i < fRows->CountItems(); i++)
			delete (ResourceRow*)fRows->ItemAt(i);

	delete fRows;
}


void
MainWindow::AddAction::Do()
{
	for (int32 i = 0; i < fRows->CountItems(); i++) {
		ResourceRow* row = (ResourceRow*)fRows->ItemAt(i);
		fList->AddRow(row, row->ActionIndex, row->Parent);
	}

	fAdded = true;
}


void
MainWindow::AddAction::Undo()
{
	for (int32 i = 0; i < fRows->CountItems(); i++)
			fList->RemoveRow((ResourceRow*)fRows->ItemAt(i));

	fAdded = false;
}


MainWindow::RemoveAction::RemoveAction(const BString& label,
	ResourceListView* list, BList* rows)
	:
	UndoContext::Action(label)
{
	fList = list;
	fRows = rows;
	fRemoved = false;
}


MainWindow::RemoveAction::~RemoveAction()
{
	if (fRemoved)
		for (int32 i = 0; i < fRows->CountItems(); i++)
			delete (ResourceRow*)fRows->ItemAt(i);

	delete fRows;
}


void
MainWindow::RemoveAction::Do()
{
	for (int32 i = 0; i < fRows->CountItems(); i++)
		fList->RemoveRow((ResourceRow*)fRows->ItemAt(i));

	fRemoved = true;
}


void
MainWindow::RemoveAction::Undo()
{
	for (int32 i = 0; i < fRows->CountItems(); i++) {
		ResourceRow* row = (ResourceRow*)fRows->ItemAt(i);

		fList->AddRow(row, row->ActionIndex, row->Parent);
	}

	fRemoved = false;
}

// TODO: Implement EditAction

MainWindow::MoveUpAction::MoveUpAction(const BString& label,
	ResourceListView* list, BList* rows)
	:
	UndoContext::Action(label)
{
	fList = list;
	fRows = rows;
}


MainWindow::MoveUpAction::~MoveUpAction()
{
	delete fRows;
}


void
MainWindow::MoveUpAction::Do()
{
	for (int32 i = 0; i < fRows->CountItems(); i++) {
		ResourceRow* row = (ResourceRow*)fRows->ItemAt(i);

		fList->SwapRows(row->ActionIndex, row->ActionIndex - 1,
			row->Parent, row->Parent);

		row->ActionIndex--;
	}

	fList->SelectionChanged();
}


void
MainWindow::MoveUpAction::Undo()
{
	for (int32 i = fRows->CountItems() - 1; i >= 0; i--) {
		ResourceRow* row = (ResourceRow*)fRows->ItemAt(i);

		fList->SwapRows(row->ActionIndex, row->ActionIndex + 1,
			row->Parent, row->Parent);

		row->ActionIndex++;
	}

	fList->SelectionChanged();
}

MainWindow::MoveDownAction::MoveDownAction(const BString& label,
	ResourceListView* list, BList* rows)
	:
	UndoContext::Action(label)
{
	fList = list;
	fRows = rows;
}


MainWindow::MoveDownAction::~MoveDownAction()
{
	delete fRows;
}


void
MainWindow::MoveDownAction::Do()
{
	for (int32 i = fRows->CountItems() - 1; i >= 0; i--) {
		ResourceRow* row = (ResourceRow*)fRows->ItemAt(i);

		fList->SwapRows(row->ActionIndex, row->ActionIndex + 1,
			row->Parent, row->Parent);

		row->ActionIndex++;
	}

	fList->SelectionChanged();
}


void
MainWindow::MoveDownAction::Undo()
{
	for (int32 i = 0; i < fRows->CountItems(); i++) {
		ResourceRow* row = (ResourceRow*)fRows->ItemAt(i);

		fList->SwapRows(row->ActionIndex, row->ActionIndex - 1,
			row->Parent, row->Parent);

		row->ActionIndex--;
	}

	fList->SelectionChanged();
}

