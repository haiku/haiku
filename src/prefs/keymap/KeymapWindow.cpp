#include <Alert.h>
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Debug.h>
#include <GraphicsDefs.h>
#include <ListView.h>
#include <MenuItem.h>
#include <ScrollView.h>
#include <StringView.h>
#include <View.h>
#include "KeymapWindow.h"
#include "KeymapListItem.h"
#include "KeymapApplication.h"
#include "messages.h"

KeymapWindow::KeymapWindow( BRect frame )
	:	BWindow( frame, WINDOW_TITLE, B_TITLED_WINDOW,
			B_NOT_ZOOMABLE|B_NOT_RESIZABLE|B_ASYNCHRONOUS_CONTROLS )
{
	rgb_color	tempColor;
	BRect		bounds = Bounds();
	BMenuBar	*menubar;

	fApplication = (KeymapApplication*) be_app;
	fSelectedMap = fApplication->CurrentMap();
	
	// Add the menu bar
	menubar = AddMenuBar();

	// The view to hold all but the menu bar
	bounds.top = menubar->Bounds().bottom + 1;
	fPlaceholderView = new BView( bounds, "placeholderView", 
		B_FOLLOW_NONE, 0 );
	tempColor = ui_color( B_MENU_BACKGROUND_COLOR );
	fPlaceholderView->SetViewColor( tempColor );
	AddChild( fPlaceholderView );

	// Create the Maps box and contents
	AddMaps();
	
	// The 'Use' button
	bounds.Set( 527,200, 600,220 );
	fUseButton = new BButton( bounds, "useButton", "Use",
		new BMessage( USE_KEYMAP ));
	fPlaceholderView->AddChild( fUseButton );
	
	// The 'Revert' button
	bounds.Set( 442,200, 515,220 );
	fRevertButton = new BButton( bounds, "revertButton", "Revert",
		new BMessage( REVERT ));
	fPlaceholderView->AddChild( fRevertButton );

	fMapView = new MapView(BRect(149,29,601,209), "mapView");
	AddChild(fMapView);
	
	SetPulseRate(10000);
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
	menu = new BMenu( "Font" );
	int32 numFamilies = count_font_families(); 
	font_family family, current_family;
	font_style current_style; 
	uint32 flags;
	
	be_plain_font->GetFamilyAndStyle(&current_family, &current_style);
		
	for ( int32 i = 0; i < numFamilies; i++ )
		if ( get_font_family(i, &family, &flags) == B_OK ) {
			BMenuItem *item = 
				new BMenuItem(family, new BMessage( MENU_FONT_CHANGED));
			menu->AddItem(item);
			if(strcmp(family, current_family) == 0)
				item->SetMarked(true);
		}
	menubar->AddItem( menu );
	
	return menubar;
}


void 
KeymapWindow::AddMaps()
{
	// The Maps box
	BRect bounds = BRect(9,11,140,226);
	BBox *mapsBox = new BBox(bounds);
	mapsBox->SetLabel("Maps");
	fPlaceholderView->AddChild( mapsBox );

	// The System list
	BStringView *systemLabel = new BStringView(BRect(13,13,113,33), "system", "System");
	mapsBox->AddChild(systemLabel);
	
	bounds = BRect( 13,35, 103,105 );
	BList *entryList = fApplication->SystemMaps();
	fSystemListView = new BListView( bounds, "systemList" );
	BList *listItems = ListItemsFromEntryList( entryList );
	fSystemListView->AddList( listItems );
	mapsBox->AddChild( new BScrollView( "systemScrollList", fSystemListView,
		B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true ));
	fSystemListView->SetSelectionMessage( new BMessage( SYSTEM_MAP_SELECTED ));
	delete listItems;
	delete entryList;

	// The User list
	BStringView *userLabel = new BStringView(BRect(13,110,113,128), "user", "User");
	mapsBox->AddChild(userLabel);

	bounds = BRect(13,130,103,200);
	entryList = fApplication->UserMaps();
	fUserListView = new BListView( bounds, "userList" );
	// '(Current)'
	KeymapListItem *currentKeymapItem = ItemFromEntry( fApplication->CurrentMap() );
	if( currentKeymapItem != NULL )
		fUserListView->AddItem( currentKeymapItem );
	// Saved keymaps
	listItems = ListItemsFromEntryList( entryList );
	fUserListView->AddList( listItems );
	mapsBox->AddChild( new BScrollView( "systemScrollList", fUserListView,
		B_FOLLOW_LEFT | B_FOLLOW_TOP, 0, false, true ));
	fUserListView->SetSelectionMessage( new BMessage( USER_MAP_SELECTED ));
	delete listItems;
	delete entryList;
	
	
}


BList* 
KeymapWindow::ListItemsFromEntryList( BList * entryList)
{
	BEntry			*currentEntry;
	BList			*listItems;
	int				nrItems;
	#ifdef DEBUG
		char	name[B_FILE_NAME_LENGTH];
	#endif //DEBUG

	listItems = new BList();
	nrItems = entryList->CountItems();
	for( int index=0; index<nrItems; index++ ) {
		currentEntry = (BEntry*)entryList->ItemAt( index );
		listItems->AddItem( new KeymapListItem( currentEntry ));
		
		#ifdef DEBUG
			currentEntry->GetName( name );
			printf("New list item: %s\n",name);
		#endif //DEBUG
	}

	return listItems;
}


KeymapListItem* 
KeymapWindow::ItemFromEntry( BEntry *entry )
{
	KeymapListItem	*item;

	if( entry->Exists() ) {
		item = new KeymapListItem( entry );
		item->SetText( "(Current)" );
	}
	else
		item = NULL;

	return item;
}


bool 
KeymapWindow::QuitRequested()
{
	be_app->PostMessage( B_QUIT_REQUESTED );
	return true;
}


void 
KeymapWindow::MessageReceived( BMessage* message )
{
	switch( message->what ) {
		case MENU_FILE_OPEN:
			break;
		case MENU_FILE_SAVE:
			break;
		case MENU_FILE_SAVE_AS:
			break;
		case MENU_EDIT_UNDO:
			break;
		case MENU_EDIT_CUT:
			break;
		case MENU_EDIT_COPY:
			break;
		case MENU_EDIT_PASTE:
			break;
		case MENU_EDIT_CLEAR:
			break;
		case MENU_EDIT_SELECT_ALL:
			break;
		case SYSTEM_MAP_SELECTED:
			HandleSystemMapSelected( message );
			break;
		case USER_MAP_SELECTED:
			HandleUserMapSelected( message );
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
KeymapWindow::HandleSystemMapSelected( BMessage *selectionMessage )
{
	HandleMapSelected( selectionMessage, fSystemListView, fUserListView );
}


void 
KeymapWindow::HandleUserMapSelected( BMessage *selectionMessage )
{
	HandleMapSelected( selectionMessage, fUserListView, fSystemListView );
}


void 
KeymapWindow::HandleMapSelected( BMessage *selectionMessage,
	BListView * selectedView, BListView * otherView )
{
	int32	index;
	KeymapListItem	*keymapListItem;

	// Anything selected?
	index = selectedView->CurrentSelection( 0 );
	if( index < 0 ) {
		#if DEBUG
			printf("index<0; HandleMapSelected ends here.\n");
	
		#endif //DEBUG
		return;
	}
	
	// Store selected map in fSelectedMap
	keymapListItem = (KeymapListItem*)selectedView->ItemAt( index );
	if( keymapListItem == NULL )
		return;
	fSelectedMap = keymapListItem->KeymapEntry();
	#if DEBUG
		char	name[B_FILE_NAME_LENGTH];
		fSelectedMap->GetName( name );
		printf("fSelectedMap has been set to %s\n",name);
	#endif //DEBUG

	// Deselect item in other BListView
	otherView->DeselectAll();
}


void 
KeymapWindow::UseKeymap()
{
	if( fSelectedMap != NULL )
		fApplication->UseKeymap( fSelectedMap );
	else {
		// There is no keymap selected!
		BAlert	*alert;
		alert = new BAlert( "w>noKeymap", "No keymap has been selected", "Bummer",
			NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT );
		alert->Go();
	}
}


MapView::MapView(BRect rect, const char *name)
	: BView(rect, name, B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW | B_PULSE_NEEDED)
{

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
#define isPressed(i) (fOldKeyInfo.key_states[i>>3] & (1 << (7 - i%8)) ) 
	
	// Esc key
	BRect keyRect = BRect(11,50,29,68);
	DrawKey(keyRect, isPressed(i));
	
	DrawBorder(keyRect.InsetByCopy(-1, -1));
	
	// Fx keys
	i++;
	keyRect.OffsetBySelf(36,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	
	DrawBorder(BRect(keyRect.left - 55, 49, keyRect.right + 1, 69));
	
	i++;
	keyRect.OffsetBySelf(27,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	
	DrawBorder(BRect(keyRect.left - 55, 49, keyRect.right + 1, 69));
	
	i++;
	keyRect.OffsetBySelf(27,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	
	DrawBorder(BRect(keyRect.left - 55, 49, keyRect.right + 1, 69));
	
	// Pause, PrintScreen, ...
	i++;
	keyRect.OffsetBySelf(35,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	
	DrawBorder(BRect(keyRect.left - 37, 49, keyRect.right + 1, 69));
	
	// 1st line : numbers and backspace
	i++;
	keyRect = BRect(11,78,29,96);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	keyRect.right += 18;
	DrawKey(keyRect, isPressed(i));
	keyRect.left += 18;
	
	// Insert, pg up ...
	i++;
	keyRect.OffsetBySelf(35,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(0x20));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(0x21));
	
	DrawBorder(BRect(keyRect.left - 37, 77, keyRect.right + 1, 115));
	
	// 2nd line : tab and azerty ...
	i = 0x26;
	keyRect = BRect(11,96,38,114);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(27,0);
	keyRect.right -= 9;
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	keyRect.right += 9;
	DrawKey(keyRect, isPressed(i));
	keyRect.left += 9;
	
	// Suppr, pg down ...
	i++;
	keyRect.OffsetBySelf(35,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	
	// 3rd line : caps and qsdfg ...
	i = 0x3b;
	keyRect = BRect(11,114,47,132);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(36,0);
	keyRect.right -= 18;
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	keyRect.right += 18;
	DrawKey(keyRect, isPressed(i));
	keyRect.left += 18;
	
	// 4th line : shift and wxcv ...
	i = 0x4b;
	keyRect = BRect(11,132,56,150);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(45,0);
	keyRect.right -= 27;
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	keyRect.right += 27;
	DrawKey(keyRect, isPressed(i));
	keyRect.left += 27;
	
	//5th line : Ctrl, Alt, Space ...
	i = 0x5c;
	keyRect = BRect(11,150,38,168);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(27,0);
	keyRect.OffsetBySelf(26,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(27,0);
	keyRect.right += 92;
	DrawKey(keyRect, isPressed(i), true);
	i++;
	keyRect.right -= 92;
	keyRect.OffsetBySelf(92,0);
	keyRect.OffsetBySelf(27,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(26,0);
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	
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
	i++;
	keyRect = BRect(298,150,316,168);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i = 0x57;
	keyRect.OffsetBySelf(-18,-18);
	DrawKey(keyRect, isPressed(i));
	
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
	keyRect = BRect(369,78,387,96);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i = 0x37;
	keyRect.OffsetBySelf(-54, 18);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	keyRect.bottom += 18;
	DrawKey(keyRect, isPressed(i));
	i = 0x48;
	keyRect.bottom -= 18;
	keyRect.OffsetBySelf(-54, 18);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i = 0x58;
	keyRect.OffsetBySelf(-36, 18);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.OffsetBySelf(18,0);
	keyRect.bottom += 18;
	DrawKey(keyRect, isPressed(i));
	i = 0x64;
	keyRect.bottom -= 18;
	keyRect.OffsetBySelf(-54, 18);
	keyRect.right += 18;
	DrawKey(keyRect, isPressed(i));
	i++;
	keyRect.right -= 18;
	keyRect.OffsetBySelf(36,0);
	DrawKey(keyRect, isPressed(i));
	
	DrawBorder(BRect(keyRect.left - 37, 77, keyRect.right + 19, 169));
	
	// lights
#define isLighted(i) (fOldKeyInfo.modifiers & i) 

	DrawBorder(BRect(368, 49, 442, 69));
	
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
	DrawString("num", BPoint(lightRect.left, 65));
		
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
	DrawString("caps", BPoint(lightRect.left-1, 65));
	
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
	DrawString("scroll", BPoint(lightRect.left-2, 65));
	
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
	
	BView::Draw(rect);
}


void
MapView::DrawKey(BRect rect, bool pressed, bool vertical)
{
	BRect r = rect;
	SetHighColor(0,0,0);
	StrokeRect(r);
	
	if(!pressed) {
		r.InsetBySelf(1,1);
		SetHighColor(64,64,64);
		StrokeRect(r);
		SetHighColor(200,200,200);
		StrokeLine(BPoint(r.left, r.bottom-1), r.LeftTop());
		StrokeLine(BPoint(r.left+3, r.top));
		SetHighColor(184,184,184);
		StrokeLine(BPoint(r.left+6, r.top));
		SetHighColor(168,168,168);
		StrokeLine(BPoint(r.left+9, r.top));
		SetHighColor(152,152,152);
		StrokeLine(BPoint(r.right-1, r.top));
		
		r.InsetBySelf(1,1);
		SetHighColor(255,255,255);
		StrokeRect(r);
		SetHighColor(160,160,160);
		StrokeLine(r.LeftBottom(), r.LeftBottom());
		SetHighColor(96,96,96);
		StrokeLine(r.RightBottom());
		SetHighColor(64,64,64);
		StrokeLine(BPoint(r.right, r.bottom-1));
		SetHighColor(96,96,96);
		StrokeLine(BPoint(r.right, r.top-1));
		SetHighColor(160,160,160);
		StrokeLine(r.RightTop());
	
		SetHighColor(255,255,255);
		StrokeLine(BPoint(r.left+1, r.bottom-1), BPoint(r.left+2, r.bottom-1));
		SetHighColor(200,200,200);
		StrokeLine(BPoint(r.right-1, r.bottom-1));
		SetHighColor(160,160,160);
		StrokeLine(BPoint(r.right-1, r.bottom-2));
		SetHighColor(200,200,200);
		StrokeLine(BPoint(r.right-1, r.top+1));
		
		r.InsetBySelf(1,1);
		r.bottom -= 1;
		BRect fillRect = r;
			
		if(!vertical) {
			int32 w1 = 4;
			int32 w2 = 3;
			if(rect.Width() > 20) {
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
		SetHighColor(48,48,48);
		StrokeRect(r);
		SetHighColor(136,136,136);
		StrokeLine(BPoint(r.left+1, r.bottom), r.RightBottom());
		StrokeLine(BPoint(r.right, r.top+1));
		
		r.InsetBySelf(1,1);
		SetHighColor(72,72,72);
		StrokeRect(r);
		SetHighColor(48,48,48);
		StrokeLine(r.LeftTop(), r.LeftTop());
		SetHighColor(152,152,152);
		StrokeLine(BPoint(r.left+1, r.bottom), r.RightBottom());
		StrokeLine(r.RightTop());
		SetHighColor(160,160,160);
		StrokeLine(r.RightTop());
		
		r.InsetBySelf(1,1);
		SetHighColor(112,112,112);
		FillRect(r);
		SetHighColor(136,136,136);
		StrokeLine(r.LeftTop(), r.LeftTop());
		
	}
}


void
MapView::DrawBorder(BRect borderRect)
{
	SetHighColor(80,80,80);
	StrokeRect(borderRect);
	SetHighColor(255,255,255);
	StrokeLine(BPoint(borderRect.left + 1, borderRect.bottom), borderRect.RightBottom());
	StrokeLine(BPoint(borderRect.right, borderRect.top + 1));
}

void
MapView::Pulse()
{
	key_info info;
	bool need_update = false;
	get_key_info(&info);

	if (fOldKeyInfo.modifiers != info.modifiers) {
		need_update = true;
	}
		
	for (int8 i=0; i<16; i++)
		if (fOldKeyInfo.key_states[i] != info.key_states[i]) {
			need_update = true;
			break;
		}
		
	if (need_update) {
		fOldKeyInfo.modifiers = info.modifiers;
		for (int8 j=0; j<16; j++) 
			fOldKeyInfo.key_states[j] = info.key_states[j];
		Invalidate();
	}	
}
