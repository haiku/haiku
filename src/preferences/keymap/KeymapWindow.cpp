/*
 * Copyright 2004-2015 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexandre Deckner, alex@zappotek.com
 *		Axel Dörfler, axeld@pinc-software.de
 *		Jérôme Duval
 *		John Scipione, jscipione@gmai.com
 *		Sandor Vroemisse
 */


#include "KeymapWindow.h"

#include <string.h>
#include <stdio.h>

#include <Alert.h>
#include <Button.h>
#include <Catalog.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <Locale.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Screen.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextControl.h>

#include "KeyboardLayoutView.h"
#include "KeymapApplication.h"
#include "KeymapListItem.h"
#include "KeymapNames.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Keymap window"


static const uint32 kMsgMenuFileOpen = 'mMFO';
static const uint32 kMsgMenuFileSaveAs = 'mMFA';

static const uint32 kChangeKeyboardLayout = 'cKyL';

static const uint32 kMsgSwitchShortcuts = 'swSc';

static const uint32 kMsgMenuFontChanged = 'mMFC';

static const uint32 kMsgSystemMapSelected = 'SmST';
static const uint32 kMsgUserMapSelected = 'UmST';

static const uint32 kMsgDefaultKeymap = 'Dflt';
static const uint32 kMsgRevertKeymap = 'Rvrt';
static const uint32 kMsgKeymapUpdated = 'kMup';

static const uint32 kMsgDeadKeyAcuteChanged = 'dkAc';
static const uint32 kMsgDeadKeyCircumflexChanged = 'dkCc';
static const uint32 kMsgDeadKeyDiaeresisChanged = 'dkDc';
static const uint32 kMsgDeadKeyGraveChanged = 'dkGc';
static const uint32 kMsgDeadKeyTildeChanged = 'dkTc';

static const char* kDeadKeyTriggerNone = "<none>";

static const char* kCurrentKeymapName = "(Current)";
static const char* kDefaultKeymapName = "US-International";


static int
compare_key_list_items(const void* a, const void* b)
{
	KeymapListItem* item1 = *(KeymapListItem**)a;
	KeymapListItem* item2 = *(KeymapListItem**)b;
	return BLocale::Default()->StringCompare(item1->Text(), item2->Text());
}


KeymapWindow::KeymapWindow()
	:
	BWindow(BRect(80, 50, 650, 300), B_TRANSLATE_SYSTEM_NAME("Keymap"),
		B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
{
	fKeyboardLayoutView = new KeyboardLayoutView("layout");
	fKeyboardLayoutView->SetKeymap(&fCurrentMap);
	fKeyboardLayoutView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 192));

	fTextControl = new BTextControl(B_TRANSLATE("Sample and clipboard:"),
		"", NULL);

	fSwitchShortcutsButton = new BButton("switch", "",
		new BMessage(kMsgSwitchShortcuts));

	// controls pane
	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(_CreateMenu())
		.AddGroup(B_HORIZONTAL)
			.SetInsets(B_USE_WINDOW_SPACING)
			.Add(_CreateMapLists(), 0.25)
			.AddGroup(B_VERTICAL)
				.Add(fKeyboardLayoutView)
				.AddGroup(B_HORIZONTAL)
					.Add(_CreateDeadKeyMenuField(), 0.0)
					.AddGlue()
					.Add(fSwitchShortcutsButton)
					.End()
				.Add(fTextControl)
				.AddGlue(0.0)
				.AddGroup(B_HORIZONTAL)
					.AddGlue()
					.Add(fDefaultsButton = new BButton("defaultsButton",
						B_TRANSLATE("Defaults"),
							new BMessage(kMsgDefaultKeymap)))
					.Add(fRevertButton = new BButton("revertButton",
						B_TRANSLATE("Revert"), new BMessage(kMsgRevertKeymap)))
					.End()
				.End()
			.End()
		.End();

	fKeyboardLayoutView->SetTarget(fTextControl->TextView());
	fTextControl->MakeFocus();

	// Make sure the user keymap directory exists
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append("Keymap");

	entry_ref ref;
	BEntry entry(path.Path(), true); // follow symlink
	BDirectory userKeymapsDir(&entry);
	if (userKeymapsDir.InitCheck() != B_OK
		&& create_directory(path.Path(), S_IRWXU | S_IRWXG | S_IRWXO)
			== B_OK) {
		get_ref_for_path(path.Path(), &ref);
	} else if (entry.InitCheck() == B_OK)
		entry.GetRef(&ref);
	else
		get_ref_for_path(path.Path(), &ref);

	BMessenger messenger(this);
	fOpenPanel = new BFilePanel(B_OPEN_PANEL, &messenger, &ref,
		B_FILE_NODE, false, NULL);
	fSavePanel = new BFilePanel(B_SAVE_PANEL, &messenger, &ref,
		B_FILE_NODE, false, NULL);

	BRect windowFrame;
	if (_LoadSettings(windowFrame) == B_OK) {
		ResizeTo(windowFrame.Width(), windowFrame.Height());
		MoveTo(windowFrame.LeftTop());
		MoveOnScreen();
	} else
		CenterOnScreen();

	// TODO: this might be a bug in the interface kit, but scrolling to
	// selection does not correctly work unless the window is shown.
	Show();
	Lock();

	// Try and find the current map name in the two list views (if the name
	// was read at all)
	_SelectCurrentMap();

	KeymapListItem* current
		= static_cast<KeymapListItem*>(fUserListView->FirstItem());

	fCurrentMap.Load(current->EntryRef());
	fPreviousMap = fCurrentMap;
	fAppliedMap = fCurrentMap;
	fCurrentMap.SetTarget(this, new BMessage(kMsgKeymapUpdated));

	_UpdateButtons();

	_UpdateDeadKeyMenu();
	_UpdateSwitchShortcutButton();

	Unlock();
}


KeymapWindow::~KeymapWindow()
{
	delete fOpenPanel;
	delete fSavePanel;
}


bool
KeymapWindow::QuitRequested()
{
	_SaveSettings();

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
KeymapWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
		{
			entry_ref ref;
			int32 i = 0;
			while (message->FindRef("refs", i++, &ref) == B_OK) {
				fCurrentMap.Load(ref);
				fAppliedMap = fCurrentMap;
			}
			fKeyboardLayoutView->SetKeymap(&fCurrentMap);
			fSystemListView->DeselectAll();
			fUserListView->DeselectAll();
			break;
		}

		case B_SAVE_REQUESTED:
		{
			entry_ref ref;
			const char* name;
			if (message->FindRef("directory", &ref) == B_OK
				&& message->FindString("name", &name) == B_OK) {
				BDirectory directory(&ref);
				BEntry entry(&directory, name);
				entry.GetRef(&ref);
				fCurrentMap.SetName(name);
				fCurrentMap.Save(ref);
				fAppliedMap = fCurrentMap;
				_FillUserMaps();
				fCurrentMapName = name;
				_SelectCurrentMap();
			}
			break;
		}

		case kMsgMenuFileOpen:
			fOpenPanel->Show();
			break;
		case kMsgMenuFileSaveAs:
			fSavePanel->Show();
			break;
		case kMsgShowModifierKeysWindow:
			be_app->PostMessage(kMsgShowModifierKeysWindow);
			break;

		case kChangeKeyboardLayout:
		{
			entry_ref ref;
			BPath path;
			if (message->FindRef("ref", &ref) == B_OK)
				path.SetTo(&ref);

			_SetKeyboardLayout(path.Path());
			break;
		}

		case kMsgSwitchShortcuts:
			_SwitchShortcutKeys();
			break;

		case kMsgMenuFontChanged:
		{
			BMenuItem* item = fFontMenu->FindMarked();
			if (item != NULL) {
				BFont font;
				font.SetFamilyAndStyle(item->Label(), NULL);
				fKeyboardLayoutView->SetBaseFont(font);
				fTextControl->TextView()->SetFontAndColor(&font);
			}
			break;
		}

		case kMsgSystemMapSelected:
		case kMsgUserMapSelected:
		{
			BListView* listView;
			BListView* otherListView;

			if (message->what == kMsgSystemMapSelected) {
				listView = fSystemListView;
				otherListView = fUserListView;
			} else {
				listView = fUserListView;
				otherListView = fSystemListView;
			}

			int32 index = listView->CurrentSelection();
			if (index < 0)
				break;

			// Deselect item in other BListView
			otherListView->DeselectAll();

			if (index == 0 && listView == fUserListView) {
				// we can safely ignore the "(Current)" item
				break;
			}

			KeymapListItem* item
				= static_cast<KeymapListItem*>(listView->ItemAt(index));
			if (item != NULL) {
				status_t status = fCurrentMap.Load(item->EntryRef());
				if (status != B_OK) {
					listView->RemoveItem(item);
					break;
				}

				fAppliedMap = fCurrentMap;
				fKeyboardLayoutView->SetKeymap(&fCurrentMap);
				_UseKeymap();
				_UpdateButtons();
			}
			break;
		}

		case kMsgDefaultKeymap:
			_DefaultKeymap();
			_UpdateButtons();
			break;

		case kMsgRevertKeymap:
			_RevertKeymap();
			_UpdateButtons();
			break;

		case kMsgUpdateNormalKeys:
		{
			uint32 keyCode;
			if (message->FindUInt32("keyCode", &keyCode) != B_OK)
				break;

			bool unset;
			if (message->FindBool("unset", &unset) == B_OK && unset) {
				fCurrentMap.SetKey(keyCode, modifiers(), 0, "", 0);
				_UpdateButtons();
				fKeyboardLayoutView->SetKeymap(&fCurrentMap);
			}
			break;
		}

		case kMsgUpdateModifierKeys:
		{
			uint32 keyCode;
			bool unset;
			if (message->FindBool("unset", &unset) != B_OK)
				unset = false;

			if (message->FindUInt32("left_shift_key", &keyCode) == B_OK) {
				fCurrentMap.SetModifier(unset ? 0x00 : keyCode,
					B_LEFT_SHIFT_KEY);
			}

			if (message->FindUInt32("right_shift_key", &keyCode) == B_OK) {
				fCurrentMap.SetModifier(unset ? 0x00 : keyCode,
					B_RIGHT_SHIFT_KEY);
			}

			if (message->FindUInt32("left_control_key", &keyCode) == B_OK) {
				fCurrentMap.SetModifier(unset ? 0x00 : keyCode,
					B_LEFT_CONTROL_KEY);
			}

			if (message->FindUInt32("right_control_key", &keyCode) == B_OK) {
				fCurrentMap.SetModifier(unset ? 0x00 : keyCode,
					B_RIGHT_CONTROL_KEY);
			}

			if (message->FindUInt32("left_option_key", &keyCode) == B_OK) {
				fCurrentMap.SetModifier(unset ? 0x00 : keyCode,
					B_LEFT_OPTION_KEY);
			}

			if (message->FindUInt32("right_option_key", &keyCode) == B_OK) {
				fCurrentMap.SetModifier(unset ? 0x00 : keyCode,
					B_RIGHT_OPTION_KEY);
			}

			if (message->FindUInt32("left_command_key", &keyCode) == B_OK) {
				fCurrentMap.SetModifier(unset ? 0x00 : keyCode,
					B_LEFT_COMMAND_KEY);
			}

			if (message->FindUInt32("right_command_key", &keyCode) == B_OK) {
				fCurrentMap.SetModifier(unset ? 0x00 : keyCode,
					B_RIGHT_COMMAND_KEY);
			}

			if (message->FindUInt32("menu_key", &keyCode) == B_OK)
				fCurrentMap.SetModifier(unset ? 0x00 : keyCode, B_MENU_KEY);

			if (message->FindUInt32("caps_key", &keyCode) == B_OK)
				fCurrentMap.SetModifier(unset ? 0x00 : keyCode, B_CAPS_LOCK);

			if (message->FindUInt32("num_key", &keyCode) == B_OK)
				fCurrentMap.SetModifier(unset ? 0x00 : keyCode, B_NUM_LOCK);

			if (message->FindUInt32("scroll_key", &keyCode) == B_OK)
				fCurrentMap.SetModifier(unset ? 0x00 : keyCode, B_SCROLL_LOCK);

			_UpdateButtons();
			fKeyboardLayoutView->SetKeymap(&fCurrentMap);
			break;
		}

		case kMsgKeymapUpdated:
			_UpdateButtons();
			fSystemListView->DeselectAll();
			fUserListView->Select(0L);
			break;

		case kMsgDeadKeyAcuteChanged:
		{
			BMenuItem* item = fAcuteMenu->FindMarked();
			if (item != NULL) {
				const char* trigger = item->Label();
				if (strcmp(trigger, kDeadKeyTriggerNone) == 0)
					trigger = NULL;
				fCurrentMap.SetDeadKeyTrigger(kDeadKeyAcute, trigger);
				fKeyboardLayoutView->Invalidate();
			}
			break;
		}

		case kMsgDeadKeyCircumflexChanged:
		{
			BMenuItem* item = fCircumflexMenu->FindMarked();
			if (item != NULL) {
				const char* trigger = item->Label();
				if (strcmp(trigger, kDeadKeyTriggerNone) == 0)
					trigger = NULL;
				fCurrentMap.SetDeadKeyTrigger(kDeadKeyCircumflex, trigger);
				fKeyboardLayoutView->Invalidate();
			}
			break;
		}

		case kMsgDeadKeyDiaeresisChanged:
		{
			BMenuItem* item = fDiaeresisMenu->FindMarked();
			if (item != NULL) {
				const char* trigger = item->Label();
				if (strcmp(trigger, kDeadKeyTriggerNone) == 0)
					trigger = NULL;
				fCurrentMap.SetDeadKeyTrigger(kDeadKeyDiaeresis, trigger);
				fKeyboardLayoutView->Invalidate();
			}
			break;
		}

		case kMsgDeadKeyGraveChanged:
		{
			BMenuItem* item = fGraveMenu->FindMarked();
			if (item != NULL) {
				const char* trigger = item->Label();
				if (strcmp(trigger, kDeadKeyTriggerNone) == 0)
					trigger = NULL;
				fCurrentMap.SetDeadKeyTrigger(kDeadKeyGrave, trigger);
				fKeyboardLayoutView->Invalidate();
			}
			break;
		}

		case kMsgDeadKeyTildeChanged:
		{
			BMenuItem* item = fTildeMenu->FindMarked();
			if (item != NULL) {
				const char* trigger = item->Label();
				if (strcmp(trigger, kDeadKeyTriggerNone) == 0)
					trigger = NULL;
				fCurrentMap.SetDeadKeyTrigger(kDeadKeyTilde, trigger);
				fKeyboardLayoutView->Invalidate();
			}
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


BMenuBar*
KeymapWindow::_CreateMenu()
{
	BMenuBar* menuBar = new BMenuBar(Bounds(), "menubar");

	// Create the File menu
	BMenu* menu = new BMenu(B_TRANSLATE("File"));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Open" B_UTF8_ELLIPSIS),
		new BMessage(kMsgMenuFileOpen), 'O'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Save as" B_UTF8_ELLIPSIS),
		new BMessage(kMsgMenuFileSaveAs)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(
		B_TRANSLATE("Set modifier keys" B_UTF8_ELLIPSIS),
		new BMessage(kMsgShowModifierKeysWindow)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Quit"),
		new BMessage(B_QUIT_REQUESTED), 'Q'));
	menuBar->AddItem(menu);

	// Create keyboard layout menu
	fLayoutMenu = new BMenu(B_TRANSLATE("Layout"));
	_AddKeyboardLayouts(fLayoutMenu);
	menuBar->AddItem(fLayoutMenu);

	// Create the Font menu
	fFontMenu = new BMenu(B_TRANSLATE("Font"));
	fFontMenu->SetRadioMode(true);
	int32 numFamilies = count_font_families();
	font_family family, currentFamily;
	font_style currentStyle;
	uint32 flags;

	be_plain_font->GetFamilyAndStyle(&currentFamily, &currentStyle);

	for (int32 i = 0; i < numFamilies; i++) {
		if (get_font_family(i, &family, &flags) == B_OK) {
			BMenuItem* item
				= new BMenuItem(family, new BMessage(kMsgMenuFontChanged));
			fFontMenu->AddItem(item);

			if (!strcmp(family, currentFamily))
				item->SetMarked(true);
		}
	}
	menuBar->AddItem(fFontMenu);

	return menuBar;
}


BMenuField*
KeymapWindow::_CreateDeadKeyMenuField()
{
	BPopUpMenu* deadKeyMenu = new BPopUpMenu(B_TRANSLATE("Select dead keys"),
		false, false);

	fAcuteMenu = new BMenu(B_TRANSLATE("Acute trigger"));
	fAcuteMenu->SetRadioMode(true);
	fAcuteMenu->AddItem(new BMenuItem("\xC2\xB4",
		new BMessage(kMsgDeadKeyAcuteChanged)));
	fAcuteMenu->AddItem(new BMenuItem("'",
		new BMessage(kMsgDeadKeyAcuteChanged)));
	fAcuteMenu->AddItem(new BMenuItem(kDeadKeyTriggerNone,
		new BMessage(kMsgDeadKeyAcuteChanged)));
	deadKeyMenu->AddItem(fAcuteMenu);

	fCircumflexMenu = new BMenu(B_TRANSLATE("Circumflex trigger"));
	fCircumflexMenu->SetRadioMode(true);
	fCircumflexMenu->AddItem(new BMenuItem("^",
		new BMessage(kMsgDeadKeyCircumflexChanged)));
	fCircumflexMenu->AddItem(new BMenuItem(kDeadKeyTriggerNone,
		new BMessage(kMsgDeadKeyCircumflexChanged)));
	deadKeyMenu->AddItem(fCircumflexMenu);

	fDiaeresisMenu = new BMenu(B_TRANSLATE("Diaeresis trigger"));
	fDiaeresisMenu->SetRadioMode(true);
	fDiaeresisMenu->AddItem(new BMenuItem("\xC2\xA8",
		new BMessage(kMsgDeadKeyDiaeresisChanged)));
	fDiaeresisMenu->AddItem(new BMenuItem("\"",
		new BMessage(kMsgDeadKeyDiaeresisChanged)));
	fDiaeresisMenu->AddItem(new BMenuItem(kDeadKeyTriggerNone,
		new BMessage(kMsgDeadKeyDiaeresisChanged)));
	deadKeyMenu->AddItem(fDiaeresisMenu);

	fGraveMenu = new BMenu(B_TRANSLATE("Grave trigger"));
	fGraveMenu->SetRadioMode(true);
	fGraveMenu->AddItem(new BMenuItem("`",
		new BMessage(kMsgDeadKeyGraveChanged)));
	fGraveMenu->AddItem(new BMenuItem(kDeadKeyTriggerNone,
		new BMessage(kMsgDeadKeyGraveChanged)));
	deadKeyMenu->AddItem(fGraveMenu);

	fTildeMenu = new BMenu(B_TRANSLATE("Tilde trigger"));
	fTildeMenu->SetRadioMode(true);
	fTildeMenu->AddItem(new BMenuItem("~",
		new BMessage(kMsgDeadKeyTildeChanged)));
	fTildeMenu->AddItem(new BMenuItem(kDeadKeyTriggerNone,
		new BMessage(kMsgDeadKeyTildeChanged)));
	deadKeyMenu->AddItem(fTildeMenu);

	return new BMenuField(NULL, deadKeyMenu);
}


BView*
KeymapWindow::_CreateMapLists()
{
	// The System list
	fSystemListView = new BListView("systemList");
	fSystemListView->SetSelectionMessage(new BMessage(kMsgSystemMapSelected));

	BScrollView* systemScroller = new BScrollView("systemScrollList",
		fSystemListView, 0, false, true);

	// The User list
	fUserListView = new BListView("userList");
	fUserListView->SetSelectionMessage(new BMessage(kMsgUserMapSelected));
	BScrollView* userScroller = new BScrollView("userScrollList",
		fUserListView, 0, false, true);

	// Saved keymaps

	_FillSystemMaps();
	_FillUserMaps();

	_SetListViewSize(fSystemListView);
	_SetListViewSize(fUserListView);

	return BLayoutBuilder::Group<>(B_VERTICAL)
		.Add(new BStringView("system", B_TRANSLATE("System:")))
		.Add(systemScroller, 3)
		.Add(new BStringView("user", B_TRANSLATE("User:")))
		.Add(userScroller)
		.View();
}


void
KeymapWindow::_AddKeyboardLayouts(BMenu* menu)
{
	directory_which dataDirectories[] = {
		B_USER_NONPACKAGED_DATA_DIRECTORY,
		B_USER_DATA_DIRECTORY,
		B_SYSTEM_NONPACKAGED_DATA_DIRECTORY,
		B_SYSTEM_DATA_DIRECTORY,
	};

	for (uint32 i = 0;
			i < sizeof(dataDirectories) / sizeof(dataDirectories[0]); i++) {
		BPath path;
		if (find_directory(dataDirectories[i], &path) != B_OK)
			continue;

		if (path.Append("KeyboardLayouts") != B_OK)
			continue;

		BDirectory directory;
		if (directory.SetTo(path.Path()) == B_OK)
			_AddKeyboardLayoutMenu(menu, directory);
	}
}


/*!	Adds a menu populated with the keyboard layouts found in the passed
	in directory to the passed in menu. Each subdirectory in the passed
	in directory is added as a submenu recursively.
*/
void
KeymapWindow::_AddKeyboardLayoutMenu(BMenu* menu, BDirectory directory)
{
	entry_ref ref;

	while (directory.GetNextRef(&ref) == B_OK) {
		if (menu->FindItem(ref.name) != NULL)
			continue;

		BDirectory subdirectory;
		subdirectory.SetTo(&ref);
		if (subdirectory.InitCheck() == B_OK) {
			BMenu* submenu = new BMenu(B_TRANSLATE_NOCOLLECT(ref.name));

			_AddKeyboardLayoutMenu(submenu, subdirectory);
			menu->AddItem(submenu, (int32)0);
		} else {
			BMessage* message = new BMessage(kChangeKeyboardLayout);

			message->AddRef("ref", &ref);
			menu->AddItem(new BMenuItem(B_TRANSLATE_NOCOLLECT(ref.name),
				message), (int32)0);
		}
	}
}


/*!	Sets the keyboard layout with the passed in path and marks the
	corresponding menu item. If the path is not found in the menu this method
	sets the default keyboard layout and marks the corresponding menu item.
*/
status_t
KeymapWindow::_SetKeyboardLayout(const char* path)
{
	status_t status = fKeyboardLayoutView->GetKeyboardLayout()->Load(path);

	// mark a menu item (unmarking all others)
	_MarkKeyboardLayoutItem(path, fLayoutMenu);

	if (path == NULL || path[0] == '\0' || status != B_OK) {
		fKeyboardLayoutView->GetKeyboardLayout()->SetDefault();
		BMenuItem* item = fLayoutMenu->FindItem(
			fKeyboardLayoutView->GetKeyboardLayout()->Name());
		if (item != NULL)
			item->SetMarked(true);
	}

	// Refresh currently set layout
	fKeyboardLayoutView->SetKeyboardLayout(
		fKeyboardLayoutView->GetKeyboardLayout());

	return status;
}


/*!	Marks a keyboard layout item by iterating through the menus recursively
	searching for the menu item with the passed in path. This method always
	iterates through all menu items and unmarks them. If no item with the
	passed in path is found it is up to the caller to set the default keyboard
	layout and mark item corresponding to the default keyboard layout path.
*/
void
KeymapWindow::_MarkKeyboardLayoutItem(const char* path, BMenu* menu)
{
	BMenuItem* item = NULL;
	entry_ref ref;

	for (int32 i = 0; i < menu->CountItems(); i++) {
		item = menu->ItemAt(i);
		if (item == NULL)
			continue;

		// Unmark each item initially
		item->SetMarked(false);

		BMenu* submenu = item->Submenu();
		if (submenu != NULL)
			_MarkKeyboardLayoutItem(path, submenu);
		else {
			if (item->Message()->FindRef("ref", &ref) == B_OK) {
				BPath layoutPath(&ref);
				if (path != NULL && path[0] != '\0' && layoutPath == path) {
					// Found it, mark the item
					item->SetMarked(true);
				}
			}
		}
	}
}


/*!	Sets the label of the "Switch Shorcuts" button to make it more
	descriptive what will happen when you press that button.
*/
void
KeymapWindow::_UpdateSwitchShortcutButton()
{
	const char* label = B_TRANSLATE("Switch shortcut keys");
	if (fCurrentMap.KeyForModifier(B_LEFT_COMMAND_KEY) == 0x5d
		&& fCurrentMap.KeyForModifier(B_LEFT_CONTROL_KEY) == 0x5c) {
		label = B_TRANSLATE("Switch shortcut keys to Windows/Linux mode");
	} else if (fCurrentMap.KeyForModifier(B_LEFT_COMMAND_KEY) == 0x5c
		&& fCurrentMap.KeyForModifier(B_LEFT_CONTROL_KEY) == 0x5d) {
		label = B_TRANSLATE("Switch shortcut keys to Haiku mode");
	}

	fSwitchShortcutsButton->SetLabel(label);
}


/*!	Marks the menu items corresponding to the dead key state of the current
	key map.
*/
void
KeymapWindow::_UpdateDeadKeyMenu()
{
	BString trigger;
	fCurrentMap.GetDeadKeyTrigger(kDeadKeyAcute, trigger);
	if (!trigger.Length())
		trigger = kDeadKeyTriggerNone;
	BMenuItem* menuItem = fAcuteMenu->FindItem(trigger.String());
	if (menuItem)
		menuItem->SetMarked(true);

	fCurrentMap.GetDeadKeyTrigger(kDeadKeyCircumflex, trigger);
	if (!trigger.Length())
		trigger = kDeadKeyTriggerNone;
	menuItem = fCircumflexMenu->FindItem(trigger.String());
	if (menuItem)
		menuItem->SetMarked(true);

	fCurrentMap.GetDeadKeyTrigger(kDeadKeyDiaeresis, trigger);
	if (!trigger.Length())
		trigger = kDeadKeyTriggerNone;
	menuItem = fDiaeresisMenu->FindItem(trigger.String());
	if (menuItem)
		menuItem->SetMarked(true);

	fCurrentMap.GetDeadKeyTrigger(kDeadKeyGrave, trigger);
	if (!trigger.Length())
		trigger = kDeadKeyTriggerNone;
	menuItem = fGraveMenu->FindItem(trigger.String());
	if (menuItem)
		menuItem->SetMarked(true);

	fCurrentMap.GetDeadKeyTrigger(kDeadKeyTilde, trigger);
	if (!trigger.Length())
		trigger = kDeadKeyTriggerNone;
	menuItem = fTildeMenu->FindItem(trigger.String());
	if (menuItem)
		menuItem->SetMarked(true);
}


void
KeymapWindow::_UpdateButtons()
{
	if (fCurrentMap != fAppliedMap) {
		fCurrentMap.SetName(kCurrentKeymapName);
		_UseKeymap();
	}

	fDefaultsButton->SetEnabled(
		fCurrentMapName.ICompare(kDefaultKeymapName) != 0);
	fRevertButton->SetEnabled(fCurrentMap != fPreviousMap);

	_UpdateDeadKeyMenu();
	_UpdateSwitchShortcutButton();
}


void
KeymapWindow::_SwitchShortcutKeys()
{
	uint32 leftCommand = fCurrentMap.Map().left_command_key;
	uint32 leftControl = fCurrentMap.Map().left_control_key;
	uint32 rightCommand = fCurrentMap.Map().right_command_key;
	uint32 rightControl = fCurrentMap.Map().right_control_key;

	// switch left side
	fCurrentMap.Map().left_command_key = leftControl;
	fCurrentMap.Map().left_control_key = leftCommand;

	// switch right side
	fCurrentMap.Map().right_command_key = rightControl;
	fCurrentMap.Map().right_control_key = rightCommand;

	fKeyboardLayoutView->SetKeymap(&fCurrentMap);
	_UpdateButtons();
}


//!	Restores the default keymap.
void
KeymapWindow::_DefaultKeymap()
{
	fCurrentMap.RestoreSystemDefault();
	fAppliedMap = fCurrentMap;

	fKeyboardLayoutView->SetKeymap(&fCurrentMap);

	fCurrentMapName = _GetActiveKeymapName();
	_SelectCurrentMap();
}


//!	Saves previous map to the "Key_map" file.
void
KeymapWindow::_RevertKeymap()
{
	entry_ref ref;
	_GetCurrentKeymap(ref);

	status_t status = fPreviousMap.Save(ref);
	if (status != B_OK) {
		printf("error when saving keymap: %s", strerror(status));
		return;
	}

	fPreviousMap.Use();
	fCurrentMap.Load(ref);
	fAppliedMap = fCurrentMap;

	fKeyboardLayoutView->SetKeymap(&fCurrentMap);

	fCurrentMapName = _GetActiveKeymapName();
	_SelectCurrentMap();
}


//!	Saves current map to the "Key_map" file.
void
KeymapWindow::_UseKeymap()
{
	entry_ref ref;
	_GetCurrentKeymap(ref);

	status_t status = fCurrentMap.Save(ref);
	if (status != B_OK) {
		printf("error when saving : %s", strerror(status));
		return;
	}

	fCurrentMap.Use();
	fAppliedMap.Load(ref);

	fCurrentMapName = _GetActiveKeymapName();
	_SelectCurrentMap();
}


void
KeymapWindow::_FillSystemMaps()
{
	BListItem* item;
	while ((item = fSystemListView->RemoveItem(static_cast<int32>(0))))
		delete item;

	// TODO: common keymaps!
	BPath path;
	if (find_directory(B_SYSTEM_DATA_DIRECTORY, &path) != B_OK)
		return;

	path.Append("Keymaps");

	BDirectory directory;
	entry_ref ref;

	if (directory.SetTo(path.Path()) == B_OK) {
		while (directory.GetNextRef(&ref) == B_OK) {
			fSystemListView->AddItem(
				new KeymapListItem(ref, B_TRANSLATE_NOCOLLECT(ref.name)));
		}
	}

	fSystemListView->SortItems(&compare_key_list_items);
}


void
KeymapWindow::_FillUserMaps()
{
	BListItem* item;
	while ((item = fUserListView->RemoveItem(static_cast<int32>(0))))
		delete item;

	entry_ref ref;
	_GetCurrentKeymap(ref);

	fUserListView->AddItem(new KeymapListItem(ref, B_TRANSLATE("(Current)")));

	fCurrentMapName = _GetActiveKeymapName();

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append("Keymap");

	BDirectory directory;
	if (directory.SetTo(path.Path()) == B_OK) {
		while (directory.GetNextRef(&ref) == B_OK) {
			fUserListView->AddItem(new KeymapListItem(ref));
		}
	}

	fUserListView->SortItems(&compare_key_list_items);
}


void
KeymapWindow::_SetListViewSize(BListView* listView)
{
	float minWidth = 0;
	for (int32 i = 0; i < listView->CountItems(); i++) {
		BStringItem* item = (BStringItem*)listView->ItemAt(i);
		float width = listView->StringWidth(item->Text());
		if (width > minWidth)
			minWidth = width;
	}

	listView->SetExplicitMinSize(BSize(minWidth + 8, 32));
}


status_t
KeymapWindow::_GetCurrentKeymap(entry_ref& ref)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return B_ERROR;

	path.Append("Key_map");

	return get_ref_for_path(path.Path(), &ref);
}


BString
KeymapWindow::_GetActiveKeymapName()
{
	BString mapName = kCurrentKeymapName;
		// safe default

	entry_ref ref;
	_GetCurrentKeymap(ref);

	BNode node(&ref);

	if (node.InitCheck() == B_OK)
		node.ReadAttrString("keymap:name", &mapName);

	return mapName;
}


bool
KeymapWindow::_SelectCurrentMap(BListView* view)
{
	if (fCurrentMapName.Length() <= 0)
		return false;

	for (int32 i = 0; i < view->CountItems(); i++) {
		BStringItem* current = dynamic_cast<BStringItem *>(view->ItemAt(i));
		if (current != NULL && fCurrentMapName == current->Text()) {
			view->Select(i);
			view->ScrollToSelection();
			return true;
		}
	}

	return false;
}


void
KeymapWindow::_SelectCurrentMap()
{
	if (!_SelectCurrentMap(fSystemListView)
		&& !_SelectCurrentMap(fUserListView)) {
		// Select the "(Current)" entry if no name matches
		fUserListView->Select(0L);
	}
}


status_t
KeymapWindow::_GetSettings(BFile& file, int mode) const
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path,
		(mode & O_ACCMODE) != O_RDONLY);
	if (status != B_OK)
		return status;

	path.Append("Keymap settings");

	return file.SetTo(path.Path(), mode);
}


status_t
KeymapWindow::_LoadSettings(BRect& windowFrame)
{
	BScreen screen(this);

	windowFrame.Set(-1, -1, 669, 357);
	// See if we can use a larger default size
	if (screen.Frame().Width() > 1200) {
		windowFrame.right = 899;
		windowFrame.bottom = 349;
	}
	float scaling = be_plain_font->Size() / 12.0f;
	windowFrame.right *= scaling;
	windowFrame.bottom *= scaling;

	BFile file;
	status_t status = _GetSettings(file, B_READ_ONLY);
	if (status == B_OK) {
		BMessage settings;
		status = settings.Unflatten(&file);
		if (status == B_OK) {
			BRect frame;
			status = settings.FindRect("window frame", &frame);
			if (status == B_OK)
				windowFrame = frame;

			const char* layoutPath;
			if (settings.FindString("keyboard layout", &layoutPath) == B_OK)
				_SetKeyboardLayout(layoutPath);
		}
	}

	return status;
}


status_t
KeymapWindow::_SaveSettings()
{
	BFile file;
	status_t status
		= _GetSettings(file, B_WRITE_ONLY | B_ERASE_FILE | B_CREATE_FILE);
	if (status != B_OK)
		return status;

	BMessage settings('keym');
	settings.AddRect("window frame", Frame());

	BPath path = _GetMarkedKeyboardLayoutPath(fLayoutMenu);
	if (path.InitCheck() == B_OK)
		settings.AddString("keyboard layout", path.Path());

	return settings.Flatten(&file);
}


/*!	Gets the path of the currently marked keyboard layout item
	by searching through each of the menus recursively until
	a marked item is found.
*/
BPath
KeymapWindow::_GetMarkedKeyboardLayoutPath(BMenu* menu)
{
	BPath path;
	BMenuItem* item = NULL;
	entry_ref ref;

	for (int32 i = 0; i < menu->CountItems(); i++) {
		item = menu->ItemAt(i);
		if (item == NULL)
			continue;

		BMenu* submenu = item->Submenu();
		if (submenu != NULL) {
			path = _GetMarkedKeyboardLayoutPath(submenu);
			if (path.InitCheck() == B_OK)
				return path;
		} else {
			if (item->IsMarked()
				&& item->Message()->FindRef("ref", &ref) == B_OK) {
				path.SetTo(&ref);
				return path;
			}
		}
	}

	return path;
}
