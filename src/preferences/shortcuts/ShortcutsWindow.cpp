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
#include <Button.h>
#include <Catalog.h>
#include <Clipboard.h>
#include <ControlLook.h>
#include <File.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <Input.h>
#include <Locale.h>
#include <Message.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <MessageFilter.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <String.h>
#include <SupportDefs.h>

#include "ColumnListView.h"

#include "KeyInfos.h"
#include "MetaKeyStateMap.h"
#include "ParseCommandLine.h"
#include "ShortcutsFilterConstants.h"
#include "ShortcutsSpec.h"


// Window sizing constraints
#define MAX_WIDTH 10000
#define MAX_HEIGHT 10000
	// SetSizeLimits does not provide a mechanism for specifying an
	// unrestricted maximum.  10,000 seems to be the most common value used
	// in other Haiku system applications.

#define WINDOW_SETTINGS_FILE_NAME "Shortcuts_window_settings"
	// Because the "shortcuts_settings" file (SHORTCUTS_SETTING_FILE_NAME) is
	// already used as a communications method between this configurator and
	// the "shortcut_catcher" input_server filter, it should not be overloaded
	// with window position information.  Instead, a separate file is used.

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ShortcutsWindow"

#define ERROR "Shortcuts error"
#define WARNING "Shortcuts warning"


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
	BWindow(BRect(0, 0, 0, 0), B_TRANSLATE_SYSTEM_NAME("Shortcuts"),
		B_TITLED_WINDOW, 0L),
	fSavePanel(NULL), 
	fOpenPanel(NULL), 
	fSelectPanel(NULL), 
	fKeySetModified(false), 		
	fLastOpenWasAppend(false)
{
	ShortcutsSpec::InitializeMetaMaps();
	
	float spacing = be_control_look->DefaultItemSpacing();

	BView* top = new BView(Bounds(), NULL, B_FOLLOW_ALL_SIDES, 0);
	top->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(top);

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

	top->AddChild(menuBar);

	BRect tableBounds = Bounds();
	tableBounds.top = menuBar->Bounds().bottom + 1;
	tableBounds.right -= B_V_SCROLL_BAR_WIDTH;
	tableBounds.bottom -= B_H_SCROLL_BAR_HEIGHT;

	BScrollView* containerView;
	fColumnListView = new ColumnListView(tableBounds, &containerView, NULL,
		B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
		B_SINGLE_SELECTION_LIST, true, true, true, B_NO_BORDER);

	fColumnListView->SetEditMessage(new BMessage(HOTKEY_ITEM_MODIFIED),
		BMessenger(this));

	float minListWidth = 0;
		// A running total is kept as the columns are created.
	float cellWidth = be_plain_font->StringWidth("Either") + 20;
		// ShortcutsSpec does not seem to translate the string "Either".

	for (int i = 0; i < ShortcutsSpec::NUM_META_COLUMNS; i++) {
		const char* name = ShortcutsSpec::GetColumnName(i);
		float headerWidth = be_plain_font->StringWidth(name) + 20;
		float width = max_c(headerWidth, cellWidth);
		minListWidth += width + 1;

		fColumnListView->AddColumn(
			new CLVColumn(name, CreateMetaPopUp(i), width, CLV_SORT_KEYABLE));
	}

	float keyCellWidth = be_plain_font->StringWidth("Caps Lock") + 20;
	fColumnListView->AddColumn(new CLVColumn(B_TRANSLATE("Key"),
		CreateKeysPopUp(), keyCellWidth, CLV_SORT_KEYABLE));
	minListWidth += keyCellWidth + 1;

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
	minListWidth += 323.0 + 1;
	minListWidth += B_V_SCROLL_BAR_WIDTH;

	fColumnListView->SetSortFunction(ShortcutsSpec::MyCompare);
	top->AddChild(containerView);

	fColumnListView->SetSelectionMessage(new BMessage(HOTKEY_ITEM_SELECTED));
	fColumnListView->SetTarget(this);

	fAddButton = new BButton(BRect(0, 0, 0, 0), "add",
		B_TRANSLATE("Add new shortcut"), new BMessage(ADD_HOTKEY_ITEM),
		B_FOLLOW_BOTTOM);
	fAddButton->ResizeToPreferred();
	fAddButton->MoveBy(spacing,
		Bounds().bottom - fAddButton->Bounds().bottom - spacing);
	top->AddChild(fAddButton);

	fRemoveButton = new BButton(BRect(0, 0, 0, 0), "remove",
		B_TRANSLATE("Remove selected shortcut"),
		new BMessage(REMOVE_HOTKEY_ITEM), B_FOLLOW_BOTTOM);
	fRemoveButton->ResizeToPreferred();
	fRemoveButton->MoveBy(fAddButton->Frame().right + spacing,
		Bounds().bottom - fRemoveButton->Bounds().bottom - spacing);
	top->AddChild(fRemoveButton);

	fRemoveButton->SetEnabled(false);

	fSaveButton = new BButton(BRect(0, 0, 0, 0), "save",
		B_TRANSLATE("Save & apply"), new BMessage(SAVE_KEYSET),
		B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	fSaveButton->ResizeToPreferred();
	fSaveButton->MoveBy(Bounds().right - fSaveButton->Bounds().right - spacing,
		Bounds().bottom - fSaveButton->Bounds().bottom - spacing);
	top->AddChild(fSaveButton);

	fSaveButton->SetEnabled(false);

	containerView->ResizeBy(0,
		-(fAddButton->Bounds().bottom + 2 * spacing + 2));

	float minButtonBarWidth = fRemoveButton->Frame().right
		+ fSaveButton->Bounds().right + 2 * spacing;
	float minWidth = max_c(minListWidth, minButtonBarWidth);

	float menuBarHeight = menuBar->Bounds().bottom;
	float buttonBarHeight = Bounds().bottom - containerView->Frame().bottom;
	float minHeight = menuBarHeight + 200 + buttonBarHeight;

	SetSizeLimits(minWidth, MAX_WIDTH, minHeight, MAX_HEIGHT);
		// SetSizeLimits() will resize the window to the minimum size.

	CenterOnScreen();

	entry_ref windowSettingsRef;
	if (_GetWindowSettingsFile(&windowSettingsRef)) {
		// The window settings file is not accepted via B_REFS_RECEIVED; this
		// is a behind-the-scenes file that the user will never see or
		// interact with.
		BFile windowSettingsFile(&windowSettingsRef, B_READ_ONLY);
		BMessage loadMsg;
		if (loadMsg.Unflatten(&windowSettingsFile) == B_OK)
			_LoadWindowSettings(loadMsg);
	}

	entry_ref keySetRef;
	if (_GetSettingsFile(&keySetRef)) {
		BMessage msg(B_REFS_RECEIVED);
		msg.AddRef("refs", &keySetRef);
		msg.AddString("startupRef", "please");
		PostMessage(&msg);
			// Tell ourselves to load this file if it exists.
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
		alert->SetShortcut(1, B_ESCAPE);
		switch(alert->Go()) {
			case 1:
				ret = false;
				break;

			case 2:
				// Save: automatically if possible, otherwise go back and open
				// up the file requester
				if (fLastSaved.InitCheck() == B_OK) {
					if (_SaveKeySet(fLastSaved) == false) {
						BAlert* alert = new BAlert(ERROR, 
							B_TRANSLATE("Shortcuts was unable to save your "
								"KeySet file!"), 
							B_TRANSLATE("Oh no"));
						alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
						alert->Go();
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

	if (ret) {
		fColumnListView->DeselectAll();

		// Save the window position.
		entry_ref ref;
		if (_GetWindowSettingsFile(&ref)) {
			BEntry entry(&ref);
			_SaveWindowSettings(entry);
		}
	}
	
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


// Gets the filesystem location of the "Shortcuts_window_settings" file.
bool
ShortcutsWindow::_GetWindowSettingsFile(entry_ref* eref)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return false;
	else
		path.Append(WINDOW_SETTINGS_FILE_NAME);

	return BEntry(path.Path(), true).GetRef(eref) == B_OK;
}


// Saves the application settings file to (saveEntry).  Because this is a
// non-essential file, errors are ignored when writing the settings.
void
ShortcutsWindow::_SaveWindowSettings(BEntry& saveEntry)
{
	BFile saveTo(&saveEntry, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (saveTo.InitCheck() != B_OK)
		return;

	BMessage saveMsg;
	saveMsg.AddRect("window frame", Frame());

	for (int i = 0; i < fColumnListView->CountColumns(); i++) {
		CLVColumn* column = fColumnListView->ColumnAt(i);
		saveMsg.AddFloat("column width", column->Width());
	}

	saveMsg.Flatten(&saveTo);
}


// Loads the application settings file from (loadMsg) and resizes the interface
// to match the previously saved settings.  Because this is a non-essential
// file, errors are ignored when loading the settings.
void
ShortcutsWindow::_LoadWindowSettings(const BMessage& loadMsg)
{
	BRect frame;
	if (loadMsg.FindRect("window frame", &frame) == B_OK) {
		// Ensure the frame does not resize below the computed minimum.
		float width = max_c(Bounds().right, frame.right - frame.left);
		float height = max_c(Bounds().bottom, frame.bottom - frame.top);
		ResizeTo(width, height);

		// Ensure the frame is not placed outside of the screen.
		BScreen screen(this);
		float left = min_c(screen.Frame().right - width, frame.left);
		float top = min_c(screen.Frame().bottom - height, frame.top);
		MoveTo(left, top);
	}

	for (int i = 0; i < fColumnListView->CountColumns(); i++) {
		CLVColumn* column = fColumnListView->ColumnAt(i);
		float columnWidth;
		if (loadMsg.FindFloat("column width", i, &columnWidth) == B_OK)
			column->SetWidth(max_c(column->Width(), columnWidth));
	}
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
							BAlert* alert = new BAlert(ERROR, 
								B_TRANSLATE("Shortcuts was couldn't open your "
								"KeySet file!"), B_TRANSLATE("OK"));
							alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
							alert->Go(NULL);
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
					BAlert* alert = new BAlert(ERROR, 
						B_TRANSLATE("Shortcuts was unable to parse your "
						"KeySet file!"), B_TRANSLATE("OK"));
					alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
					alert->Go(NULL);
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
				BAlert* alert = new BAlert(ERROR, 
					B_TRANSLATE("Shortcuts wasn't able to save your keyset."), 
					B_TRANSLATE("OK"));
				alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
				alert->Go(NULL);
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

