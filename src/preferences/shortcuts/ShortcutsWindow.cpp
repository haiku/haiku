/*
 * Copyright 1999-2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jeremy Friesner
 *		Fredrik Modéen
 */


#include "ShortcutsWindow.h"

#include <math.h>
#include <stdio.h>

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <Clipboard.h>
#include <File.h>
#include <FindDirectory.h>
#include <Input.h>
#include <Locale.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <MessageFilter.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <String.h>

#include "ColumnListView.h"

#include "KeyInfos.h"
#include "MetaKeyStateMap.h"
#include "ParseCommandLine.h"
#include "ShortcutsFilterConstants.h"
#include "ShortcutsSpec.h"


// Window sizing constraints
#define MIN_WIDTH	600
#define MIN_HEIGHT	130
#define MAX_WIDTH	65535
#define MAX_HEIGHT	65535

// Default window position
#define WINDOW_START_X 30
#define WINDOW_START_Y 100

#undef B_TRANSLATE_CONTEXT
#define B_TRANSLATE_CONTEXT "ShortcutsWindow"

#define ERROR "Shortcuts error"
#define WARNING "Shortcuts warning"

// Global constants for Shortcuts
#define V_SPACING 5 // vertical spacing between GUI components


// Creates a pop-up-menu that reflects the possible states of the specified 
// meta-key.
static BPopUpMenu*
CreateMetaPopUp(int col)
{
	MetaKeyStateMap& map = GetNthKeyMap(col);
	BPopUpMenu * popup = new BPopUpMenu(NULL, false);
	int numStates = map.GetNumStates();
	
	for (int i = 0; i < numStates; i++) 
		popup->AddItem(new BMenuItem(map.GetNthStateDesc(i), NULL));
	
	return popup;
}


// Creates a pop-up that allows the user to choose a key-cap visually
static BPopUpMenu*
CreateKeysPopUp()
{
	BPopUpMenu* popup = new BPopUpMenu(NULL, false);
	int numKeys = GetNumKeyIndices();
	for (int i = 0; i < numKeys; i++) {
		const char* next = GetKeyName(i);
		
		if (next) 
			popup->AddItem(new BMenuItem(next, NULL));
	}
	return popup;
}


ShortcutsWindow::ShortcutsWindow()
	:
	BWindow(BRect(WINDOW_START_X, WINDOW_START_Y, WINDOW_START_X + MIN_WIDTH, 
		WINDOW_START_Y + MIN_HEIGHT * 2), B_TRANSLATE_SYSTEM_NAME("Shortcuts"),
		B_DOCUMENT_WINDOW, 0L), 
	fSavePanel(NULL), 
	fOpenPanel(NULL), 
	fSelectPanel(NULL), 
	fKeySetModified(false), 		
	fLastOpenWasAppend(false)
{
	ShortcutsSpec::InitializeMetaMaps();

	SetSizeLimits(MIN_WIDTH, MAX_WIDTH, MIN_HEIGHT, MAX_HEIGHT);
	BMenuBar* menuBar = new BMenuBar(BRect(0, 0, 0, 0), "Menu Bar");

	BMenu* fileMenu = new BMenu(B_TRANSLATE("File"));
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Open KeySet" B_UTF8_ELLIPSIS), 
		new BMessage(OPEN_KEYSET), 'O'));
	fileMenu->AddItem(new BMenuItem(
		B_TRANSLATE("Append KeySet" B_UTF8_ELLIPSIS), 
		new BMessage(APPEND_KEYSET), 'A'));
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Revert to saved"), 
		new BMessage(REVERT_KEYSET), 'A'));
	fileMenu->AddItem(new BSeparatorItem);
	fileMenu->AddItem(new BMenuItem(
		B_TRANSLATE("Save KeySet as" B_UTF8_ELLIPSIS), 
		new BMessage(SAVE_KEYSET_AS), 'S'));
	fileMenu->AddItem(new BSeparatorItem);
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(B_QUIT_REQUESTED), 'Q'));
	menuBar->AddItem(fileMenu);

	AddChild(menuBar);

	font_height fh;
	be_plain_font->GetHeight(&fh);
	float vButtonHeight = ceil(fh.ascent) + ceil(fh.descent) + 5.0f;

	BRect tableBounds = Bounds();
	tableBounds.top = menuBar->Bounds().bottom + 1;
	tableBounds.right -= B_V_SCROLL_BAR_WIDTH;
	tableBounds.bottom -= (B_H_SCROLL_BAR_HEIGHT + V_SPACING + vButtonHeight + 
		V_SPACING * 2);
	
	BScrollView* containerView;
	fColumnListView = new ColumnListView(tableBounds, &containerView, NULL, 
		B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE, 
		B_SINGLE_SELECTION_LIST, true, true, true, B_NO_BORDER);
	
	fColumnListView->SetEditMessage(new BMessage(HOTKEY_ITEM_MODIFIED), 
		BMessenger(this));
	
	const float metaWidth = 50.0f;

	for (int i = 0; i < ShortcutsSpec::NUM_META_COLUMNS; i++) {
		fColumnListView->AddColumn(
			new CLVColumn(ShortcutsSpec::GetColumnName(i), CreateMetaPopUp(i), 
			metaWidth, CLV_SORT_KEYABLE));
	}

	fColumnListView->AddColumn(new CLVColumn(B_TRANSLATE("Key"),
		CreateKeysPopUp(), 60, CLV_SORT_KEYABLE));

	BPopUpMenu* popup = new BPopUpMenu(NULL, false);
	popup->AddItem(new BMenuItem(
		B_TRANSLATE("(Choose application with file requester)"), NULL));
	popup->AddItem(new BMenuItem(
		B_TRANSLATE("*InsertString \"Your Text Here\""), NULL));
	popup->AddItem(new BMenuItem(
		B_TRANSLATE("*MoveMouse +20 +0"), NULL));
	popup->AddItem(new BMenuItem(B_TRANSLATE("*MoveMouseTo 50% 50%"), NULL));
	popup->AddItem(new BMenuItem(B_TRANSLATE("*MouseButton 1"), NULL));
	popup->AddItem(new BMenuItem(
		B_TRANSLATE("*LaunchHandler text/html"), NULL));
	popup->AddItem(new BMenuItem(
		B_TRANSLATE("*Multi \"*MoveMouseTo 100% 0\" \"*MouseButton 1\""),
		NULL));
	popup->AddItem(new BMenuItem(B_TRANSLATE("*MouseDown"), NULL));
	popup->AddItem(new BMenuItem(B_TRANSLATE("*MouseUp"), NULL));
	popup->AddItem(new BMenuItem(
		B_TRANSLATE("*SendMessage application/x-vnd.Be-TRAK 'Tfnd'"), NULL));
	popup->AddItem(new BMenuItem(B_TRANSLATE("*Beep"), NULL));
	fColumnListView->AddColumn(new CLVColumn(B_TRANSLATE("Application"), popup,
		323.0, CLV_SORT_KEYABLE));

	fColumnListView->SetSortFunction(ShortcutsSpec::MyCompare);
	AddChild(containerView);

	fColumnListView->SetSelectionMessage(new BMessage(HOTKEY_ITEM_SELECTED));
	fColumnListView->SetTarget(this);

	BRect buttonBounds = Bounds();
	buttonBounds.left += V_SPACING;
	buttonBounds.right = ((buttonBounds.right - buttonBounds.left) / 2.0f)
		+ buttonBounds.left;
	buttonBounds.bottom -= V_SPACING * 2;
	buttonBounds.top = buttonBounds.bottom - vButtonHeight;
	buttonBounds.right -= B_V_SCROLL_BAR_WIDTH;
	float origRight = buttonBounds.right;
	buttonBounds.right = (buttonBounds.left + origRight) * 0.40f - 
		(V_SPACING / 2);
	AddChild(fAddButton = new ResizableButton(Bounds(), buttonBounds, "add", 
		B_TRANSLATE("Add new shortcut"), new BMessage(ADD_HOTKEY_ITEM)));
	buttonBounds.left = buttonBounds.right + V_SPACING;
	buttonBounds.right = origRight;
	AddChild(fRemoveButton = new ResizableButton(Bounds(), buttonBounds, 
		"remove", B_TRANSLATE("Remove selected shortcut"), 
		new BMessage(REMOVE_HOTKEY_ITEM)));
	
	fRemoveButton->SetEnabled(false);

	float offset = (buttonBounds.right - buttonBounds.left) / 2.0f;
	BRect saveButtonBounds = buttonBounds;
	saveButtonBounds.right = Bounds().right - B_V_SCROLL_BAR_WIDTH - offset;
	saveButtonBounds.left = buttonBounds.right + V_SPACING + offset;
	AddChild(fSaveButton = new ResizableButton(Bounds(), saveButtonBounds, 
		"save", B_TRANSLATE("Save & apply"), new BMessage(SAVE_KEYSET)));
	
	fSaveButton->SetEnabled(false);

	entry_ref ref;	
	if (_GetSettingsFile(&ref)) {
		BMessage msg(B_REFS_RECEIVED);
		msg.AddRef("refs", &ref);
		msg.AddString("startupRef", "please");
		PostMessage(&msg); // Tell ourself to load this file if it exists.
	}
	Show();
}


ShortcutsWindow::~ShortcutsWindow()
{
	delete fSavePanel;
	delete fOpenPanel;
	delete fSelectPanel;
	be_app->PostMessage(B_QUIT_REQUESTED);
}


bool
ShortcutsWindow::QuitRequested()
{
	bool ret = true;

	if (fKeySetModified) {
		BAlert* alert = new BAlert(WARNING, 
			B_TRANSLATE("Really quit without saving your changes?"),
			B_TRANSLATE("Don't save"), B_TRANSLATE("Cancel"),
			B_TRANSLATE("Save"));
		switch(alert->Go()) {
			case 1:
				ret = false;
				break;

			case 2:
				// Save: automatically if possible, otherwise go back and open
				// up the file requester
				if (fLastSaved.InitCheck() == B_OK) {
					if (_SaveKeySet(fLastSaved) == false) {
						(new BAlert(ERROR, 
							B_TRANSLATE("Shortcuts was unable to save your "
								"KeySet file!"), 
							B_TRANSLATE("Oh no")))->Go();
						ret = true; //quit anyway
					}
				} else {
					PostMessage(SAVE_KEYSET);
					ret = false;
				}
				break;
			default:
				ret = true;
				break;
		}
	}

	if (ret)
		fColumnListView->DeselectAll();
	return ret;
}


bool
ShortcutsWindow::_GetSettingsFile(entry_ref* eref)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return false;
	else 
		path.Append(SHORTCUTS_SETTING_FILE_NAME);

	if (BEntry(path.Path(), true).GetRef(eref) == B_OK)
		return true;
	else
		return false;
}


// Saves a settings file to (saveEntry). Returns true iff successful.
bool
ShortcutsWindow::_SaveKeySet(BEntry& saveEntry)
{
	BFile saveTo(&saveEntry, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (saveTo.InitCheck() != B_OK)
		return false;

	BMessage saveMsg;
	for (int i = 0; i < fColumnListView->CountItems(); i++) {
		BMessage next;
		if (((ShortcutsSpec*)fColumnListView->ItemAt(i))->Archive(&next) 
			== B_OK)
			saveMsg.AddMessage("spec", &next);
		else
			printf("Error archiving ShortcutsSpec #%i!\n", i);
	}
	
	bool ret = (saveMsg.Flatten(&saveTo) == B_OK);
	
	if (ret) {
		fKeySetModified = false;
		fSaveButton->SetEnabled(false);
	}

	return ret;
}


// Appends new entries from the file specified in the "spec" entry of 
// (loadMsg). Returns true iff successful.
bool
ShortcutsWindow::_LoadKeySet(const BMessage& loadMsg)
{
	int i = 0;
	BMessage msg;
	while (loadMsg.FindMessage("spec", i++, &msg) == B_OK) {
		ShortcutsSpec* spec = (ShortcutsSpec*)ShortcutsSpec::Instantiate(&msg);
		if (spec != NULL) 
			fColumnListView->AddItem(spec);
		else 
			printf("_LoadKeySet: Error parsing spec!\n");
	}
	return true;
}


// Creates a new entry and adds it to the GUI. (defaultCommand) will be the 
// text in the entry, or NULL if no text is desired.
void
ShortcutsWindow::_AddNewSpec(const char* defaultCommand)
{
	_MarkKeySetModified();

	ShortcutsSpec* spec;
	int curSel = fColumnListView->CurrentSelection();
	if (curSel >= 0) {
		spec = new ShortcutsSpec(*((ShortcutsSpec*)
			fColumnListView->ItemAt(curSel)));

		if (defaultCommand)
			spec->SetCommand(defaultCommand);
	} else 
		spec = new ShortcutsSpec(defaultCommand ? defaultCommand : "");

	fColumnListView->AddItem(spec);
	fColumnListView->Select(fColumnListView->CountItems() - 1);
	fColumnListView->ScrollToSelection();
}


void
ShortcutsWindow::MessageReceived(BMessage* msg)
{
	switch(msg->what) {
		case OPEN_KEYSET: 
		case APPEND_KEYSET: 
			fLastOpenWasAppend = (msg->what == APPEND_KEYSET);
			if (fOpenPanel) 
				fOpenPanel->Show();
			else {
				BMessenger m(this);
				fOpenPanel = new BFilePanel(B_OPEN_PANEL, &m, NULL, 0, false);
				fOpenPanel->Show();
			}
			fOpenPanel->SetButtonLabel(B_DEFAULT_BUTTON, fLastOpenWasAppend ? 
				B_TRANSLATE("Append") : B_TRANSLATE("Open"));
			break;

		case REVERT_KEYSET:
		{
			// Send a message to myself, to get me to reload the settings file
			fLastOpenWasAppend = false;
			BMessage reload(B_REFS_RECEIVED);
			entry_ref eref;
			_GetSettingsFile(&eref);
			reload.AddRef("refs", &eref);
			reload.AddString("startupRef", "yeah");
			PostMessage(&reload);
			break;
		}

		// Respond to drag-and-drop messages here
		case B_SIMPLE_DATA:
		{
			int i = 0;

			entry_ref ref;
			while (msg->FindRef("refs", i++, &ref) == B_OK) {
				BEntry entry(&ref);
				if (entry.InitCheck() == B_OK) {
					BPath path(&entry);
					
					if (path.InitCheck() == B_OK) {
						// Add a new item with the given path.
						BString str(path.Path());
						DoStandardEscapes(str);
						_AddNewSpec(str.String());
					}
				}
			}
			break;
		}

		// Respond to FileRequester's messages here
		case B_REFS_RECEIVED:
		{
			// Find file ref
			entry_ref ref;
			bool isStartMsg = msg->HasString("startupRef");
			if (msg->FindRef("refs", &ref) == B_OK) {
				// load the file into (fileMsg)
				BMessage fileMsg;
				{
					BFile file(&ref, B_READ_ONLY);
					if ((file.InitCheck() != B_OK) 
						|| (fileMsg.Unflatten(&file) != B_OK)) {
						if (isStartMsg) {
							// use this to save to anyway
							fLastSaved = BEntry(&ref);
							break;
						} else {
							(new BAlert(ERROR, 
								B_TRANSLATE("Shortcuts was couldn't open your "
								"KeySet file!"), B_TRANSLATE("OK")))->Go(NULL);
							break;
						}
					}
				}
 
				if (fLastOpenWasAppend == false) {
					// Clear the menu...
					while (ShortcutsSpec* item
						= ((ShortcutsSpec*)fColumnListView->RemoveItem(0L))) {
						delete item;
					}
				}

				if (_LoadKeySet(fileMsg)) {
					if (isStartMsg) fLastSaved = BEntry(&ref);
					fSaveButton->SetEnabled(isStartMsg == false);

					// If we just loaded in the Shortcuts settings file, then 
					// no need to tell the user to save on exit.
					entry_ref eref;
					_GetSettingsFile(&eref);
					if (ref == eref) fKeySetModified = false;
				} else {
					(new BAlert(ERROR, 
						B_TRANSLATE("Shortcuts was unable to parse your "
						"KeySet file!"), 
						B_TRANSLATE("OK")))->Go(NULL);
					break;
				}
			}
			break;
		}

		// These messages come from the pop-up menu of the Applications column
		case SELECT_APPLICATION:
		{
			int csel = fColumnListView->CurrentSelection();
			if (csel >= 0) {
				entry_ref aref;
				if (msg->FindRef("refs", &aref) == B_OK) {
					BEntry ent(&aref);
					if (ent.InitCheck() == B_OK) {
						BPath path;
						if ((ent.GetPath(&path) == B_OK) 
							&& (((ShortcutsSpec *)
							fColumnListView->ItemAt(csel))->
							ProcessColumnTextString(ShortcutsSpec::
							STRING_COLUMN_INDEX, path.Path()))) {
							
							fColumnListView->InvalidateItem(csel);
							_MarkKeySetModified();
						}
					}
				}
			}
			break;
		}

		case SAVE_KEYSET:
		{
			bool showSaveError = false;

			const char * name;
			entry_ref entry;
			if ((msg->FindString("name", &name) == B_OK) 
				&& (msg->FindRef("directory", &entry) == B_OK)) {
				BDirectory dir(&entry);
				BEntry saveTo(&dir, name, true);
				showSaveError = ((saveTo.InitCheck() != B_OK) 
				|| (_SaveKeySet(saveTo) == false));
			} else if (fLastSaved.InitCheck() == B_OK) {
				// We've saved this before, save over previous file.
				showSaveError = (_SaveKeySet(fLastSaved) == false);
			} else PostMessage(SAVE_KEYSET_AS); // open the save requester...

			if (showSaveError) {
				(new BAlert(ERROR, 
					B_TRANSLATE("Shortcuts wasn't able to save your keyset."), 
					B_TRANSLATE("OK")))->Go(NULL);
			}
			break;
		}

		case SAVE_KEYSET_AS:
		{
			if (fSavePanel)
				fSavePanel->Show();
			else {
				BMessage msg(SAVE_KEYSET);
				BMessenger messenger(this);
				fSavePanel = new BFilePanel(B_SAVE_PANEL, &messenger, NULL, 0, 
					false, &msg);
				fSavePanel->Show();
			}
			break;
		}

		case ADD_HOTKEY_ITEM:
			_AddNewSpec(NULL);
			break;

		case REMOVE_HOTKEY_ITEM:
		{
			int index = fColumnListView->CurrentSelection();
			if (index >= 0) {
				CLVListItem* item = (CLVListItem*) 
					fColumnListView->ItemAt(index);
				fColumnListView->RemoveItem(index);
				delete item;
				_MarkKeySetModified();

				// Rules for new selection: If there is an item at (index),
				// select it. Otherwise, if there is an item at (index-1), 
				// select it. Otherwise, select nothing.
				int num = fColumnListView->CountItems();
				if (num > 0) {
					if (index < num)
						fColumnListView->Select(index);
					else {
						if (index > 0)
							index--;
						if (index < num) 
							fColumnListView->Select(index);
					}
				}
			}
			break;
		}

		// Received when the user clicks on the ColumnListView
		case HOTKEY_ITEM_SELECTED:
		{
			int32 index = -1;
			msg->FindInt32("index", &index);
			bool validItem = (index >= 0);
			fRemoveButton->SetEnabled(validItem);
			break;
		}

		// Received when an entry is to be modified in response to GUI activity
		case HOTKEY_ITEM_MODIFIED:
		{
			int32 row, column;

			if ((msg->FindInt32("row", &row) == B_OK) 
				&& (msg->FindInt32("column", &column) == B_OK)) {
				int32 key;
				const char* bytes;

				if (row >= 0) {
					ShortcutsSpec* item = (ShortcutsSpec*)
						fColumnListView->ItemAt(row);
					bool repaintNeeded = false; // default

					if (msg->HasInt32("mouseClick")) {
						repaintNeeded = item->ProcessColumnMouseClick(column);
					} else if ((msg->FindString("bytes", &bytes) == B_OK)
						&& (msg->FindInt32("key", &key) == B_OK)) {
						repaintNeeded = item->ProcessColumnKeyStroke(column, 
							bytes, key);
					} else if (msg->FindInt32("unmappedkey", &key) == 
						B_OK) {
						repaintNeeded = ((column == item->KEY_COLUMN_INDEX) 
							&& ((key > 0xFF) || (GetKeyName(key) != NULL)) 
							&& (item->ProcessColumnKeyStroke(column, NULL, 
							key)));
					} else if (msg->FindString("text", &bytes) == B_OK) {
						if ((bytes[0] == '(')&&(bytes[1] == 'C')) {
							if (fSelectPanel)
								fSelectPanel->Show();
							else {
								BMessage msg(SELECT_APPLICATION);
								BMessenger m(this);
								fSelectPanel = new BFilePanel(B_OPEN_PANEL, &m,
									NULL, 0, false, &msg);
								fSelectPanel->Show();
							}
							fSelectPanel->SetButtonLabel(B_DEFAULT_BUTTON, 
								B_TRANSLATE("Select"));
						} else {
							repaintNeeded = item->ProcessColumnTextString(
								column, bytes);
						}
					}
					
					if (repaintNeeded) {
						fColumnListView->InvalidateItem(row);
						_MarkKeySetModified();
					}
				}
			}
			break;
		}

		default:
			BWindow::MessageReceived(msg);
			break;
	}
}


void
ShortcutsWindow::_MarkKeySetModified()
{
	if (fKeySetModified == false) {
		fKeySetModified = true;
		fSaveButton->SetEnabled(true);
	}
}


void
ShortcutsWindow::Quit()
{
	for (int i = fColumnListView->CountItems() - 1; i >= 0; i--)
		delete (ShortcutsSpec*)fColumnListView->ItemAt(i);

	fColumnListView->MakeEmpty();
	BWindow::Quit();
}


void
ShortcutsWindow::FrameResized(float w, float h)
{
	fAddButton->ChangeToNewSize(w, h);
	fRemoveButton->ChangeToNewSize(w, h);
	fSaveButton->ChangeToNewSize(w, h);
}


void
ShortcutsWindow::DispatchMessage(BMessage* msg, BHandler* handler)
{
	switch(msg->what) {
		case B_COPY:
		case B_CUT:
			if (be_clipboard->Lock()) {
				int32 row = fColumnListView->CurrentSelection(); 
				int32 column = fColumnListView->GetSelectedColumn();
				if ((row >= 0) 
					&& (column == ShortcutsSpec::STRING_COLUMN_INDEX)) {
					ShortcutsSpec* spec = (ShortcutsSpec*)
						fColumnListView->ItemAt(row);
					if (spec) {
						BMessage* data = be_clipboard->Data();
						data->RemoveName("text/plain");
						data->AddData("text/plain", B_MIME_TYPE, 
							spec->GetCellText(column), 
							strlen(spec->GetCellText(column)));
						be_clipboard->Commit();
						
						if (msg->what == B_CUT) {
							spec->ProcessColumnTextString(column, "");
							_MarkKeySetModified();
							fColumnListView->InvalidateItem(row);
						}
					}
				}
				be_clipboard->Unlock();
			}
			break;

		case B_PASTE:
			if (be_clipboard->Lock()) {
				BMessage* data = be_clipboard->Data();
				const char* text;
				ssize_t textLen;
				if (data->FindData("text/plain", B_MIME_TYPE, (const void**) 
					&text, &textLen) == B_OK) {
					int32 row = fColumnListView->CurrentSelection(); 
					int32 column = fColumnListView->GetSelectedColumn();
					if ((row >= 0) 
						&& (column == ShortcutsSpec::STRING_COLUMN_INDEX)) {
						ShortcutsSpec* spec = (ShortcutsSpec*) 
							fColumnListView->ItemAt(row);
						if (spec) {
							for (ssize_t i = 0; i < textLen; i++) {
								char buf[2] = {text[i], 0x00};
								spec->ProcessColumnKeyStroke(column, buf, 0);
							}
						}
						fColumnListView->InvalidateItem(row);
						_MarkKeySetModified();
					}
				}
				be_clipboard->Unlock();
			}
			break;
		
		default:
			BWindow::DispatchMessage(msg, handler);
			break;
	}
}

