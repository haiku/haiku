/*
 * Copyright 1999-2009 Jeremy Friesner
 * Copyright 2009-2010 Haiku, Inc. All rights reserved.
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
#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <ControlLook.h>
#include <File.h>
#include <FilePanel.h>
#include <FindDirectory.h>
#include <Input.h>
#include <LayoutBuilder.h>
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
#include <usb/USB_hid.h>
#include <usb/USB_hid_page_consumer.h>

#include "EditWindow.h"
#include "KeyInfos.h"
#include "MetaKeyStateMap.h"
#include "ParseCommandLine.h"
#include "PopUpColumn.h"
#include "ShortcutsFilterConstants.h"
#include "ShortcutsSpec.h"


// Window sizing constraints
#define MAX_WIDTH 10000
#define MAX_HEIGHT 10000
	// SetSizeLimits does not provide a mechanism for specifying an
	// unrestricted maximum. 10,000 seems to be the most common value used
	// in other Haiku system applications.

#define WINDOW_SETTINGS_FILE_NAME "Shortcuts_window_settings"
	// Because the "shortcuts_settings" file (SHORTCUTS_SETTING_FILE_NAME) is
	// already used as a communications method between this configurator and
	// the "shortcut_catcher" input_server filter, it should not be overloaded
	// with window position information, instead, a separate file is used.

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ShortcutsWindow"

#define ERROR "Shortcuts error"
#define WARNING "Shortcuts warning"


// Creates a pop-up-menu that reflects the possible states of the specified
// meta-key.
static BPopUpMenu*
CreateMetaPopUp(int column)
{
	MetaKeyStateMap& map = GetNthKeyMap(column);
	BPopUpMenu* popup = new BPopUpMenu(NULL, false);
	int stateCount = map.GetNumStates();

	for (int i = 0; i < stateCount; i++)
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
		if (next != NULL)
			popup->AddItem(new BMenuItem(next, NULL));
	}

	return popup;
}


ShortcutsWindow::ShortcutsWindow()
	:
	BWindow(BRect(0, 0, 0, 0), B_TRANSLATE_SYSTEM_NAME("Shortcuts"),
		B_TITLED_WINDOW, B_AUTO_UPDATE_SIZE_LIMITS),
	fSavePanel(NULL),
	fOpenPanel(NULL),
	fSelectPanel(NULL),
	fKeySetModified(false),
	fLastOpenWasAppend(false)
{
	ShortcutsSpec::InitializeMetaMaps();

	BMenuBar* menuBar = new BMenuBar("Menu Bar");

	BMenu* fileMenu = new BMenu(B_TRANSLATE("File"));
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Open KeySet" B_UTF8_ELLIPSIS),
		new BMessage(OPEN_KEYSET), 'O'));
	fileMenu->AddItem(new BMenuItem(
		B_TRANSLATE("Append KeySet" B_UTF8_ELLIPSIS),
		new BMessage(APPEND_KEYSET), 'A'));
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Revert to saved"),
		new BMessage(REVERT_KEYSET), 'R'));
	fileMenu->AddItem(new BSeparatorItem);
	fileMenu->AddItem(new BMenuItem(
		B_TRANSLATE("Save KeySet as" B_UTF8_ELLIPSIS),
		new BMessage(SAVE_KEYSET_AS), 'S'));
	fileMenu->AddItem(new BSeparatorItem);
	fileMenu->AddItem(new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(B_QUIT_REQUESTED), 'Q'));
	menuBar->AddItem(fileMenu);

	fColumnListView = new BColumnListView(NULL,
		B_WILL_DRAW | B_FRAME_EVENTS, B_FANCY_BORDER, false);

	float cellWidth = be_plain_font->StringWidth("Either") + 20;
		// ShortcutsSpec does not seem to translate the string "Either".

	for (int i = 0; i < ShortcutsSpec::NUM_META_COLUMNS; i++) {
		const char* name = ShortcutsSpec::GetColumnName(i);
		float headerWidth = be_plain_font->StringWidth(name) + 20;
		float width = max_c(headerWidth, cellWidth);

		fColumnListView->AddColumn(new PopUpColumn(CreateMetaPopUp(i), name,
				width, width - 1, width * 1.5, B_TRUNCATE_END, false, true, 1),
			fColumnListView->CountColumns());
	}

	float keyCellWidth = be_plain_font->StringWidth("Caps Lock") + 20;
	fColumnListView->AddColumn(new PopUpColumn(CreateKeysPopUp(),
			B_TRANSLATE("Key"), keyCellWidth, keyCellWidth - 10,
			keyCellWidth + 30, B_TRUNCATE_END),
		fColumnListView->CountColumns());
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
	fColumnListView->AddColumn(new PopUpColumn(popup, B_TRANSLATE("Application"),
			300.0, 223.0, 324.0, B_TRUNCATE_END, true),
		fColumnListView->CountColumns());

	fColumnListView->SetSelectionMessage(new BMessage(HOTKEY_ITEM_SELECTED));
	fColumnListView->SetSelectionMode(B_SINGLE_SELECTION_LIST);
	fColumnListView->SetTarget(this);

	fAddButton = new BButton("add", B_TRANSLATE("Add new shortcut"),
		new BMessage(ADD_HOTKEY_ITEM));

	fRemoveButton = new BButton("remove",
		B_TRANSLATE("Remove selected shortcut"),
		new BMessage(REMOVE_HOTKEY_ITEM));
	fRemoveButton->SetEnabled(false);

	fSaveButton = new BButton("save", B_TRANSLATE("Save & apply"),
		new BMessage(SAVE_KEYSET));
	fSaveButton->SetEnabled(false);

	CenterOnScreen();

	fColumnListView->ResizeAllColumnsToPreferred();

	entry_ref windowSettingsRef;
	if (_GetWindowSettingsFile(&windowSettingsRef)) {
		// The window settings file is not accepted via B_REFS_RECEIVED; this
		// is a behind-the-scenes file that the user will never see or
		// interact with.
		BFile windowSettingsFile(&windowSettingsRef, B_READ_ONLY);
		BMessage loadMessage;
		if (loadMessage.Unflatten(&windowSettingsFile) == B_OK)
			_LoadWindowSettings(loadMessage);
	}

	entry_ref keySetRef;
	if (_GetSettingsFile(&keySetRef)) {
		BMessage message(B_REFS_RECEIVED);
		message.AddRef("refs", &keySetRef);
		message.AddString("startupRef", "please");
		PostMessage(&message);
			// tell ourselves to load this file if it exists
	} else {
		_AddNewSpec("/bin/setvolume -t", (B_HID_USAGE_PAGE_CONSUMER << 16) | B_HID_UID_CON_MUTE);
		_AddNewSpec("/bin/setvolume -i", (B_HID_USAGE_PAGE_CONSUMER << 16) | B_HID_UID_CON_VOLUME_INCREMENT);
		_AddNewSpec("/bin/setvolume -d", (B_HID_USAGE_PAGE_CONSUMER << 16) | B_HID_UID_CON_VOLUME_DECREMENT);
		fLastSaved = BEntry(&keySetRef);
		PostMessage(SAVE_KEYSET);
	}

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(menuBar)
		.AddGroup(B_VERTICAL)
			.SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET))
			.SetInsets(B_USE_WINDOW_SPACING)
			.Add(fColumnListView)
			.AddGroup(B_HORIZONTAL)
				.AddGroup(B_HORIZONTAL)
				.SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP))
				.Add(fAddButton)
				.Add(fRemoveButton)
				.End()
				.AddGroup(B_HORIZONTAL)
					.SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_TOP))
					.Add(fSaveButton)
				.End()
			.End()
		.End();

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
	bool result = true;

	if (fKeySetModified) {
		BAlert* alert = new BAlert(WARNING,
			B_TRANSLATE("Save changes before closing?"),
			B_TRANSLATE("Cancel"), B_TRANSLATE("Don't save"),
			B_TRANSLATE("Save"));
		alert->SetShortcut(0, B_ESCAPE);
		alert->SetShortcut(1, 'd');
		alert->SetShortcut(2, 's');
		switch (alert->Go()) {
			case 0:
				result = false;
				break;

			case 1:
				result = true;
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
						result = true; // quit anyway
					}
				} else {
					PostMessage(SAVE_KEYSET);
					result = false;
				}
				break;
		}
	}

	if (result) {
		fColumnListView->DeselectAll();

		// Save the window position.
		entry_ref ref;
		if (_GetWindowSettingsFile(&ref)) {
			BEntry entry(&ref);
			_SaveWindowSettings(entry);
		}
	}

	return result;
}


bool
ShortcutsWindow::_GetSettingsFile(entry_ref* eref)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return false;
	else
		path.Append(SHORTCUTS_SETTING_FILE_NAME);
	BEntry entry(path.Path(), true);
	entry.GetRef(eref);
	return entry.Exists();
}


// Saves a settings file to (saveEntry). Returns true iff successful.
bool
ShortcutsWindow::_SaveKeySet(BEntry& saveEntry)
{
	BFile saveTo(&saveEntry, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (saveTo.InitCheck() != B_OK)
		return false;

	BMessage saveMessage;
	for (int i = 0; i < fColumnListView->CountRows(); i++) {
		BMessage next;
		if (((ShortcutsSpec*)fColumnListView->RowAt(i))->Archive(&next)
				== B_OK) {
			saveMessage.AddMessage("spec", &next);
		} else
			printf("Error archiving ShortcutsSpec #%i!\n", i);
	}

	bool result = (saveMessage.Flatten(&saveTo) == B_OK);

	if (result) {
		fKeySetModified = false;
		fSaveButton->SetEnabled(false);
	}

	return result;
}


// Appends new entries from the file specified in the "spec" entry of
// (loadMessage). Returns true iff successful.
bool
ShortcutsWindow::_LoadKeySet(const BMessage& loadMessage)
{
	int i = 0;
	BMessage message;
	while (loadMessage.FindMessage("spec", i++, &message) == B_OK) {
		ShortcutsSpec* spec
			= (ShortcutsSpec*)ShortcutsSpec::Instantiate(&message);
		if (spec != NULL)
			fColumnListView->AddRow(spec);
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

	BMessage columnsState;
	fColumnListView->SaveState(&columnsState);
	saveMsg.AddMessage ("columns state", &columnsState);

	saveMsg.Flatten(&saveTo);
}


// Loads the application settings file from (loadMessage) and resizes
// the interface to match the previously saved settings. Because this
// is a non-essential file, errors are ignored when loading the settings.
void
ShortcutsWindow::_LoadWindowSettings(const BMessage& loadMessage)
{
	BRect frame;
	if (loadMessage.FindRect("window frame", &frame) == B_OK) {
		// ensure the frame does not resize below the computed minimum.
		float width = max_c(Bounds().right, frame.right - frame.left);
		float height = max_c(Bounds().bottom, frame.bottom - frame.top);
		ResizeTo(width, height);

		// ensure the frame is not placed outside of the screen.
		BScreen screen(this);
		float left = min_c(screen.Frame().right - width, frame.left);
		float top = min_c(screen.Frame().bottom - height, frame.top);
		MoveTo(left, top);
	}

	BMessage columnsStateMessage;
	if (loadMessage.FindMessage ("columns state", &columnsStateMessage) == B_OK)
		fColumnListView->LoadState(&columnsStateMessage);
}


// Creates a new entry and adds it to the GUI. (defaultCommand) will be the
// text in the entry, or NULL if no text is desired.
void
ShortcutsWindow::_AddNewSpec(const char* defaultCommand, uint32 keyCode)
{
	_MarkKeySetModified();

	ShortcutsSpec* spec;
	BRow* curSel = fColumnListView->CurrentSelection();
	if (curSel)
		spec = new ShortcutsSpec(*((ShortcutsSpec*)curSel));
	else {
		spec = new ShortcutsSpec("");
		for (int i = 0; i < fColumnListView->CountColumns(); i++)
			spec->SetField(new BStringField(""), i);
	}

	fColumnListView->AddRow(spec);
	fColumnListView->AddToSelection(spec);
	fColumnListView->ScrollTo(spec);
	if (defaultCommand)
		spec->SetCommand(defaultCommand);
	if (keyCode != 0) {
		spec->ProcessColumnTextString(ShortcutsSpec::KEY_COLUMN_INDEX,
			GetFallbackKeyName(keyCode).String());
	}
}


void
ShortcutsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case OPEN_KEYSET:
		case APPEND_KEYSET:
			fLastOpenWasAppend = (message->what == APPEND_KEYSET);
			if (fOpenPanel)
				fOpenPanel->Show();
			else {
				BMessenger messenger(this);
				fOpenPanel = new BFilePanel(B_OPEN_PANEL, &messenger, NULL,
					0, false);
				fOpenPanel->Show();
			}
			fOpenPanel->SetButtonLabel(B_DEFAULT_BUTTON, fLastOpenWasAppend ?
				B_TRANSLATE("Append") : B_TRANSLATE("Open"));
			break;

		// send a message to myself, to get me to reload the settings file
		case REVERT_KEYSET:
		{
			fLastOpenWasAppend = false;
			BMessage reload(B_REFS_RECEIVED);
			entry_ref eref;
			_GetSettingsFile(&eref);
			reload.AddRef("refs", &eref);
			reload.AddString("startupRef", "yeah");
			PostMessage(&reload);
			break;
		}

		// respond to drag-and-drop messages here
		case B_SIMPLE_DATA:
		{
			int i = 0;

			entry_ref ref;
			while (message->FindRef("refs", i++, &ref) == B_OK) {
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

		// respond to FileRequester's messages here
		case B_REFS_RECEIVED:
		{
			// Find file ref
			entry_ref ref;
			bool isStartMsg = message->HasString("startupRef");
			if (message->FindRef("refs", &ref) == B_OK) {
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
					while (fColumnListView->CountRows()) {
						ShortcutsSpec* row =
							static_cast<ShortcutsSpec*>(fColumnListView->RowAt(0));
						fColumnListView->RemoveRow(row);
						delete row;
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

		// these messages come from the pop-up menu of the Applications column
		case SELECT_APPLICATION:
		{
			ShortcutsSpec* row =
				static_cast<ShortcutsSpec*>(fColumnListView->CurrentSelection());
			if (row != NULL) {
				entry_ref aref;
				if (message->FindRef("refs", &aref) == B_OK) {
					BEntry ent(&aref);
					if (ent.InitCheck() == B_OK) {
						BPath path;
						if ((ent.GetPath(&path) == B_OK)
							&& (row->
								ProcessColumnTextString(ShortcutsSpec::STRING_COLUMN_INDEX,
									path.Path()))) {
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

			const char* name;
			entry_ref entry;
			if ((message->FindString("name", &name) == B_OK)
				&& (message->FindRef("directory", &entry) == B_OK)) {
				BDirectory dir(&entry);
				BEntry saveTo(&dir, name, true);
				showSaveError = ((saveTo.InitCheck() != B_OK)
					|| (_SaveKeySet(saveTo) == false));
			} else if (fLastSaved.InitCheck() == B_OK) {
				// We've saved this before, save over previous file.
				showSaveError = (_SaveKeySet(fLastSaved) == false);
			} else
				PostMessage(SAVE_KEYSET_AS);
					// open the save requester...

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
				BMessage message(SAVE_KEYSET);
				BMessenger messenger(this);
				fSavePanel = new BFilePanel(B_SAVE_PANEL, &messenger, NULL, 0,
					false, &message);
				fSavePanel->Show();
			}
			break;
		}

		case ADD_HOTKEY_ITEM:
			_AddNewSpec(NULL);
			break;

		case REMOVE_HOTKEY_ITEM:
		{
			BRow* item = fColumnListView->CurrentSelection();
			if (item) {
				int index = fColumnListView->IndexOf(item);
				fColumnListView->RemoveRow(item);
				delete item;
				_MarkKeySetModified();

				// Rules for new selection: If there is an item at (index),
				// select it. Otherwise, if there is an item at (index-1),
				// select it. Otherwise, select nothing.
				int num = fColumnListView->CountRows();
				if (num > 0) {
					if (index < num)
						fColumnListView->AddToSelection(
							fColumnListView->RowAt(index));
					else {
						if (index > 0)
							index--;
						if (index < num)
							fColumnListView->AddToSelection(
								fColumnListView->RowAt(index));
					}
				}
			}
			break;
		}

		// Received when the user clicks on the ColumnListView
		case HOTKEY_ITEM_SELECTED:
		{
			if (fColumnListView->CountRows() > 0)
				fRemoveButton->SetEnabled(true);
			else
				fRemoveButton->SetEnabled(false);
			break;
		}

		// Received when an entry is to be modified in response to GUI activity
		case HOTKEY_ITEM_MODIFIED:
		{
			int32 row, column;

			if ((message->FindInt32("row", &row) == B_OK)
				&& (message->FindInt32("column", &column) == B_OK)) {
				int32 key;
				const char* bytes;

				if (row >= 0) {
					ShortcutsSpec* item = (ShortcutsSpec*)
						fColumnListView->RowAt(row);
					bool repaintNeeded = false; // default

					if (message->HasInt32("mouseClick")) {
						repaintNeeded = item->ProcessColumnMouseClick(column);
					} else if ((message->FindString("bytes", &bytes) == B_OK)
						&& (message->FindInt32("key", &key) == B_OK)) {
						repaintNeeded = item->ProcessColumnKeyStroke(column,
							bytes, key);
					} else if (message->FindInt32("unmappedkey", &key) ==
						B_OK) {
						repaintNeeded = ((column == item->KEY_COLUMN_INDEX)
							&& ((key > 0xFF) || (GetKeyName(key) != NULL))
							&& (item->ProcessColumnKeyStroke(column, NULL,
							key)));
					} else if (message->FindString("text", &bytes) == B_OK) {
						if ((bytes[0] == '(')&&(bytes[1] == 'C')) {
							if (fSelectPanel)
								fSelectPanel->Show();
							else {
								BMessage message(SELECT_APPLICATION);
								BMessenger m(this);
								fSelectPanel = new BFilePanel(B_OPEN_PANEL, &m,
									NULL, 0, false, &message);
								fSelectPanel->Show();
							}
							fSelectPanel->SetButtonLabel(B_DEFAULT_BUTTON,
								B_TRANSLATE("Select"));
						} else
							repaintNeeded = item->ProcessColumnTextString(
								column, bytes);
					}

					if (repaintNeeded) {
						fColumnListView->Invalidate(row);
						_MarkKeySetModified();
					}
				}
			}
			break;
		}

		default:
			BWindow::MessageReceived(message);
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
	BWindow::Quit();
}


void
ShortcutsWindow::DispatchMessage(BMessage* message, BHandler* handler)
{
	switch (message->what) {
		case B_SIMPLE_DATA:
			MessageReceived(message);
			break;

		case B_COPY:
		case B_CUT:
			if (be_clipboard->Lock()) {
				ShortcutsSpec* row =
					static_cast<ShortcutsSpec*>(fColumnListView->CurrentSelection());
				if (row) {
					BMessage* data = be_clipboard->Data();
					data->RemoveName("text/plain");
					data->AddData("text/plain", B_MIME_TYPE,
						row->GetCellText(ShortcutsSpec::STRING_COLUMN_INDEX),
						strlen(row->GetCellText(ShortcutsSpec::STRING_COLUMN_INDEX)));
					be_clipboard->Commit();

					if (message->what == B_CUT) {
						row->ProcessColumnTextString(
							ShortcutsSpec::STRING_COLUMN_INDEX, "");
						_MarkKeySetModified();
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
					ShortcutsSpec* row =
					static_cast<ShortcutsSpec*>(fColumnListView->CurrentSelection());
					if (row) {
						for (ssize_t i = 0; i < textLen; i++) {
							char buf[2] = {text[i], 0x00};
							row->ProcessColumnKeyStroke(
								ShortcutsSpec::STRING_COLUMN_INDEX, buf, 0);
						}
					}
					_MarkKeySetModified();
				}
				be_clipboard->Unlock();
			}
			break;

		case B_KEY_DOWN:
		case B_UNMAPPED_KEY_DOWN:
		{
			ShortcutsSpec* selected;
			int32 modifiers = message->GetInt32("modifiers", 0);
			// These should not block key detection here:
			modifiers &= ~(B_CAPS_LOCK | B_SCROLL_LOCK | B_NUM_LOCK);
			if (modifiers != 0)
				BWindow::DispatchMessage(message, handler);
			else if (handler == fColumnListView
				&& (selected =
					static_cast<ShortcutsSpec*>(fColumnListView->CurrentSelection()))) {
				uint32 keyCode = message->GetInt32("key", 0);
				const char* keyName = GetKeyName(keyCode);
				selected->ProcessColumnTextString(
						ShortcutsSpec::KEY_COLUMN_INDEX,
						keyName != NULL ? keyName : GetFallbackKeyName(keyCode).String());
				_MarkKeySetModified();
			}
			break;
		}
		default:
			BWindow::DispatchMessage(message, handler);
			break;
	}
}
