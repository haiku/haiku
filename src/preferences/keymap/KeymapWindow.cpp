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

static const uint32 kMsgMenuEditUndo = 'mMEU';

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
				.Add(fTextControl)
				.AddGlue(0.0)
				.Add(BGroupLayoutBuilder(B_HORIZONTAL, 10)
					.AddGlue(0.0)
					.Add(fUseButton)
					.Add(fRevertButton)))
			.SetInsets(10, 10, 10, 10)));

	fKeyboardLayoutView->SetTarget(fTextControl->TextView());
	fTextControl->MakeFocus();
	AddCommonFilter(new KeymapMessageFilter(B_PROGRAMMED_DELIVERY, B_ANY_SOURCE,
		&fCurrentMap));
// TODO: this does not work for some reason, investigate!
//	fTextControl->AddFilter(fTextFilter);

	_UpdateButtons();

	fCurrentMap.SetTarget(this, new BMessage(kMsgKeymapUpdated));

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
	// was read at all) - this will also load the fCurrentMap
	if (!_SelectCurrentMap(fSystemListView)
		&& !_SelectCurrentMap(fUserListView))
		fUserListView->Select(0L);

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

#if 0
		case kMsgMenuEditUndo:
		case kMsgMenuEditCut:
		case kMsgMenuEditCopy:
		case kMsgMenuEditPaste:
		case kMsgMenuEditClear:
		case kMsgMenuEditSelectAll:
			fMapView->MessageReceived(message);
			break;
#endif
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
		{
			KeymapListItem *keymapListItem = 
				static_cast<KeymapListItem*>(fSystemListView->ItemAt(
					fSystemListView->CurrentSelection()));
			if (keymapListItem) {
				fCurrentMap.Load(keymapListItem->KeymapEntry());

				if (fFirstTime) {
					fPreviousMap.Load(keymapListItem->KeymapEntry());
					fAppliedMap.Load(keymapListItem->KeymapEntry());
					fFirstTime = false;
				}

				fKeyboardLayoutView->SetKeymap(&fCurrentMap);

				// Deselect item in other BListView
				fUserListView->DeselectAll();
				_UpdateButtons();
			}
			break;
		}

		case kMsgUserMapSelected:
		{
			int32 index = fUserListView->CurrentSelection();
			if (index == 0) {
				// we can safely ignore the "(Current)" item
				break;
			}

			KeymapListItem *keymapListItem = 
				static_cast<KeymapListItem*>(fUserListView->ItemAt(index));
			if (keymapListItem != NULL) {
				fCurrentMap.Load(keymapListItem->KeymapEntry());

				if (fFirstTime) {
					fPreviousMap.Load(keymapListItem->KeymapEntry());
					fAppliedMap.Load(keymapListItem->KeymapEntry());
					fFirstTime = false;
				}

				fKeyboardLayoutView->SetKeymap(&fCurrentMap);

				// Deselect item in other BListView
				fSystemListView->DeselectAll();
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
	menu = new BMenu("Layout");
	menu->AddItem(item = new BMenuItem(
		fKeyboardLayoutView->GetKeyboardLayout()->Name(), NULL));
	item->SetMarked(true);
	menuBar->AddItem(menu);
#if 0	
	// Create the Edit menu
	menu = new BMenu("Edit");
	currentItem = new BMenuItem("Undo",
		new BMessage(kMsgMenuEditUndo), 'Z');
	currentItem->SetEnabled(false);
	menu->AddItem(currentItem);
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem( "Cut",
		new BMessage(kMsgMenuEditCut), 'X'));
	menu->AddItem(new BMenuItem( "Copy",
		new BMessage(kMsgMenuEditCopy), 'C'));
	menu->AddItem(new BMenuItem( "Paste",
		new BMessage(kMsgMenuEditPaste), 'V'));
	menu->AddItem(new BMenuItem( "Clear",
		new BMessage(kMsgMenuEditClear)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem( "Select All",
		new BMessage(kMsgMenuEditSelectAll), 'A'));
	menuBar->AddItem(menu);
#endif

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

	// '(Current)'
	KeymapListItem* currentKeymapItem
		= static_cast<KeymapListItem*>(fUserListView->FirstItem());
	if (currentKeymapItem != NULL)
		fUserListView->AddItem(currentKeymapItem);

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
KeymapWindow::_UpdateButtons()
{
	fUseButton->SetEnabled(!fCurrentMap.Equals(fAppliedMap));
	fRevertButton->SetEnabled(!fCurrentMap.Equals(fPreviousMap));
}


//!	Saves previous map to the "Key_map" file.
void 
KeymapWindow::_RevertKeymap()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append("Key_map");

	entry_ref ref;
	get_ref_for_path(path.Path(), &ref);

	status_t status = fPreviousMap.Save(ref);
	if (status != B_OK) {
		printf("error when saving keymap: %s", strerror(status));
		return;
	}

	fPreviousMap.Use();
	fAppliedMap.Load(ref);

	// TODO: add = operator
	fCurrentMap.Load(ref);
	fKeyboardLayoutView->SetKeymap(&fCurrentMap);

	fCurrentMapName = _GetActiveKeymapName();

	if (!_SelectCurrentMap(fSystemListView)
		&& !_SelectCurrentMap(fUserListView))
		fUserListView->Select(0L);
}


void 
KeymapWindow::_UseKeymap()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append("Key_map");

	entry_ref ref;
	get_ref_for_path(path.Path(), &ref);

	status_t err;
	if ((err = fCurrentMap.Save(ref)) != B_OK) {
		printf("error when saving : %s", strerror(err));
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
	BListItem *item;
	while ((item = fUserListView->RemoveItem(static_cast<int32>(0))))
		delete item;

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;

	path.Append("Key_map");

	entry_ref ref;
	get_ref_for_path(path.Path(), &ref);

	fUserListView->AddItem(new KeymapListItem(ref, "(Current)"));

	fCurrentMapName = _GetActiveKeymapName();

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


BString
KeymapWindow::_GetActiveKeymapName()
{
	BString mapName = "(Current)";	// safe default

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return mapName;

	path.Append("Key_map");

	entry_ref ref;
	get_ref_for_path(path.Path(), &ref);
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
