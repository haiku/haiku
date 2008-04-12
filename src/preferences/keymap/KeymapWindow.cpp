/*
 * Copyright 2004-2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Sandor Vroemisse
 *		Jérôme Duval
 *		Alexandre Deckner, alex@zappotek.com
 */

#include <Alert.h>
#include <Application.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <Clipboard.h>
#include <Debug.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <GraphicsDefs.h>
#include <ListView.h>
#include <MenuItem.h>
#include <Path.h>
#include <Region.h>
#include <ScrollView.h>
#include <StringView.h>
#include <TextView.h>
#include <View.h>
#include <string.h>
#include "KeymapWindow.h"
#include "KeymapListItem.h"
#include "KeymapApplication.h"

static const uint32 kMsgMenuFileOpen = 'mMFO';
static const uint32 kMsgMenuFileSave = 'mMFS';
static const uint32 kMsgMenuFileSaveAs = 'mMFA';
static const uint32 kMsgMenuEditUndo = 'mMEU';
static const uint32 kMsgMenuEditCut = 'mMEX';
static const uint32 kMsgMenuEditCopy = 'mMEC';
static const uint32 kMsgMenuEditPaste = 'mMEV';
static const uint32 kMsgMenuEditClear = 'mMEL';
static const uint32 kMsgMenuEditSelectAll = 'mMEA';
static const uint32 kMsgMenuFontChanged = 'mMFC';
static const uint32 kMsgSystemMapSelected = 'SmST';
static const uint32 kMsgUserMapSelected = 'UmST';
static const uint32 kMsgUseKeymap = 'UkyM';
static const uint32 kMsgRevertKeymap = 'Rvrt';

KeymapWindow::KeymapWindow()
	:	BWindow(BRect(80, 25, 692, 281), "Keymap", B_TITLED_WINDOW,
			B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS )
{
	fFirstTime = true;
	
	// Add the menu bar
	BMenuBar *menubar = AddMenuBar();

	// The view to hold all but the menu bar
	BRect bounds = Bounds();
	bounds.top = menubar->Bounds().bottom + 1;
	BBox *placeholderView = new BBox(bounds, "placeholderView", 
		B_FOLLOW_ALL);
	placeholderView->SetBorder(B_NO_BORDER);
	AddChild(placeholderView);
	
	// Create the Maps box and contents
	AddMaps(placeholderView);
	
	fMapView = new MapView(BRect(150, 9, 600, 189), "mapView", &fCurrentMap);
	placeholderView->AddChild(fMapView);
	
	BMenuItem *item = fFontMenu->FindMarked();
	if (item) {
		fMapView->SetFontFamily(item->Label());
	}
	
	// The 'Use' button
	fUseButton = new BButton(BRect(527, 200, 600, 220), "useButton", "Use",
		new BMessage(kMsgUseKeymap));
	placeholderView->AddChild(fUseButton);
	
	// The 'Revert' button
	fRevertButton = new BButton(BRect(442, 200, 515, 220), "revertButton",
		 "Revert", new BMessage(kMsgRevertKeymap));
	placeholderView->AddChild(fRevertButton);
	UpdateButtons();
	
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append("Keymap");
	
	entry_ref ref;
	get_ref_for_path(path.Path(), &ref);
	
	BDirectory userKeymapsDir(&ref);
	if (userKeymapsDir.InitCheck() != B_OK) {
		create_directory(path.Path(), S_IRWXU | S_IRWXG | S_IRWXO);
	}
	
	fOpenPanel = new BFilePanel(B_OPEN_PANEL, new BMessenger(this), &ref, 
		B_FILE_NODE, false, NULL);
	fSavePanel = new BFilePanel(B_SAVE_PANEL, new BMessenger(this), &ref, 
		B_FILE_NODE, false, NULL);
}


KeymapWindow::~KeymapWindow(void)
{
	delete fOpenPanel;
	delete fSavePanel;
}


BMenuBar *
KeymapWindow::AddMenuBar()
{
	BRect		bounds;
	BMenu		*menu;
	BMenuItem	*currentItem;
	BMenuBar	*menubar;

	bounds = Bounds();
	menubar = new BMenuBar(bounds, "menubar");
	AddChild(menubar);
	
	// Create the File menu
	menu = new BMenu("File");
	menu->AddItem(new BMenuItem("Open" B_UTF8_ELLIPSIS,
		new BMessage(kMsgMenuFileOpen), 'O'));
	menu->AddSeparatorItem();
	currentItem = new BMenuItem("Save",
		new BMessage(kMsgMenuFileSave), 'S');
	currentItem->SetEnabled(false);
	menu->AddItem(currentItem);
	menu->AddItem(new BMenuItem("Save As" B_UTF8_ELLIPSIS,
		new BMessage(kMsgMenuFileSaveAs)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem("Quit",
		new BMessage(B_QUIT_REQUESTED), 'Q'));
	menubar->AddItem(menu);

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
	menubar->AddItem(menu);
	
	// Create the Font menu
	fFontMenu = new BMenu("Font");
	fFontMenu->SetRadioMode(true);
	int32 numFamilies = count_font_families(); 
	font_family family, current_family;
	font_style current_style; 
	uint32 flags;
	
	be_plain_font->GetFamilyAndStyle(&current_family, &current_style);
	
	for (int32 i = 0; i < numFamilies; i++ )
		if (get_font_family(i, &family, &flags) == B_OK) {
			BMenuItem *item = 
				new BMenuItem(family, new BMessage(kMsgMenuFontChanged));
			fFontMenu->AddItem(item);
			if (strcmp(family, current_family) == 0)
				item->SetMarked(true);
		}
	menubar->AddItem(fFontMenu);
	
	return menubar;
}


void 
KeymapWindow::AddMaps(BView *placeholderView)
{
	// The Maps box
	BRect bounds = BRect(9, 11, 140, 226);
	BBox *mapsBox = new BBox(bounds);
	mapsBox->SetLabel("Maps");
	placeholderView->AddChild(mapsBox);

	// The System list
	BStringView *systemLabel = new BStringView(BRect(13, 13, 113, 33), "system", "System");
	mapsBox->AddChild(systemLabel);
	
	bounds = BRect(13, 35, 103, 105);
	fSystemListView = new BListView(bounds, "systemList");
	
	mapsBox->AddChild(new BScrollView("systemScrollList", fSystemListView,
		B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true));
	fSystemListView->SetSelectionMessage(new BMessage(kMsgSystemMapSelected));

	// The User list
	BStringView *userLabel = new BStringView(BRect(13, 110, 113, 128), "user", "User");
	mapsBox->AddChild(userLabel);

	bounds = BRect(13, 130, 103, 200);
	fUserListView = new BListView(bounds, "userList");
	// '(Current)'
	KeymapListItem *currentKeymapItem = static_cast<KeymapListItem*>(fUserListView->FirstItem());
	if (currentKeymapItem != NULL)
		fUserListView->AddItem(currentKeymapItem);
	// Saved keymaps
	mapsBox->AddChild(new BScrollView("userScrollList", fUserListView,
		B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true));
	fUserListView->SetSelectionMessage(new BMessage(kMsgUserMapSelected));
	
	FillSystemMaps();
	FillUserMaps();

	// try and find the current map name in the two list views (if the name was read at all)
	if (!SelectCurrentMap(fSystemListView))
		if (!SelectCurrentMap(fUserListView))
			fUserListView->Select(0L);
}


bool 
KeymapWindow::QuitRequested()
{
	if (!IsActive())
		return false;
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void 
KeymapWindow::MessageReceived(BMessage* message)
{
	switch(message->what) {
		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED: 
		{
			entry_ref ref;
			int32 i = 0;
			while (message->FindRef("refs", i++, &ref) == B_OK) {
				fCurrentMap.Load(ref);
			}
			fMapView->Invalidate();
			fSystemListView->DeselectAll();
			fUserListView->DeselectAll();
		}
			break;
		case B_SAVE_REQUESTED:
		{
			entry_ref ref;
			const char *name;
			if ((message->FindRef("directory", &ref) == B_OK)
				&& (message->FindString("name", &name) == B_OK)) {
				
				BDirectory directory(&ref);
				BEntry entry(&directory, name);
				entry.GetRef(&ref);
				fCurrentMap.Save(ref);
				
				FillUserMaps();
			}
		}
			break;
		case kMsgMenuFileOpen:
			fOpenPanel->Show();
			break;
		case kMsgMenuFileSave:
			break;
		case kMsgMenuFileSaveAs:
			fSavePanel->Show();
			break;
		case kMsgMenuEditUndo:
		case kMsgMenuEditCut:
		case kMsgMenuEditCopy:
		case kMsgMenuEditPaste:
		case kMsgMenuEditClear:
		case kMsgMenuEditSelectAll:
			fMapView->MessageReceived(message);
			break;
		case kMsgMenuFontChanged:
		{
			BMenuItem *item = fFontMenu->FindMarked();
			if (item) {
				fMapView->SetFontFamily(item->Label());
				fMapView->Invalidate();
			}
		}
			break;
		case kMsgSystemMapSelected:
		{
			KeymapListItem *keymapListItem = 
				static_cast<KeymapListItem*>(fSystemListView->ItemAt(fSystemListView->CurrentSelection()));
			if (keymapListItem) {
				fCurrentMap.Load(keymapListItem->KeymapEntry());
				fMapView->Invalidate();
				
				// Deselect item in other BListView
				fUserListView->DeselectAll();
				UpdateButtons();
			}
		}
			break;
		case kMsgUserMapSelected:
		{
			KeymapListItem *keymapListItem = 
				static_cast<KeymapListItem*>(fUserListView->ItemAt(fUserListView->CurrentSelection()));
			if (keymapListItem) {
				fCurrentMap.Load(keymapListItem->KeymapEntry());
				
				if (fFirstTime) {
					fPreviousMap.Load(keymapListItem->KeymapEntry());
					fAppliedMap.Load(keymapListItem->KeymapEntry());
					fFirstTime = false;
				}
				
				fMapView->Invalidate();
				
				// Deselect item in other BListView
				fSystemListView->DeselectAll();
				UpdateButtons();
			}
		}
			break;
		case kMsgUseKeymap:
			UseKeymap();
			UpdateButtons();
			break;
		case kMsgRevertKeymap:
			RevertKeymap();
			UpdateButtons();
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


 void 
KeymapWindow::UpdateButtons()
{
	fUseButton->SetEnabled(!fCurrentMap.Equals(fAppliedMap));
	fRevertButton->SetEnabled(!fCurrentMap.Equals(fPreviousMap));
}


void 
KeymapWindow::RevertKeymap()
{
	//saves previous map to the Key_map file
	
	printf("REVERT\n");
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path)!=B_OK)
		return;
	
	path.Append("Key_map");

	entry_ref ref;
	get_ref_for_path(path.Path(), &ref);

	status_t err;
	if ((err = fPreviousMap.Save(ref)) != B_OK) {
		printf("error when saving : %s", strerror(err));
		return;
	}
	fPreviousMap.Use();
	fAppliedMap.Load(ref);
	
	//select and load it (first item in fUserListView is a ref to Key_map)
	fUserListView->DeselectAll();
	fUserListView->Select(0);		
}


void 
KeymapWindow::UseKeymap()
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
	
	fUserListView->Select(0);
}


void
KeymapWindow::FillSystemMaps()
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
	
	if (directory.SetTo(path.Path()) == B_OK)
		while( directory.GetNextRef(&ref) == B_OK ) {
			fSystemListView->AddItem(new KeymapListItem(ref));
		}
}


void 
KeymapWindow::FillUserMaps()
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

	BNode node(&ref);
	char name[B_FILE_NAME_LENGTH];
	name[0] = '\0';
	if (node.InitCheck() == B_OK) {
		ssize_t readSize = node.ReadAttr("keymap:name", B_STRING_TYPE, 0, name, sizeof(name) - 1);
		if (readSize > 0) {
			name[readSize] = '\0';
			fCurrentMapName = name;
		}
	}	

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK)
		return;
	
	path.Append("Keymap");
	
	BDirectory directory;
	
	if (directory.SetTo(path.Path()) == B_OK)
		while( directory.GetNextRef(&ref) == B_OK ) {
			fUserListView->AddItem(new KeymapListItem(ref));
		}
}


bool
KeymapWindow::SelectCurrentMap(BListView *view)
{
	bool found = false;
	if (fCurrentMapName.Length() > 0) {
		for (int32 i = 0; i < view->CountItems(); i++) {
			BStringItem *current = dynamic_cast<BStringItem *>(view->ItemAt(i));
			if (current && (fCurrentMapName == current->Text())) {
				found = true;
				view->Select(i);
				view->ScrollToSelection();
				break;
			}
		}
	}	
	return found;
}


MapView::MapView(BRect rect, const char *name, Keymap* keymap)
	: BControl(rect, name, NULL, NULL, B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW),
		fCurrentFont(*be_plain_font),
		fCurrentMap(keymap),
		fCurrentMouseKey(0)
{
	// TODO: Properly handle font sensitivity in drawing the keys.
	// This at least prevents the app from looking horrible until the font sensitivity for this app
	// can be done the Right Way.
	if (fCurrentFont.Size() > 14)
		fCurrentFont.SetSize(14);
		
	BRect frameRect = BRect(14, 16, Bounds().right - 12, 30);
	BRect textRect = frameRect;
	textRect.OffsetTo(B_ORIGIN);
	textRect.InsetBy(1, 1);
	fTextView = new KeymapTextView(frameRect, "testzone", textRect,
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS);
	fTextView->MakeEditable(true);
	fTextView->MakeSelectable(true);
	
	AddChild(fTextView);
		
	fBitmap = new BBitmap(Bounds(), B_RGB32, true, false);			
	fOffscreenView = new BView(Bounds(), "off_view", 0, 0);

	fBitmap->Lock();
	fBitmap->AddChild(fOffscreenView);
	fBitmap->Unlock();	
	
	for (uint32 j = 0; j < 128; j++)
		fKeysToDraw[j] = false;
	
	BRect keyRect = BRect(11, 50, 29, 68);
	uint32 i = 1;
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	
	// Fx keys
	i++;
	keyRect.OffsetBySelf(36, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	
	i++;
	keyRect.OffsetBySelf(27, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	
	i++;
	keyRect.OffsetBySelf(27, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	
	// Pause, PrintScreen, ...
	i++;
	keyRect.OffsetBySelf(35, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	
	// 1st line : numbers and backspace
	i++;
	keyRect = BRect(11, 78, 29, 96);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	keyRect.right += 18;
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	keyRect.left += 18;
	
	// Insert, pg up ...
	i++;
	keyRect.OffsetBySelf(35, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	
	// 2nd line : tab and azerty ...
	i = 0x26;
	keyRect = BRect(11, 96, 38, 114);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(27, 0);
	keyRect.right -= 9;
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	keyRect.right += 9;
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	keyRect.left += 9;
	
	// Suppr, pg down ...
	i++;
	keyRect.OffsetBySelf(35, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	
	// 3rd line : caps and qsdfg ...
	i = 0x3b;
	keyRect = BRect(11, 114, 47, 132);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(36, 0);
	keyRect.right -= 18;
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	keyRect.right += 18;
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	keyRect.left += 18;
	
	// 4th line : shift and wxcv ...
	i = 0x4b;
	keyRect = BRect(11, 132, 56, 150);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(45, 0);
	keyRect.right -= 27;
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	keyRect.right += 27;
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	keyRect.left += 27;
	
	//5th line : Ctrl, Alt, Space ...
	i = 0x5c;
	keyRect = BRect(11, 150, 38, 168);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(27, 0);
	keyRect.OffsetBySelf(26, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(27, 0);
	keyRect.right += 92;
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.right -= 92;
	keyRect.OffsetBySelf(92, 0);
	keyRect.OffsetBySelf(27, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(26, 0);
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	
	// Arrows
	i++;
	keyRect = BRect(298, 150, 316, 168);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i = 0x57;
	keyRect.OffsetBySelf(-18, -18);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	
	// numkeys
	i = 0x22;
	keyRect = BRect(369, 78, 387, 96);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i = 0x37;
	keyRect.OffsetBySelf(-54, 18);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	keyRect.bottom += 18;
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i = 0x48;
	keyRect.bottom -= 18;
	keyRect.OffsetBySelf(-54, 18);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i = 0x58;
	keyRect.OffsetBySelf(-36, 18);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.OffsetBySelf(18, 0);
	keyRect.bottom += 18;
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i = 0x64;
	keyRect.bottom -= 18;
	keyRect.OffsetBySelf(-54, 18);
	keyRect.right += 18;
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	i++;
	keyRect.right -= 18;
	keyRect.OffsetBySelf(36, 0);
	fKeysRect[i] = keyRect;
	fKeysToDraw[i] = true;
	
	for (uint32 j = 0; j < 128; j++)
		fKeysVertical[j] = false;
	
	fKeysVertical[0x5e] = true;
	
	fActiveDeadKey = 0;
	
	for (int8 j = 0; j < 16; j++)
		fOldKeyInfo.key_states[j] = 0;
	fOldKeyInfo.modifiers = 0;
}


MapView::~MapView()
{
	delete fBitmap;
}


void
MapView::_InitOffscreen()
{
	if (fBitmap->Lock()) {			
		_DrawBackground();
		fOffscreenView->Sync();
		fBitmap->Unlock();
	}
}


void
MapView::AttachedToWindow()
{
	BControl::AttachedToWindow();
		
	SetEventMask(B_KEYBOARD_EVENTS, 0);
	SetViewColor(B_TRANSPARENT_COLOR);	
	fTextView->SetViewColor(255, 255, 255);
	_InitOffscreen();	
}


void
MapView::_DrawBackground(){
	BRect r = fOffscreenView->Bounds();
	fOffscreenView->SetHighColor(0, 0, 0);
	fOffscreenView->StrokeRect(r);
	
	r.InsetBySelf(1, 1);
	fOffscreenView->SetHighColor(168, 168, 168);
	fOffscreenView->StrokeRect(r);
	fOffscreenView->SetHighColor(80, 80, 80);
	fOffscreenView->StrokeLine(BPoint(r.left + 2, r.bottom), r.RightBottom());
	fOffscreenView->StrokeLine(r.RightTop());
	
	r.InsetBySelf(1, 1);
	fOffscreenView->SetHighColor(255, 255, 255);
	fOffscreenView->StrokeRect(r);
	fOffscreenView->SetHighColor(112, 112, 112);
	fOffscreenView->StrokeLine(BPoint(r.left + 1, r.bottom), r.RightBottom());
	fOffscreenView->StrokeLine(r.RightTop());
	
	r.InsetBySelf(1, 1);
	fOffscreenView->SetHighColor(168, 168, 168);
	fOffscreenView->FillRect(r);
	
	fOffscreenView->SetHighColor(255, 255, 255);
	fOffscreenView->FillRect(BRect(r.right - 1, r.bottom - 1, r.right, r.bottom));
	fOffscreenView->SetHighColor(184, 184, 184);
	fOffscreenView->StrokeLine(BPoint(r.right - 9, r.bottom), BPoint(r.right - 2, r.bottom));
	fOffscreenView->StrokeLine(BPoint(r.right, r.bottom - 9), BPoint(r.right, r.bottom - 2));
		
	// Esc key
	_DrawBorder(fOffscreenView, BRect(10, 49, 30, 69));
	
	// Fx keys
	_DrawBorder(fOffscreenView, BRect(46, 49, 120, 69));
	
	_DrawBorder(fOffscreenView, BRect(127, 49, 201, 69));
	
	_DrawBorder(fOffscreenView, BRect(208, 49, 282, 69));
	
	// Pause, PrintScreen, ...
	_DrawBorder(fOffscreenView, BRect(297, 49, 353, 69));
	
	// Insert, pg up ...
	_DrawBorder(fOffscreenView, BRect(297, 77, 353, 115));
	
	fOffscreenView->SetHighColor(80, 80, 80);
	fOffscreenView->StrokeLine(BPoint(10, 169), BPoint(10, 77));
	fOffscreenView->StrokeLine(BPoint(282, 77));
	fOffscreenView->SetHighColor(255, 255, 255);
	fOffscreenView->StrokeLine(BPoint(282, 169));
	fOffscreenView->StrokeLine(BPoint(253, 169));
	fOffscreenView->SetHighColor(80, 80, 80);
	fOffscreenView->StrokeLine(BPoint(253, 151));
	fOffscreenView->SetHighColor(255, 255, 255);
	fOffscreenView->StrokeLine(BPoint(238, 151));
	fOffscreenView->StrokeLine(BPoint(238, 169));
	fOffscreenView->StrokeLine(BPoint(63, 169));
	fOffscreenView->SetHighColor(80, 80, 80);
	fOffscreenView->StrokeLine(BPoint(63, 151));
	fOffscreenView->SetHighColor(255, 255, 255);
	fOffscreenView->StrokeLine(BPoint(39, 151));
	fOffscreenView->StrokeLine(BPoint(39, 169));
	fOffscreenView->StrokeLine(BPoint(11, 169));
	
	// Arrows
	fOffscreenView->SetHighColor(80, 80, 80);
	fOffscreenView->StrokeLine(BPoint(297, 169), BPoint(297, 149));
	fOffscreenView->StrokeLine(BPoint(315, 149));
	fOffscreenView->StrokeLine(BPoint(315, 131));
	fOffscreenView->StrokeLine(BPoint(335, 131));
	fOffscreenView->StrokeLine(BPoint(336, 149), BPoint(353, 149));
	fOffscreenView->SetHighColor(255, 255, 255);
	fOffscreenView->StrokeLine(BPoint(335, 132), BPoint(335, 149));
	fOffscreenView->StrokeLine(BPoint(353, 150), BPoint(353, 169));
	fOffscreenView->StrokeLine(BPoint(298, 169));
	
	// numkeys
	_DrawBorder(fOffscreenView, BRect(368, 77, 442, 169));
	
	_DrawLocksBackground();
	
	// the line separator
	r = BRect(11, 40, 353, 43);
	fOffscreenView->SetHighColor(255, 255, 255);
	fOffscreenView->StrokeLine(r.LeftBottom(), r.LeftTop());
	fOffscreenView->StrokeLine(r.RightTop());
	fOffscreenView->SetHighColor(80, 80, 80);
	fOffscreenView->StrokeLine(r.RightBottom());
	fOffscreenView->StrokeLine(r.LeftBottom());
	r.OffsetBySelf(2, 4);
	r.bottom = r.top + 1;
	fOffscreenView->SetHighColor(136, 136, 136);
	fOffscreenView->FillRect(r);
	fOffscreenView->FillRect(BRect(354, 41, 355, 43));	
	
	// around the textview
	fOffscreenView->SetHighColor(0, 0, 0);
	r = BRect(11, 13, Bounds().right - 11, 31);
	fOffscreenView->StrokeLine(r.LeftBottom(), r.LeftTop());
	fOffscreenView->StrokeLine(r.RightTop());
	fOffscreenView->SetHighColor(80, 80, 80);
	fOffscreenView->StrokeLine(r.LeftBottom() + BPoint(1, 0),
		r.LeftTop() + BPoint(1, 1));
	fOffscreenView->StrokeLine(r.RightTop() + BPoint(0,1));
	fOffscreenView->SetHighColor(136, 136, 136);
	fOffscreenView->StrokeLine(r.LeftBottom() + BPoint(2, -1),
		r.LeftTop() + BPoint(2, 2));
	fOffscreenView->StrokeLine(r.RightTop()+BPoint(0, 2));
	fOffscreenView->StrokeLine(r.LeftBottom()+BPoint(1, 0),
		r.LeftBottom() + BPoint(1, 0));
	fOffscreenView->SetHighColor(255, 255, 255);
	fOffscreenView->StrokeLine(r.RightTop() + BPoint(0, 1), r.RightBottom());
	fOffscreenView->StrokeLine(r.LeftBottom() + BPoint(2, 0));
	
	_DrawKeysBackground();
	
}


void
MapView::Draw(BRect rect)
{	
	// draw the background
	if (!fBitmap->Lock())
		return;

	if (fOffscreenView->Bounds().Intersects(rect)) {
		BRegion region(rect);
		ConstrainClippingRegion(&region);
		DrawBitmapAsync(fBitmap, B_ORIGIN);
		ConstrainClippingRegion(NULL);
	}

	fBitmap->Unlock();
	
	// draw symbols and lights
	for (uint32 i = 1; i <= 128; i++)
		if (fKeysToDraw[i] && rect.Intersects(fKeysRect[i]))
			_DrawKey(i);			
	
	if (rect.Intersects(BRect(368, 49, 442, 69)))
		_DrawLocksLights();	
}


void
MapView::_DrawLocksBackground()
{
	_DrawBorder(fOffscreenView, BRect(368, 49, 442, 69));
	
	escapement_delta delta;
	delta.nonspace = 0.0;
	BFont font(be_plain_font);
	font.SetSize(8.0);
	font.SetFlags(B_DISABLE_ANTIALIASING);
	font.SetSpacing(B_CHAR_SPACING);
	fOffscreenView->SetFont(&font);
	BRect lightRect = BRect(372, 53, 386, 56);
	
	fOffscreenView->SetHighColor(80, 80, 80);
	fOffscreenView->StrokeLine(lightRect.LeftBottom(), lightRect.RightBottom());
	fOffscreenView->StrokeLine(lightRect.RightTop());
	fOffscreenView->SetHighColor(255, 255, 255);
	fOffscreenView->StrokeLine(BPoint(lightRect.right - 1, lightRect.top),
		lightRect.LeftTop());
	fOffscreenView->StrokeLine(BPoint(lightRect.left, lightRect.bottom - 1));
	fOffscreenView->SetHighColor(0, 55, 0);	
	fOffscreenView->FillRect(lightRect.InsetByCopy(1, 1));
	fOffscreenView->SetHighColor(64, 64, 64);
	fOffscreenView->DrawString("num", BPoint(lightRect.left - 2, 65), &delta);
	
	lightRect.OffsetBy(26, 0);
	fOffscreenView->SetHighColor(80, 80, 80);
	fOffscreenView->StrokeLine(lightRect.LeftBottom(), lightRect.RightBottom());
	fOffscreenView->StrokeLine(lightRect.RightTop());
	fOffscreenView->SetHighColor(255, 255, 255);
	fOffscreenView->StrokeLine(BPoint(lightRect.right - 1, lightRect.top),
		lightRect.LeftTop());
	fOffscreenView->StrokeLine(BPoint(lightRect.left, lightRect.bottom - 1));
	fOffscreenView->SetHighColor(0, 55, 0);
	fOffscreenView->FillRect(lightRect.InsetByCopy(1, 1));
	fOffscreenView->SetHighColor(64, 64, 64);
	fOffscreenView->DrawString("caps", BPoint(lightRect.left - 2, 65), &delta);
	
	lightRect.OffsetBy(26, 0);
	fOffscreenView->SetHighColor(80, 80, 80);
	fOffscreenView->StrokeLine(lightRect.LeftBottom(), lightRect.RightBottom());
	fOffscreenView->StrokeLine(lightRect.RightTop());
	fOffscreenView->SetHighColor(255, 255, 255);
	fOffscreenView->StrokeLine(BPoint(lightRect.right - 1, lightRect.top),
		lightRect.LeftTop());
	fOffscreenView->StrokeLine(BPoint(lightRect.left, lightRect.bottom - 1));
	fOffscreenView->SetHighColor(0, 55, 0);
	fOffscreenView->FillRect(lightRect.InsetByCopy(1, 1));
	fOffscreenView->SetHighColor(64, 64, 64);
	fOffscreenView->DrawString("scroll", BPoint(lightRect.left - 4, 65), &delta);
}


void MapView::_DrawLocksLights()
{
	BRect lightRect = BRect(372, 53, 386, 56);
	lightRect.InsetBy(1, 1);
	
	SetHighColor(0, 55, 0);	
	if (fOldKeyInfo.modifiers & B_NUM_LOCK) 
		SetHighColor(0, 178, 0);
	FillRect(lightRect);
	
	lightRect.OffsetBy(26, 0);
	SetHighColor(0, 55, 0);	
	if (fOldKeyInfo.modifiers & B_CAPS_LOCK) 
		SetHighColor(0, 178, 0);
	FillRect(lightRect);
	
	lightRect.OffsetBy(26, 0);
	SetHighColor(0, 55, 0);
	if (fOldKeyInfo.modifiers & B_SCROLL_LOCK) 
		SetHighColor(0, 178, 0);
	FillRect(lightRect);		
}


void
MapView::_InvalidateKeys()
{
	Invalidate();
}


void
MapView::_InvalidateKey(uint32 keyCode)
{
	Invalidate(fKeysRect[keyCode]);	
	
}


void
MapView::_DrawKeysBackground()
{
	for (uint32 keyCode = 1; keyCode <= 128; keyCode++){
		
		BRect r = fKeysRect[keyCode];
		if (!r.IsValid())
			return;
		fOffscreenView->SetHighColor(0, 0, 0);
		fOffscreenView->StrokeRect(r);
		
		bool vertical = fKeysVertical[keyCode];
		int32 deadKey = fCurrentMap->IsDeadKey(keyCode, fOldKeyInfo.modifiers);
		bool secondDeadKey = fCurrentMap->IsDeadSecondKey(keyCode,
			 fOldKeyInfo.modifiers, fActiveDeadKey);
				
		r.InsetBySelf(1, 1);
		if (!secondDeadKey && deadKey == 0) {
			fOffscreenView->SetHighColor(64, 64, 64);
			fOffscreenView->StrokeRect(r);
			
			fOffscreenView->BeginLineArray(14);
			rgb_color color1 = {200, 200, 200};
			fOffscreenView->AddLine(BPoint(r.left, r.bottom - 1), r.LeftTop(),
				 color1);
			fOffscreenView->AddLine(r.LeftTop(), BPoint(r.left+3, r.top),
				 color1);
			rgb_color color2 = {184, 184, 184};
			fOffscreenView->AddLine(BPoint(r.left + 3, r.top),
				 BPoint(r.left + 6, r.top), color2);
			rgb_color color3 = {168, 168, 168};
			fOffscreenView->AddLine(BPoint(r.left + 6, r.top),
				 BPoint(r.left + 9, r.top), color3);
			rgb_color color4 = {152, 152, 152};
			fOffscreenView->AddLine(BPoint(r.left + 9, r.top),
				 BPoint(r.right - 1, r.top), color4);
			
			r.InsetBySelf(1,1);
			fOffscreenView->SetHighColor(255, 255, 255);
			fOffscreenView->StrokeRect(r);
			
			rgb_color color6 = {96, 96, 96};
			fOffscreenView->AddLine(r.LeftBottom(), r.RightBottom(), color6);
			rgb_color color5 = {160, 160, 160};
			fOffscreenView->AddLine(r.LeftBottom(), r.LeftBottom(), color5);
			rgb_color color7 = {64, 64, 64};
			fOffscreenView->AddLine(r.RightBottom(),
				 BPoint(r.right, r.bottom - 1), color7);
			fOffscreenView->AddLine(BPoint(r.right, r.bottom - 1),
				 BPoint(r.right, r.top - 1), color6);
			fOffscreenView->AddLine(BPoint(r.right, r.top - 1), r.RightTop(),
				 color5);
			rgb_color color8 = {255, 255, 255};
			fOffscreenView->AddLine(BPoint(r.left + 1, r.bottom - 1),
				 BPoint(r.left + 2, r.bottom - 1), color8);
			fOffscreenView->AddLine(BPoint(r.left + 2, r.bottom - 1),
				 BPoint(r.right - 1, r.bottom - 1), color1);
			fOffscreenView->AddLine(BPoint(r.right - 1, r.bottom - 1),
				 BPoint(r.right - 1, r.bottom - 2), color5);
			fOffscreenView->AddLine(BPoint(r.right - 1, r.bottom - 2),
				 BPoint(r.right - 1, r.top + 1), color1);
			fOffscreenView->EndLineArray();
		}
		
		r.InsetBySelf(1, 1);
		r.bottom -= 1;
		BRect fillRect = r;
		
		if (!vertical) {
			int32 w1 = 4;
			int32 w2 = 3;
			if (fKeysRect[keyCode].Width() > 20) {
				w1 = 6;
				w2 = 6;
			}
			
			fillRect.right = fillRect.left + w1;
			fOffscreenView->SetHighColor(152, 152, 152);
			fOffscreenView->FillRect(fillRect);
			fillRect.left += w1;
			fillRect.right = fillRect.left + w2;
			fOffscreenView->SetHighColor(168, 168, 168);
			fOffscreenView->FillRect(fillRect);
			fillRect.left += w2;
			fillRect.right = r.right - 1;
			fOffscreenView->SetHighColor(184, 184, 184);
			fOffscreenView->FillRect(fillRect);
		} else {
			fOffscreenView->SetHighColor(200, 200, 200);
			fillRect.right -= 1;
			fillRect.bottom = fillRect.top + 2;
			fOffscreenView->FillRect(fillRect);
			fOffscreenView->SetHighColor(184, 184, 184);
			fillRect.OffsetBySelf(0, 3);
			fOffscreenView->FillRect(fillRect);
			fOffscreenView->SetHighColor(168, 168, 168);
			fillRect.OffsetBySelf(0, 3);
			fOffscreenView->FillRect(fillRect);
			fOffscreenView->SetHighColor(152, 152, 152);
			fillRect.OffsetBySelf(0, 3);
			fOffscreenView->FillRect(fillRect);
		}		
	}
	
}


void
MapView::_DrawKey(uint32 keyCode)
{
	BRect r = fKeysRect[keyCode];
	if (!r.IsValid())
		return;
		
	bool pressed = (fOldKeyInfo.key_states[keyCode >> 3] & (1 << (7 - keyCode % 8))) || (keyCode == fCurrentMouseKey);
	int32 deadKey = fCurrentMap->IsDeadKey(keyCode, fOldKeyInfo.modifiers);
	bool secondDeadKey = fCurrentMap->IsDeadSecondKey(keyCode, fOldKeyInfo.modifiers, fActiveDeadKey);
		
	if (!pressed) {
		r.InsetBySelf(1, 1);
		if (secondDeadKey) {
			SetHighColor(255, 0, 0);
			StrokeRect(r);
			r.InsetBySelf(1, 1);
			StrokeRect(r);
		} else if (deadKey > 0) {
			SetHighColor(255, 255, 0);
			StrokeRect(r);
			r.InsetBySelf(1, 1);
			StrokeRect(r);
		}		
	} else {
		r.InsetBySelf(1, 1);
		
		if (secondDeadKey) {
			SetHighColor(255, 0, 0);
			StrokeRect(r);
			r.InsetBySelf(1, 1);
			StrokeRect(r);
		} else if (deadKey > 0) {
			SetHighColor(255, 255, 0);
			StrokeRect(r);
			r.InsetBySelf(1, 1);
			StrokeRect(r);
		} else {
			SetHighColor(48, 48, 48);
			StrokeRect(r);
			
			BeginLineArray(2);
			rgb_color color1 = {136, 136, 136};
			AddLine(BPoint(r.left + 1, r.bottom), r.RightBottom(), color1);
			AddLine(r.RightBottom(), BPoint(r.right, r.top + 1), color1);
			EndLineArray();
			
			r.InsetBySelf(1, 1);
			SetHighColor(72, 72, 72);
			StrokeRect(r);
			
			BeginLineArray(4);
			rgb_color color2 = {48, 48, 48};
			AddLine(r.LeftTop(), r.LeftTop(), color2);
			rgb_color color3 = {152, 152, 152};
			AddLine(BPoint(r.left + 1, r.bottom), r.RightBottom(), color3);
			AddLine(r.RightBottom(), r.RightTop(), color3);
			rgb_color color4 = {160, 160, 160};
			AddLine(r.RightTop(), r.RightTop(), color4);
			EndLineArray();
		}
		
		r.InsetBySelf(1, 1);
		SetHighColor(112, 112, 112);
		FillRect(r);
		SetHighColor(136, 136, 136);
		StrokeLine(r.LeftTop(), r.LeftTop());
	}
	
	char *str = NULL;
	int32 numBytes;
			
	fCurrentMap->GetChars(keyCode, fOldKeyInfo.modifiers, fActiveDeadKey, &str, &numBytes);
	if (str) {
		bool hasGlyphs;
		if (deadKey > 0) {
			delete str;
			switch (deadKey) {
				case 1: str = strdup("'"); break;
				case 2: str = strdup("`"); break;
				case 3: str = strdup("^"); break;
				case 4: str = strdup("\""); break;
				case 5: str = strdup("~"); break;
			}
		} 
		fCurrentFont.GetHasGlyphs(str, 1, &hasGlyphs);
		
		if (hasGlyphs) {
			SetFont(&fCurrentFont);			
			SetHighColor(0, 0, 0);
			SetLowColor(160, 160, 160);			
			BPoint point = fKeysRect[keyCode].LeftBottom();
			point.x += 5;
			point.y -= 5;
			if (pressed) {
				point.y += 1;
				SetLowColor(112, 112, 112);
			}
			DrawString(str, point);
		}
		delete str;
	}	
}


void
MapView::_DrawBorder(BView *view, const BRect& rect)
{
	rgb_color gray = {80, 80, 80};
	rgb_color white = {255, 255, 255};
	
	view->BeginLineArray(4);
	view->AddLine(rect.LeftTop(), rect.LeftBottom(), gray);
	view->AddLine(rect.LeftTop(), rect.RightTop(), gray);
	view->AddLine(BPoint(rect.left + 1, rect.bottom), rect.RightBottom(), white);
	view->AddLine(rect.RightBottom(), BPoint(rect.right, rect.top + 1), white);
	view->EndLineArray();
}


void
MapView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case kMsgMenuEditUndo:
			fTextView->Undo(be_clipboard);
			break;
		case kMsgMenuEditCut:
			fTextView->Cut(be_clipboard);
			break;
		case kMsgMenuEditCopy:
			fTextView->Copy(be_clipboard);
			break;
		case kMsgMenuEditPaste:
			fTextView->Paste(be_clipboard);
			break;
		case kMsgMenuEditClear:
			fTextView->Clear();
			break;
		case kMsgMenuEditSelectAll:
			fTextView->SelectAll();
			break;
		case B_KEY_DOWN:
		case B_KEY_UP:
		case B_UNMAPPED_KEY_DOWN:
		case B_UNMAPPED_KEY_UP:
		case B_MODIFIERS_CHANGED: {
			key_info info;
			const uint8 *states;
			ssize_t size;
			
			if ((msg->FindData("states", B_UINT8_TYPE, reinterpret_cast<const void **>(&states), &size) != B_OK)
				|| (msg->FindInt32("modifiers", reinterpret_cast<int32 *>(&info.modifiers)) != B_OK))
				break;
			
			if (fOldKeyInfo.modifiers != info.modifiers) {
				fOldKeyInfo.modifiers = info.modifiers;
				for (int8 i = 0; i < 16; i++) 
					fOldKeyInfo.key_states[i] = states[i];
				_InvalidateKeys();
				_DrawLocksLights();
			} else {
				
				int32 keyCode = -1;
				for (int8 i = 0; i < 16; i++)
					if (fOldKeyInfo.key_states[i] != states[i]) {
						uint8 stbits = fOldKeyInfo.key_states[i] ^ states[i];
						fOldKeyInfo.key_states[i] = states[i];
						for (int8 j = 7; stbits; j--, stbits >>= 1)
							if (stbits & 1) {
								keyCode = i * 8 + j;
								_InvalidateKey(keyCode);
							}
					}
				
				if (keyCode < 0) 
					for (int8 i = 0; i < 16; i++) {
						uint8 stbits = states[i];
						for (int8 j = 7; stbits; j--, stbits >>= 1)
							if (stbits & 1) {
								keyCode = i * 8 + j;
								if (!fCurrentMap->IsModifierKey(keyCode)) {
									i = 16;
									break;
								}
							}
					}
				
				if (Window()->IsActive()
					&& msg->what == B_KEY_DOWN) {
					fTextView->MakeFocus();
					char *str = NULL;
					int32 numBytes;
					if (fActiveDeadKey) {
						fCurrentMap->GetChars(keyCode, fOldKeyInfo.modifiers, fActiveDeadKey, &str, &numBytes);
						if (numBytes > 0) {
							fTextView->FakeKeyDown(str, numBytes);
						}
						fActiveDeadKey = 0;
						_InvalidateKeys();
					} else {
						fCurrentMap->GetChars(keyCode, fOldKeyInfo.modifiers, fActiveDeadKey, &str, &numBytes);
						fActiveDeadKey = fCurrentMap->IsDeadKey(keyCode, fOldKeyInfo.modifiers);
						if (fActiveDeadKey)
							_InvalidateKeys();
						else if (numBytes > 0) {
							fTextView->FakeKeyDown(str, numBytes);
						}
					}
					delete str;
				}
			}
			break;
		}
		default:
			BControl::MessageReceived(msg);
	}	
}


void
MapView::KeyDown(const char* bytes, int32 numBytes)
{
	MessageReceived(Window()->CurrentMessage());
}


void
MapView::KeyUp(const char* bytes, int32 numBytes)
{
	MessageReceived(Window()->CurrentMessage());
}


void
MapView::MouseDown(BPoint point)
{
	uint32 buttons;
	GetMouse(&point, &buttons);
	if (buttons & B_PRIMARY_MOUSE_BUTTON) {
		fCurrentMouseKey = 0;
		for (int32 i = 0; i < 128; i++) {
			if (fKeysRect[i].IsValid() && fKeysRect[i].Contains(point)) {
				fCurrentMouseKey = i;
				_DrawKey(fCurrentMouseKey);
				char *str = NULL;
				int32 numBytes;
				fCurrentMap->GetChars(fCurrentMouseKey, fOldKeyInfo.modifiers, fActiveDeadKey, &str, &numBytes);
				if (numBytes > 0) {
					fTextView->FakeKeyDown(str, numBytes);
					delete str;
				}
				SetTracking(true);
				SetMouseEventMask(B_POINTER_EVENTS,
					B_LOCK_WINDOW_FOCUS | B_NO_POINTER_HISTORY);
				break;
			}
		}
	}
}


void
MapView::MouseUp(BPoint point)
{
	if (IsTracking())
		SetTracking(false);
	uint32 value = fCurrentMouseKey;
	fCurrentMouseKey = 0;
	_InvalidateKey(value);
}


void
MapView::MouseMoved(BPoint point, uint32 transit, const BMessage *msg)
{
	if (IsTracking()) {
		uint32 value = fCurrentMouseKey;
		for (int32 i = 0; i < 128; i++) {
			if (fKeysRect[i].Contains(point) && !fKeysRect[value].Contains(point)) {
				fCurrentMouseKey = i;
				_InvalidateKey(value);
				_InvalidateKey(fCurrentMouseKey);
				char *str = NULL;
				int32 numBytes;
				fCurrentMap->GetChars(fCurrentMouseKey, fOldKeyInfo.modifiers, fActiveDeadKey, &str, &numBytes);
				if (numBytes > 0) {
					fTextView->FakeKeyDown(str, numBytes);
					delete str;
				}
				break;
			}
		}
	}
	BControl::MouseMoved(point, transit, msg);
}


void 
MapView::SetFontFamily(const font_family family) 
{
	fCurrentFont.SetFamilyAndStyle(family, NULL); 
	fTextView->SetFontAndColor(&fCurrentFont);
}
