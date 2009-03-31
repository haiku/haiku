/*
 * Copyright 2004-2009 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Sandor Vroemisse
 *		Jérôme Duval
 *		Alexandre Deckner, alex@zappotek.com
 *		Axel Dörfler, axeld@pinc-software.de.
 */

#include "KeymapWindow.h"

#include <string.h>

#include <Alert.h>
#include <Button.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <GroupLayoutBuilder.h>
#include <ListView.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Path.h>
#include <Screen.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextControl.h>

#include "KeyboardLayoutView.h"
#include "KeymapApplication.h"
#include "KeymapListItem.h"
#include "KeymapMessageFilter.h"


static const uint32 kMsgMenuFileOpen = 'mMFO';
static const uint32 kMsgMenuFileSave = 'mMFS';
static const uint32 kMsgMenuFileSaveAs = 'mMFA';

static const uint32 kChangeKeyboardLayout = 'cKyL';

static const uint32 kMsgSwitchShortcuts = 'swSc';

static const uint32 kMsgMenuFontChanged = 'mMFC';

static const uint32 kMsgSystemMapSelected = 'SmST';
static const uint32 kMsgUserMapSelected = 'UmST';

static const uint32 kMsgUseKeymap = 'UkyM';
static const uint32 kMsgRevertKeymap = 'Rvrt';
static const uint32 kMsgKeymapUpdated = 'upkM';


KeymapWindow::KeymapWindow()
	: BWindow(BRect(80, 50, 880, 380), "Keymap", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fFirstTime(true)
{
	SetLayout(new BGroupLayout(B_VERTICAL));

	fKeyboardLayoutView = new KeyboardLayoutView("layout");
	fKeyboardLayoutView->SetKeymap(&fCurrentMap);

	fTextControl = new BTextControl("Sample and Clipboard:", "", NULL);

	fSwitchShortcutsButton = new BButton("switch", "",
		new BMessage(kMsgSwitchShortcuts));

	fUseButton = new BButton("useButton", "Use", new BMessage(kMsgUseKeymap));
	fRevertButton = new BButton("revertButton", "Revert",
		new BMessage(kMsgRevertKeymap));

	// controls pane
	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(_CreateMenu())
		.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10)
			.Add(_CreateMapLists(), 0.25)
			.Add(BGroupLayoutBuilder(B_VERTICAL, 10)
				.Add(fKeyboardLayoutView)
				//.Add(new BStringView("text label", "Sample and Clipboard:"))
				.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10)
					.Add(fTextControl)
					.Add(fSwitchShortcutsButton))
				.AddGlue(0.0)
				.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10)
					.AddGlue(0.0)
					.Add(fUseButton)
					.Add(fRevertButton)))
			.SetInsets(10, 10, 10, 10)));

	fKeyboardLayoutView->SetTarget(fTextControl->TextView());
	fTextControl->MakeFocus();
	fTextControl->TextView()->AddFilter(new KeymapMessageFilter(
		B_PROGRAMMED_DELIVERY, B_ANY_SOURCE, &fCurrentMap));

	_UpdateButtons();

	// Make sure the user keymap directory exists
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append("Keymap");

	entry_ref ref;
	get_ref_for_path(path.Path(), &ref);

	BDirectory userKeymapsDir(&ref);
	if (userKeymapsDir.InitCheck() != B_OK) {
		create_directory(path.Path(), S_IRWXU | S_IRWXG | S_IRWXO);
	}

	BMessenger messenger(this);	
	fOpenPanel = new BFilePanel(B_OPEN_PANEL, &messenger, &ref, 
		B_FILE_NODE, false, NULL);
	fSavePanel = new BFilePanel(B_SAVE_PANEL, &messenger, &ref, 
		B_FILE_NODE, false, NULL);

	BScreen screen(this);

	float width = Frame().Width();
	float height = Frame().Height();

	// Make sure we can fit on screen
	if (screen.Frame().Width() < Frame().Width())
		width = screen.Frame().Width();
	if (screen.Frame().Height() < Frame().Height())
		height = screen.Frame().Height();

	// See if we can use a larger default size
	if (screen.Frame().Width() > 1200) {
		width = 900;
		height = 400;
	}

	// TODO: store and restore position and size!
	ResizeTo(width, height);
	MoveTo(BAlert::AlertPosition(width, height));

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

	_UpdateShortcutButton();

	Unlock();
}


KeymapWindow::~KeymapWindow(void)
{
	delete fOpenPanel;
	delete fSavePanel;
}


bool 
KeymapWindow::QuitRequested()
{
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
			}
			fKeyboardLayoutView->SetKeymap(&fCurrentMap);
			fSystemListView->DeselectAll();
			fUserListView->DeselectAll();
			break;
		}

		case B_MIME_DATA:
			break;

		case B_SAVE_REQUESTED:
		{
			entry_ref ref;
			const char *name;
			if (message->FindRef("directory", &ref) == B_OK
				&& message->FindString("name", &name) == B_OK) {
				BDirectory directory(&ref);
				BEntry entry(&directory, name);
				entry.GetRef(&ref);
				fCurrentMap.Save(ref);

				_FillUserMaps();
			}
			break;
		}

		case kMsgMenuFileOpen:
			fOpenPanel->Show();
			break;
		case kMsgMenuFileSave:
			break;
		case kMsgMenuFileSaveAs:
			fSavePanel->Show();
			break;

		case kChangeKeyboardLayout:
		{
			entry_ref ref;
			if (message->FindRef("ref", &ref) == B_OK
				&& fKeyboardLayoutView->GetKeyboardLayout()->Load(ref)
						== B_OK) {
				fKeyboardLayoutView->SetKeyboardLayout(
					fKeyboardLayoutView->GetKeyboardLayout());
			} else {
				fKeyboardLayoutView->GetKeyboardLayout()->SetDefault();
				fLayoutMenu->ItemAt(0)->SetMarked(true);
			}

			fKeyboardLayoutView->SetKeyboardLayout(
				fKeyboardLayoutView->GetKeyboardLayout());
			break;
		}

		case kMsgSwitchShortcuts:
			_SwitchShortcutKeys();
			break;

		case kMsgMenuFontChanged:
		{
			BMenuItem *item = fFontMenu->FindMarked();
			if (item != NULL) {
				BFont font;
				font.SetFamilyAndStyle(item->Label(), NULL);
				fKeyboardLayoutView->SetFont(font);
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
				fUserListView->DeselectAll();
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
				if (!fFirstTime)
					fCurrentMap.Load(item->EntryRef());
				else
					fFirstTime = false;

				fKeyboardLayoutView->SetKeymap(&fCurrentMap);
				_UpdateButtons();
			}
			break;
		}

		case kMsgUseKeymap:
			_UseKeymap();
			_UpdateButtons();
			break;
		case kMsgRevertKeymap:
			_RevertKeymap();
			_UpdateButtons();
			break;

		case kMsgKeymapUpdated:
			_UpdateButtons();
			fSystemListView->DeselectAll();
			fUserListView->Select(0L);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


BMenuBar*
KeymapWindow::_CreateMenu()
{
	BMenuBar* menuBar = new BMenuBar(Bounds(), "menubar");
	BMenuItem* item;

	// Create the File menu
	BMenu* menu = new BMenu("File");
	menu->AddItem(new BMenuItem("Open" B_UTF8_ELLIPSIS,
		new BMessage(kMsgMenuFileOpen), 'O'));
	menu->AddSeparatorItem();
	item = new BMenuItem("Save", new BMessage(kMsgMenuFileSave), 'S');
	item->SetEnabled(false);
	menu->AddItem(item);
	menu->AddItem(new BMenuItem("Save As" B_UTF8_ELLIPSIS,
		new BMessage(kMsgMenuFileSaveAs)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Quit",
		new BMessage(B_QUIT_REQUESTED), 'Q'));
	menuBar->AddItem(menu);

	// Create keyboard layout menu
	fLayoutMenu = new BMenu("Layout");
	fLayoutMenu->SetRadioMode(true);
	fLayoutMenu->AddItem(item = new BMenuItem(
		fKeyboardLayoutView->GetKeyboardLayout()->Name(),
		new BMessage(kChangeKeyboardLayout)));
	item->SetMarked(true);

	_AddKeyboardLayouts(fLayoutMenu);
	menuBar->AddItem(fLayoutMenu);

	// Create the Font menu
	fFontMenu = new BMenu("Font");
	fFontMenu->SetRadioMode(true);
	int32 numFamilies = count_font_families();
	font_family family, currentFamily;
	font_style currentStyle;
	uint32 flags;

	be_plain_font->GetFamilyAndStyle(&currentFamily, &currentStyle);

	for (int32 i = 0; i < numFamilies; i++) {
		if (get_font_family(i, &family, &flags) == B_OK) {
			BMenuItem *item = 
				new BMenuItem(family, new BMessage(kMsgMenuFontChanged));
			fFontMenu->AddItem(item);

			if (!strcmp(family, currentFamily))
				item->SetMarked(true);
		}
	}
	menuBar->AddItem(fFontMenu);

	return menuBar;
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

	return BGroupLayoutBuilder(B_VERTICAL)
		.Add(new BStringView("system", "System:"))
		.Add(systemScroller, 3)
		.Add(new BStringView("user", "User:"))
		.Add(userScroller);
}


void
KeymapWindow::_AddKeyboardLayouts(BMenu* menu)
{
	directory_which dataDirectories[] = {
		B_USER_DATA_DIRECTORY,
		B_COMMON_DATA_DIRECTORY,
		B_BEOS_DATA_DIRECTORY
	};

	for (uint32 i = 0;
			i < sizeof(dataDirectories) / sizeof(dataDirectories[0]); i++) {
		BPath path;
		if (find_directory(dataDirectories[i], &path) != B_OK)
			continue;

		path.Append("KeyboardLayouts");

		BDirectory directory;		
		if (directory.SetTo(path.Path()) == B_OK) {
			entry_ref ref;
			while (directory.GetNextRef(&ref) == B_OK) {
				if (menu->FindItem(ref.name) != NULL)
					continue;

				BMessage* message = new BMessage(kChangeKeyboardLayout);
				message->AddRef("ref", &ref);

				menu->AddItem(new BMenuItem(ref.name, message));
			}
		}
	}
}


void
KeymapWindow::_UpdateShortcutButton()
{
	const char* label = "Switch Shortcut Keys";
	if (fCurrentMap.KeyForModifier(B_LEFT_COMMAND_KEY) == 0x5d
		&& fCurrentMap.KeyForModifier(B_LEFT_CONTROL_KEY) == 0x5c
		&& fCurrentMap.KeyForModifier(B_RIGHT_OPTION_KEY) == 0x5f
		&& fCurrentMap.KeyForModifier(B_RIGHT_CONTROL_KEY) == 0x60) {
		label = "Switch Shortcut Keys To Windows/Linux Mode";
	} else if (fCurrentMap.KeyForModifier(B_LEFT_COMMAND_KEY) == 0x5c
		&& fCurrentMap.KeyForModifier(B_LEFT_CONTROL_KEY) == 0x5d
		&& fCurrentMap.KeyForModifier(B_RIGHT_OPTION_KEY) == 0x60
		&& fCurrentMap.KeyForModifier(B_RIGHT_CONTROL_KEY) == 0x5f) {
		label = "Switch Shortcut Keys To Haiku Mode";
	}

	fSwitchShortcutsButton->SetLabel(label);
}


void
KeymapWindow::_UpdateButtons()
{
	fUseButton->SetEnabled(!fCurrentMap.Equals(fAppliedMap));
	fRevertButton->SetEnabled(!fCurrentMap.Equals(fPreviousMap));

	_UpdateShortcutButton();
}


void
KeymapWindow::_SwitchShortcutKeys()
{
	uint32 leftCommand = fCurrentMap.Map().left_command_key;
	uint32 leftControl = fCurrentMap.Map().left_control_key;
	uint32 rightOption = fCurrentMap.Map().right_option_key;
	uint32 rightControl = fCurrentMap.Map().right_control_key;

	// switch left side
	fCurrentMap.Map().left_command_key = leftControl;
	fCurrentMap.Map().left_control_key = leftCommand;

	// switch right side
	fCurrentMap.Map().right_option_key = rightControl;
	fCurrentMap.Map().right_control_key = rightOption;

	fKeyboardLayoutView->SetKeymap(&fCurrentMap);
	_UpdateButtons();
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
}


void
KeymapWindow::_FillSystemMaps()
{
	BListItem *item;
	while ((item = fSystemListView->RemoveItem(static_cast<int32>(0))))
		delete item;

	BPath path;
	if (find_directory(B_BEOS_ETC_DIRECTORY, &path) != B_OK)
		return;
	
	path.Append("Keymap");
	
	BDirectory directory;
	entry_ref ref;
	
	if (directory.SetTo(path.Path()) == B_OK) {
		while (directory.GetNextRef(&ref) == B_OK) {
			fSystemListView->AddItem(new KeymapListItem(ref));
		}
	}
}


void 
KeymapWindow::_FillUserMaps()
{
	BListItem* item;
	while ((item = fUserListView->RemoveItem(static_cast<int32>(0))))
		delete item;

	entry_ref ref;
	_GetCurrentKeymap(ref);

	fUserListView->AddItem(new KeymapListItem(ref, "(Current)"));

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
	BString mapName = "(Current)";	// safe default

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
		fFirstTime = false;
	}
}
