// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        KeymapWindow.cpp
//  Author:      Sandor Vroemisse, Jérôme Duval
//  Description: Keymap Preferences
//  Created :    July 12, 2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#include <Alert.h>
#include <Application.h>
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
#include <ScrollView.h>
#include <StringView.h>
#include <TextView.h>
#include <View.h>
#include <string.h>
#include "KeymapWindow.h"
#include "KeymapListItem.h"
#include "KeymapApplication.h"

#define MENU_FILE_OPEN		'mMFO'
#define MENU_FILE_SAVE		'mMFS'
#define MENU_FILE_SAVE_AS	'mMFA'
#define MENU_EDIT_UNDO		'mMEU'
#define MENU_EDIT_CUT		'mMEX'
#define MENU_EDIT_COPY		'mMEC'
#define MENU_EDIT_PASTE		'mMEV'
#define MENU_EDIT_CLEAR		'mMEL'
#define MENU_EDIT_SELECT_ALL 'mMEA'
#define MENU_FONT_CHANGED	'mMFC'
#define SYSTEM_MAP_SELECTED	'SmST'
#define USER_MAP_SELECTED	'UmST'
#define	USE_KEYMAP			'UkyM'
#define REVERT				'Rvrt'

KeymapWindow::KeymapWindow( BRect frame )
	:	BWindow( frame, WINDOW_TITLE, B_TITLED_WINDOW,
			B_NOT_ZOOMABLE|B_NOT_RESIZABLE|B_ASYNCHRONOUS_CONTROLS )
{
	// Add the menu bar
	BMenuBar *menubar = AddMenuBar();

	// The view to hold all but the menu bar
	BRect bounds = Bounds();
	bounds.top = menubar->Bounds().bottom + 1;
	BBox *placeholderView = new BBox(bounds, "placeholderView", 
		B_FOLLOW_ALL);
	placeholderView->SetBorder(B_NO_BORDER);
	AddChild( placeholderView );

	// Create the Maps box and contents
	AddMaps(placeholderView);
	
	fMapView = new MapView(BRect(150,9,600,189), "mapView", &fCurrentMap);
	placeholderView->AddChild(fMapView);
	SetPulseRate(10000);
	
	BMenuItem *item = fFontMenu->FindMarked();
	if (item) {
		fMapView->SetFontFamily(item->Label());
	}
	
	// The 'Use' button
	fUseButton = new BButton(BRect(527,200, 600,220), "useButton", "Use",
		new BMessage( USE_KEYMAP ));
	placeholderView->AddChild( fUseButton );
	
	// The 'Revert' button
	fRevertButton = new BButton(BRect(442,200, 515,220), "revertButton", "Revert",
		new BMessage( REVERT ));
	placeholderView->AddChild( fRevertButton );
	
	BPath path;
	find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	path.Append("Keymap");
	
	entry_ref ref;
	get_ref_for_path(path.Path(), &ref);
		
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
	menubar = new BMenuBar( bounds, "menubar" );
	AddChild( menubar );
	
	// Create the File menu
	menu = new BMenu( "File" );
	menu->AddItem( new BMenuItem( "Open" B_UTF8_ELLIPSIS,
		new BMessage( MENU_FILE_OPEN ), 'O' ) );
	menu->AddSeparatorItem();
	currentItem = new BMenuItem( "Save",
		new BMessage( MENU_FILE_SAVE ), 'S' );
	currentItem->SetEnabled( false );
	menu->AddItem( currentItem );
	menu->AddItem( new BMenuItem( "Save As" B_UTF8_ELLIPSIS,
		new BMessage( MENU_FILE_SAVE_AS )));
	menu->AddSeparatorItem();
	menu->AddItem( new BMenuItem( "Quit",
		new BMessage( B_QUIT_REQUESTED ), 'Q' ));
	menubar->AddItem( menu );

	// Create the Edit menu
	menu = new BMenu( "Edit" );
	currentItem = new BMenuItem( "Undo",
		new BMessage( MENU_EDIT_UNDO ), 'Z' );
	currentItem->SetEnabled( false );
	menu->AddItem( currentItem );
	menu->AddSeparatorItem();
	menu->AddItem( new BMenuItem( "Cut",
		new BMessage( MENU_EDIT_CUT ), 'X' ));
	menu->AddItem( new BMenuItem( "Copy",
		new BMessage( MENU_EDIT_COPY ), 'C' ));
	menu->AddItem( new BMenuItem( "Paste",
		new BMessage( MENU_EDIT_PASTE ), 'V' ));
	menu->AddItem( new BMenuItem( "Clear",
		new BMessage( MENU_EDIT_CLEAR )));
	menu->AddSeparatorItem();
	menu->AddItem( new BMenuItem( "Select All",
		new BMessage( MENU_EDIT_SELECT_ALL ), 'A' ));
	menubar->AddItem( menu );
	
	// Create the Font menu
	fFontMenu = new BMenu( "Font" );
	fFontMenu->SetRadioMode(true);
	int32 numFamilies = count_font_families(); 
	font_family family, current_family;
	font_style current_style; 
	uint32 flags;
	
	be_plain_font->GetFamilyAndStyle(&current_family, &current_style);
		
	for (int32 i = 0; i < numFamilies; i++ )
		if ( get_font_family(i, &family, &flags) == B_OK ) {
			BMenuItem *item = 
				new BMenuItem(family, new BMessage( MENU_FONT_CHANGED));
			fFontMenu->AddItem(item);
			if(strcmp(family, current_family) == 0)
				item->SetMarked(true);
		}
	menubar->AddItem( fFontMenu );
	
	return menubar;
}


void 
KeymapWindow::AddMaps(BView *placeholderView)
{
	// The Maps box
	BRect bounds = BRect(9,11,140,226);
	BBox *mapsBox = new BBox(bounds);
	mapsBox->SetLabel("Maps");
	placeholderView->AddChild( mapsBox );

	// The System list
	BStringView *systemLabel = new BStringView(BRect(13,13,113,33), "system", "System");
	mapsBox->AddChild(systemLabel);
	
	bounds = BRect( 13,35, 103,105 );
	fSystemListView = new BListView( bounds, "systemList" );
	
	mapsBox->AddChild( new BScrollView( "systemScrollList", fSystemListView,
		B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true ));
	fSystemListView->SetSelectionMessage( new BMessage( SYSTEM_MAP_SELECTED ));

	// The User list
	BStringView *userLabel = new BStringView(BRect(13,110,113,128), "user", "User");
	mapsBox->AddChild(userLabel);

	bounds = BRect(13,130,103,200);
	fUserListView = new BListView( bounds, "userList" );
	// '(Current)'
	KeymapListItem *currentKeymapItem = (KeymapListItem*)fUserListView->FirstItem();
	if( currentKeymapItem != NULL )
		fUserListView->AddItem( currentKeymapItem );
	// Saved keymaps
	mapsBox->AddChild( new BScrollView( "userScrollList", fUserListView,
		B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true ));
	fUserListView->SetSelectionMessage( new BMessage( USER_MAP_SELECTED ));
		
	FillSystemMaps();
	
	FillUserMaps();
}


bool 
KeymapWindow::QuitRequested()
{
	if (!IsActive())
		return false;
	be_app->PostMessage( B_QUIT_REQUESTED );
	return true;
}


void 
KeymapWindow::MessageReceived( BMessage* message )
{
	switch( message->what ) {
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
		case MENU_FILE_OPEN:
			fOpenPanel->Show();
			break;
		case MENU_FILE_SAVE:
			break;
		case MENU_FILE_SAVE_AS:
			fSavePanel->Show();
			break;
		case MENU_EDIT_UNDO:
		case MENU_EDIT_CUT:
		case MENU_EDIT_COPY:
		case MENU_EDIT_PASTE:
		case MENU_EDIT_CLEAR:
		case MENU_EDIT_SELECT_ALL:
			fMapView->MessageReceived(message);
			break;
		case MENU_FONT_CHANGED:
		{
			BMenuItem *item = fFontMenu->FindMarked();
			if (item) {
				fMapView->SetFontFamily(item->Label());
				fMapView->Invalidate();
			}	
		}
			break;
		case SYSTEM_MAP_SELECTED:
		{
			KeymapListItem *keymapListItem = 
				(KeymapListItem*)fSystemListView->ItemAt(fSystemListView->CurrentSelection());
			if (keymapListItem) {
				fCurrentMap.Load(keymapListItem->KeymapEntry());
				fMapView->Invalidate();
				
				// Deselect item in other BListView
				fUserListView->DeselectAll();
			}
		}
			break;
		case USER_MAP_SELECTED:
		{	
			KeymapListItem *keymapListItem = 
				(KeymapListItem*)fUserListView->ItemAt(fUserListView->CurrentSelection());
			if (keymapListItem) {
				fCurrentMap.Load(keymapListItem->KeymapEntry());
				fMapView->Invalidate();
					
				// Deselect item in other BListView
				fSystemListView->DeselectAll();
			}
		}
			break;
		case USE_KEYMAP:
			UseKeymap();
			break;
		case REVERT:	// do nothing, just like the original
			break;
		default:	
			BWindow::MessageReceived( message );
			break;
	}
}


void 
KeymapWindow::UseKeymap()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path)!=B_OK)
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
	
	fUserListView->Select(0);
}


void
KeymapWindow::FillSystemMaps()
{
	BListItem *item;
	while ((item = fSystemListView->RemoveItem((int32)0)))
		delete item;

	BPath path;
	if (find_directory(B_BEOS_ETC_DIRECTORY, &path)!=B_OK)
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
	while ((item = fUserListView->RemoveItem((int32)0)))
		delete item;

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path)!=B_OK)
		return;
	
	path.Append("Key_map");

	entry_ref ref;
	get_ref_for_path(path.Path(), &ref);
			
	fUserListView->AddItem(new KeymapListItem(ref, "(Current)"));

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path)!=B_OK)
		return;
	
	path.Append("Keymap");
	
	BDirectory directory;
	
	if (directory.SetTo(path.Path()) == B_OK)
		while( directory.GetNextRef(&ref) == B_OK ) {
			fUserListView->AddItem(new KeymapListItem(ref));
		}
	
	fUserListView->Select(0);
}


BEntry* 
KeymapWindow::CurrentMap()
{
	return NULL;
}


MapView::MapView(BRect rect, const char *name, Keymap* keymap)
	: BControl(rect, name, NULL, NULL, B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW|B_PULSE_NEEDED),
		fCurrentFont(*be_plain_font),
		fCurrentMap(keymap),
		fCurrentMouseKey(0)
{
	BRect frameRect = BRect(14, 16, Bounds().right-12, 30);
	BRect textRect = frameRect;
	textRect.OffsetTo(B_ORIGIN);
	textRect.InsetBy(1,1);
	fTextView = new KeymapTextView(frameRect, "testzone", textRect,
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW | B_FRAME_EVENTS);
	fTextView->MakeEditable(true);
	fTextView->MakeSelectable(true);
	
	AddChild(fTextView);

	BRect keyRect = BRect(11,50,29,68);
	int32 i = 1;
	fKeysRect[i] = keyRect;
	
	// Fx keys
	i++;
	keyRect.OffsetBySelf(36,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	
	i++;
	keyRect.OffsetBySelf(27,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	
	i++;
	keyRect.OffsetBySelf(27,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	
	// Pause, PrintScreen, ...
	i++;
	keyRect.OffsetBySelf(35,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	
	// 1st line : numbers and backspace
	i++;
	keyRect = BRect(11,78,29,96);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	keyRect.right += 18;
	fKeysRect[i] = keyRect;
	keyRect.left += 18;
	
	// Insert, pg up ...
	i++;
	keyRect.OffsetBySelf(35,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	
	// 2nd line : tab and azerty ...
	i = 0x26;
	keyRect = BRect(11,96,38,114);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(27,0);
	keyRect.right -= 9;
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	keyRect.right += 9;
	fKeysRect[i] = keyRect;
	keyRect.left += 9;
	
	// Suppr, pg down ...
	i++;
	keyRect.OffsetBySelf(35,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	
	// 3rd line : caps and qsdfg ...
	i = 0x3b;
	keyRect = BRect(11,114,47,132);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(36,0);
	keyRect.right -= 18;
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	keyRect.right += 18;
	fKeysRect[i] = keyRect;
	keyRect.left += 18;
	
	// 4th line : shift and wxcv ...
	i = 0x4b;
	keyRect = BRect(11,132,56,150);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(45,0);
	keyRect.right -= 27;
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	keyRect.right += 27;
	fKeysRect[i] = keyRect;
	keyRect.left += 27;
	
	//5th line : Ctrl, Alt, Space ...
	i = 0x5c;
	keyRect = BRect(11,150,38,168);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(27,0);
	keyRect.OffsetBySelf(26,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(27,0);
	keyRect.right += 92;
	fKeysRect[i] = keyRect;
	i++;
	keyRect.right -= 92;
	keyRect.OffsetBySelf(92,0);
	keyRect.OffsetBySelf(27,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(26,0);
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	
	// Arrows
	i++;
	keyRect = BRect(298,150,316,168);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i = 0x57;
	keyRect.OffsetBySelf(-18,-18);
	fKeysRect[i] = keyRect;
	
	// numkeys
	i = 0x22;
	keyRect = BRect(369,78,387,96);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i = 0x37;
	keyRect.OffsetBySelf(-54, 18);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	keyRect.bottom += 18;
	fKeysRect[i] = keyRect;
	i = 0x48;
	keyRect.bottom -= 18;
	keyRect.OffsetBySelf(-54, 18);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i = 0x58;
	keyRect.OffsetBySelf(-36, 18);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	fKeysRect[i] = keyRect;
	i++;
	keyRect.OffsetBySelf(18,0);
	keyRect.bottom += 18;
	fKeysRect[i] = keyRect;
	i = 0x64;
	keyRect.bottom -= 18;
	keyRect.OffsetBySelf(-54, 18);
	keyRect.right += 18;
	fKeysRect[i] = keyRect;
	i++;
	keyRect.right -= 18;
	keyRect.OffsetBySelf(36,0);
	fKeysRect[i] = keyRect;
	
	for (uint8 j = 0; j<128; j++)
		fKeysVertical[j] = false;
		
	fKeysVertical[0x5e] = true;
	
	fActiveDeadKey = 0;
}


void
MapView::AttachedToWindow()
{
	SetEventMask(B_KEYBOARD_EVENTS, 0);
	fTextView->SetViewColor(255,255,255);
	BView::AttachedToWindow();
}


void
MapView::Draw(BRect rect)
{
	BRect r = Bounds();
	SetHighColor(0,0,0);
	StrokeRect(r);
	
	r.InsetBySelf(1,1);
	SetHighColor(168,168,168);
	StrokeRect(r);
	SetHighColor(80,80,80);
	StrokeLine(BPoint(r.left+2, r.bottom), r.RightBottom());
	StrokeLine(r.RightTop());
	
	r.InsetBySelf(1,1);
	SetHighColor(255,255,255);
	StrokeRect(r);
	SetHighColor(112,112,112);
	StrokeLine(BPoint(r.left+1, r.bottom), r.RightBottom());
	StrokeLine(r.RightTop());
	
	r.InsetBySelf(1,1);
	SetHighColor(168,168,168);
	FillRect(r);
	
	SetHighColor(255,255,255);
	FillRect(BRect(r.right-1, r.bottom-1, r.right, r.bottom));
	SetHighColor(184,184,184);
	StrokeLine(BPoint(r.right-9, r.bottom), BPoint(r.right-2, r.bottom));
	StrokeLine(BPoint(r.right, r.bottom-9), BPoint(r.right, r.bottom-2));
	
	int32 i = 1;
	
	// Esc key
	DrawKey(i);
	
	DrawBorder(BRect(10,49,30,69));
	
	// Fx keys
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
		
	DrawBorder(BRect(46, 49, 120, 69));
	
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
		
	DrawBorder(BRect(127, 49, 201, 69));
	
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
		
	DrawBorder(BRect(208, 49, 282, 69));
	
	// Pause, PrintScreen, ...
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
		
	DrawBorder(BRect(297, 49, 353, 69));
	
	// 1st line : numbers and backspace
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
		
	// Insert, pg up ...
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	
	DrawBorder(BRect(297, 77, 353, 115));
	
	// 2nd line : tab and azerty ...
	i = 0x26;
	DrawKey(i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	
	// Suppr, pg down ...
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
		
	// 3rd line : caps and qsdfg ...
	i = 0x3b;
	
	DrawKey(i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
			
	// 4th line : shift and wxcv ...
	i = 0x4b;
	DrawKey(i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	
		
	//5th line : Ctrl, Alt, Space ...
	i = 0x5c;
	DrawKey(i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	
	SetHighColor(80,80,80);
	StrokeLine(BPoint(10,169), BPoint(10,77));
	StrokeLine(BPoint(282,77));
	SetHighColor(255,255,255);
	StrokeLine(BPoint(282,169));
	StrokeLine(BPoint(253,169));
	SetHighColor(80,80,80);
	StrokeLine(BPoint(253,151));
	SetHighColor(255,255,255);
	StrokeLine(BPoint(238,151));
	StrokeLine(BPoint(238,169));
	StrokeLine(BPoint(63,169));
	SetHighColor(80,80,80);
	StrokeLine(BPoint(63,151));
	SetHighColor(255,255,255);
	StrokeLine(BPoint(39,151));
	StrokeLine(BPoint(39,169));
	StrokeLine(BPoint(11,169));
	
	// Arrows
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
		
	i = 0x57;
	DrawKey(i);
	
	SetHighColor(80,80,80);
	StrokeLine(BPoint(297,169), BPoint(297,149));
	StrokeLine(BPoint(315,149));
	StrokeLine(BPoint(315,131));
	StrokeLine(BPoint(335,131));
	StrokeLine(BPoint(336,149), BPoint(353,149));
	SetHighColor(255,255,255);
	StrokeLine(BPoint(335,132), BPoint(335,149));
	StrokeLine(BPoint(353,150), BPoint(353,169));
	StrokeLine(BPoint(298,169));
	
	// numkeys
	i = 0x22;
	DrawKey(i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	
	i = 0x37;
	DrawKey(i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	
	i = 0x48;
	DrawKey(i);
	DrawKey(++i);
	DrawKey(++i);
	
	i = 0x58;
	DrawKey(i);
	DrawKey(++i);
	DrawKey(++i);
	DrawKey(++i);
	
	i = 0x64;
	DrawKey(i);
	DrawKey(++i);
		
	DrawBorder(BRect(368, 77, 442, 169));
	
	// lights
#define isLighted(i) (fOldKeyInfo.modifiers & i) 

	DrawBorder(BRect(368, 49, 442, 69));
	
	escapement_delta delta;
	delta.nonspace = 0.0;
	BFont font(be_plain_font);
	font.SetSize(9.0);
	font.SetFlags(B_DISABLE_ANTIALIASING);
	font.SetSpacing(B_CHAR_SPACING);
	SetFont(&font);
	BRect lightRect = BRect(372,53,386,56);
	
	SetHighColor(80,80,80);
	StrokeLine(lightRect.LeftBottom(), lightRect.RightBottom());
	StrokeLine(lightRect.RightTop());
	SetHighColor(255,255,255);
	StrokeLine(BPoint(lightRect.right-1, lightRect.top), lightRect.LeftTop());
	StrokeLine(BPoint(lightRect.left, lightRect.bottom-1));
	SetHighColor(0,55,0);
	if (isLighted(B_NUM_LOCK)) 
		SetHighColor(0,178,0);
	FillRect(lightRect.InsetByCopy(1,1));
	SetHighColor(64,64,64);
	DrawString("num", BPoint(lightRect.left-2, 65), &delta);
		
	lightRect.OffsetBy(26,0);
	SetHighColor(80,80,80);
	StrokeLine(lightRect.LeftBottom(), lightRect.RightBottom());
	StrokeLine(lightRect.RightTop());
	SetHighColor(255,255,255);
	StrokeLine(BPoint(lightRect.right-1, lightRect.top), lightRect.LeftTop());
	StrokeLine(BPoint(lightRect.left, lightRect.bottom-1));
	SetHighColor(0,55,0);
	if (isLighted(B_CAPS_LOCK)) 
		SetHighColor(0,178,0);
	FillRect(lightRect.InsetByCopy(1,1));
	SetHighColor(64,64,64);
	DrawString("caps", BPoint(lightRect.left-3, 65), &delta);
	
	lightRect.OffsetBy(26,0);
	SetHighColor(80,80,80);
	StrokeLine(lightRect.LeftBottom(), lightRect.RightBottom());
	StrokeLine(lightRect.RightTop());
	SetHighColor(255,255,255);
	StrokeLine(BPoint(lightRect.right-1, lightRect.top), lightRect.LeftTop());
	StrokeLine(BPoint(lightRect.left, lightRect.bottom-1));
	SetHighColor(0,55,0);
	if (isLighted(B_SCROLL_LOCK)) 
		SetHighColor(0,178,0);
	FillRect(lightRect.InsetByCopy(1,1));
	SetHighColor(64,64,64);
	DrawString("scroll", BPoint(lightRect.left-4, 65), &delta);
	
	// the line separator
	r = BRect(11, 40, 353, 43);
	SetHighColor(255,255,255);
	StrokeLine(r.LeftBottom(), r.LeftTop());
	StrokeLine(r.RightTop());
	SetHighColor(80,80,80);
	StrokeLine(r.RightBottom());
	StrokeLine(r.LeftBottom());
	r.OffsetBySelf(2,4);
	r.bottom = r.top + 1;
	SetHighColor(136,136,136);
	FillRect(r);
	FillRect(BRect(354,41,355,43));
	
	// around the textview
	SetHighColor(0,0,0);
	r = BRect(11, 13, Bounds().right-11, 31);
	StrokeLine(r.LeftBottom(), r.LeftTop());
	StrokeLine(r.RightTop());
	SetHighColor(80,80,80);
	StrokeLine(r.LeftBottom()+BPoint(1,0), r.LeftTop()+BPoint(1,1));
	StrokeLine(r.RightTop()+BPoint(0,1));
	SetHighColor(136,136,136);
	StrokeLine(r.LeftBottom()+BPoint(2,-1), r.LeftTop()+BPoint(2,2));
	StrokeLine(r.RightTop()+BPoint(0,2));
	StrokeLine(r.LeftBottom()+BPoint(1,0), r.LeftBottom()+BPoint(1,0));
	SetHighColor(255,255,255);
	StrokeLine(r.RightTop()+BPoint(0,1), r.RightBottom());
	StrokeLine(r.LeftBottom()+BPoint(2,0));
	BView::Draw(rect);
}


void
MapView::DrawKey(uint32 keyCode)
{
	BRect r = fKeysRect[keyCode];
	SetHighColor(0,0,0);
	StrokeRect(r);
	
	bool pressed = (fOldKeyInfo.key_states[keyCode>>3] & (1 << (7 - keyCode%8))) || (keyCode == fCurrentMouseKey);
	bool vertical = fKeysVertical[keyCode];
	int32 deadKey = fCurrentMap->IsDeadKey(keyCode, fOldKeyInfo.modifiers);
	bool secondDeadKey = fCurrentMap->IsDeadSecondKey(keyCode, fOldKeyInfo.modifiers, fActiveDeadKey);
	
	if (!pressed) {
		r.InsetBySelf(1,1);
		if (secondDeadKey) {
			SetHighColor(255,0,0);
			StrokeRect(r);
			r.InsetBySelf(1,1);
			StrokeRect(r);
		} else if (deadKey>0) {
			SetHighColor(255,255,0);
			StrokeRect(r);
			r.InsetBySelf(1,1);
			StrokeRect(r);
		} else {
			
			SetHighColor(64,64,64);
			StrokeRect(r);
			
			BeginLineArray(14);
			rgb_color color1 = {200,200,200};
			AddLine(BPoint(r.left, r.bottom-1), r.LeftTop(), color1);
			AddLine(r.LeftTop(), BPoint(r.left+3, r.top), color1);
			rgb_color color2 = {184,184,184};
			AddLine(BPoint(r.left+3, r.top), BPoint(r.left+6, r.top), color2);
			rgb_color color3 = {168,168,168};
			AddLine(BPoint(r.left+6, r.top), BPoint(r.left+9, r.top), color3);
			rgb_color color4 = {152,152,152};
			AddLine(BPoint(r.left+9, r.top), BPoint(r.right-1, r.top), color4);
			
			r.InsetBySelf(1,1);
			SetHighColor(255,255,255);
			StrokeRect(r);
			
			rgb_color color6 = {96,96,96};	
			AddLine(r.LeftBottom(), r.RightBottom(), color6);
			rgb_color color5 = {160,160,160};	
			AddLine(r.LeftBottom(), r.LeftBottom(), color5);
			rgb_color color7 = {64,64,64};
			AddLine(r.RightBottom(), BPoint(r.right, r.bottom-1), color7);
			AddLine(BPoint(r.right, r.bottom-1), BPoint(r.right, r.top-1), color6);	
			AddLine(BPoint(r.right, r.top-1), r.RightTop(), color5);
			rgb_color color8 = {255,255,255};
			AddLine(BPoint(r.left+1, r.bottom-1), BPoint(r.left+2, r.bottom-1), color8);
			AddLine(BPoint(r.left+2, r.bottom-1), BPoint(r.right-1, r.bottom-1), color1);
			AddLine(BPoint(r.right-1, r.bottom-1), BPoint(r.right-1, r.bottom-2), color5);
			AddLine(BPoint(r.right-1, r.bottom-2), BPoint(r.right-1, r.top+1), color1);
			EndLineArray();
		}
		
		r.InsetBySelf(1,1);
		r.bottom -= 1;
		BRect fillRect = r;
			
		if (!vertical) {
			int32 w1 = 4;
			int32 w2 = 3;
			if(fKeysRect[keyCode].Width() > 20) {
				w1 = 6;
				w2 = 6;
			}
			
			fillRect.right = fillRect.left + w1;	
			SetHighColor(152,152,152);
			FillRect(fillRect);
			fillRect.left += w1;
			fillRect.right = fillRect.left + w2;
			SetHighColor(168,168,168);
			FillRect(fillRect);
			fillRect.left += w2;
			fillRect.right = r.right-1;
			SetHighColor(184,184,184);
			FillRect(fillRect);
		} else {
			SetHighColor(200,200,200);
			fillRect.right -= 1;
			fillRect.bottom = fillRect.top + 2;
			FillRect(fillRect);
			SetHighColor(184,184,184);
			fillRect.OffsetBySelf(0,3);
			FillRect(fillRect);
			SetHighColor(168,168,168);
			fillRect.OffsetBySelf(0,3);
			FillRect(fillRect);
			SetHighColor(152,152,152);
			fillRect.OffsetBySelf(0,3);
			FillRect(fillRect);
		}
	} else {
		r.InsetBySelf(1,1);
		
		if (secondDeadKey) {
			SetHighColor(255,0,0);
			StrokeRect(r);
			r.InsetBySelf(1,1);
			StrokeRect(r);
		} else if (deadKey>0) {
			SetHighColor(255,255,0);
			StrokeRect(r);
			r.InsetBySelf(1,1);
			StrokeRect(r);
		} else {
			SetHighColor(48,48,48);
			StrokeRect(r);
			
			BeginLineArray(2);
			rgb_color color1 = {136,136,136};
			AddLine(BPoint(r.left+1, r.bottom), r.RightBottom(), color1);
			AddLine(r.RightBottom(), BPoint(r.right, r.top+1), color1);
			EndLineArray();
			
			r.InsetBySelf(1,1);
			SetHighColor(72,72,72);
			StrokeRect(r);
			
			BeginLineArray(4);
			rgb_color color2 = {48,48,48};
			AddLine(r.LeftTop(), r.LeftTop(), color2);
			rgb_color color3 = {152,152,152};
			AddLine(BPoint(r.left+1, r.bottom), r.RightBottom(), color3);
			AddLine(r.RightBottom(), r.RightTop(), color3);
			rgb_color color4 = {160,160,160};
			AddLine(r.RightTop(), r.RightTop(), color4);
			EndLineArray();
		}
		
		r.InsetBySelf(1,1);
		SetHighColor(112,112,112);
		FillRect(r);
		SetHighColor(136,136,136);
		StrokeLine(r.LeftTop(), r.LeftTop());
		
	}
	
	char *str;
	int32 numBytes;
	fCurrentMap->GetChars(keyCode, fOldKeyInfo.modifiers, &str, &numBytes);
	if (str) {
		bool hasGlyphs;
		if (deadKey>0) {
			delete str;
			hasGlyphs = true;
			switch (deadKey) {
				case 1: str = strdup("'"); break;
				case 2: str = strdup("`"); break;
				case 3: str = strdup("^"); break;
				case 4: str = strdup("\""); break;
				case 5: str = strdup("~"); break;
			}
		} else
			fCurrentFont.GetHasGlyphs(str, 1, &hasGlyphs);
		
		if (hasGlyphs) {
			SetFont(&fCurrentFont);
			SetDrawingMode(B_OP_COPY);
			SetHighColor(0,0,0);
			SetLowColor(184,184,184);
			BPoint point = fKeysRect[keyCode].LeftBottom();
			point.x += 4;
			point.y -= 5;
			DrawString(str, point);
			SetDrawingMode(B_OP_OVER);
		}
		delete str;
	}	
}


void
MapView::DrawBorder(BRect bRect)
{
	rgb_color gray = {80,80,80};
	rgb_color white = {255,255,255};
	
	BeginLineArray(4);
	AddLine(bRect.LeftTop(), bRect.LeftBottom(), gray);
	AddLine(bRect.LeftTop(), bRect.RightTop(), gray);
	AddLine(BPoint(bRect.left + 1, bRect.bottom), bRect.RightBottom(), white);
	AddLine(bRect.RightBottom(), BPoint(bRect.right, bRect.top + 1), white);
	EndLineArray();
}


void
MapView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case MENU_EDIT_UNDO:
			fTextView->Undo(be_clipboard);
			break;
		case MENU_EDIT_CUT:
			fTextView->Cut(be_clipboard);
			break;
		case MENU_EDIT_COPY:
			fTextView->Copy(be_clipboard);
			break;
		case MENU_EDIT_PASTE:
			fTextView->Paste(be_clipboard);
			break;
		case MENU_EDIT_CLEAR:
			fTextView->Clear();
			break;
		case MENU_EDIT_SELECT_ALL:
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
			
			//msg->PrintToStream();
			
			if ((msg->FindData("states", 'UBYT', (const void **)&states, &size)!=B_OK)
				|| (msg->FindInt32("modifiers", (int32 *)&info.modifiers)!=B_OK))
				break;
				
			if (fOldKeyInfo.modifiers != info.modifiers) {
				fOldKeyInfo.modifiers = info.modifiers;
				for (int8 i=0; i<16; i++) 
					fOldKeyInfo.key_states[i] = states[i];
				Invalidate();
			} else {
								
				int32 keyCode = -1;
				for (int8 i=0; i<16; i++)
					if (fOldKeyInfo.key_states[i] != states[i]) {
						uint8 stbits = fOldKeyInfo.key_states[i] ^ states[i];
						fOldKeyInfo.key_states[i] = states[i];
						for (int8 j=7; stbits; j--,stbits>>=1)
							if (stbits & 1) {
								keyCode = i*8 + j;
								DrawKey(keyCode);
							}
													
					}
				
				if (fActiveDeadKey) {
					
				}
				//
				
				
				if (keyCode<0) 
					for (int8 i=0; i<16; i++) {
						uint8 stbits = states[i];
						for (int8 j=7; stbits; j--,stbits>>=1)
							if (stbits & 1) {
								keyCode = i*8 + j;
								if (!fCurrentMap->IsModifierKey(keyCode)) {
									i = 16;
									break;
								}
							}			
					}
				
										
				if (Window()->IsActive()
					&& msg->what == B_KEY_DOWN) {
					fTextView->MakeFocus();
					char *str;
					int32 numBytes;
					if (fActiveDeadKey) {
						fCurrentMap->DeadKey(keyCode, fOldKeyInfo.modifiers, fActiveDeadKey, &str, &numBytes);
						if (numBytes>0) {
							fTextView->FakeKeyDown(str, numBytes);
						}
						fActiveDeadKey = 0;
						Invalidate();
					} else {		
						fCurrentMap->GetChars(keyCode, fOldKeyInfo.modifiers, &str, &numBytes);
						fActiveDeadKey = fCurrentMap->IsDeadKey(keyCode, fOldKeyInfo.modifiers);
						if (fActiveDeadKey)
							Invalidate();
						else if (numBytes>0) {
							fTextView->FakeKeyDown(str, numBytes);
						}
					}
					delete str;
					
				}
														
			}
			
			break;
		}
		default:
			BView::MessageReceived(msg);
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
	if(buttons & B_PRIMARY_MOUSE_BUTTON) {
		fCurrentMouseKey = 0;
		for (int32 i=0; i<128; i++) {
			if (fKeysRect[i].Contains(point)) {
				fCurrentMouseKey = i;
				DrawKey(fCurrentMouseKey);
				char *str = NULL;
				int32 numBytes;
				fCurrentMap->GetChars(fCurrentMouseKey, fOldKeyInfo.modifiers, &str, &numBytes);
				if (numBytes>0) {
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
	DrawKey(value);
}

void
MapView::MouseMoved(BPoint point, uint32 transit, const BMessage *msg)
{
	if (IsTracking()) {
		uint32 value = fCurrentMouseKey;
		for (int32 i=0; i<128; i++) {
			if (fKeysRect[i].Contains(point) && !fKeysRect[value].Contains(point)) {
				fCurrentMouseKey = i;
				DrawKey(value);
				DrawKey(fCurrentMouseKey);
				char *str = NULL;
				int32 numBytes;
				fCurrentMap->GetChars(fCurrentMouseKey, fOldKeyInfo.modifiers, &str, &numBytes);
				if (numBytes>0) {
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
};
