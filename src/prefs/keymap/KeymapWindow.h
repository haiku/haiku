// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
//
//	Copyright (c) 2004, Haiku
//
//  This software is part of the Haiku distribution and is covered 
//  by the Haiku license.
//
//
//  File:        KeymapWindow.h
//  Author:      Sandor Vroemisse, Jérôme Duval
//  Description: Keymap Preferences
//  Created :    July 12, 2004
// 
// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~

#ifndef KEYMAP_WINDOW_H
#define KEYMAP_WINDOW_H

#include <interface/Window.h>
#include <support/List.h>
#include <interface/MenuBar.h>
#include "Keymap.h"

#define WINDOW_TITLE				"Keymap"
#define WINDOW_LEFT_TOP_POSITION	BPoint( 80, 25 )
#define WINDOW_DIMENSIONS			BRect( 0,0, 612,256 )

class KeymapListItem;
class KeymapApplication;

class MapView : public BView
{
public:
	MapView(BRect rect, const char *name, Keymap *keymap);
	void Draw(BRect rect);
	void DrawKey(int32 keyCode);
	void DrawBorder(BRect borderRect);
	void AttachedToWindow();
	void KeyDown(const char* bytes, int32 numBytes);
	void KeyUp(const char* bytes, int32 numBytes);
	void MessageReceived(BMessage *msg);
	void SetFontFamily(const font_family family) {fCurrentFont.SetFamilyAndStyle(family, NULL); };
private:	
	key_info fOldKeyInfo;
	BRect fKeysRect[128];
	bool fKeysVertical[128];
	uint8 fKeyState[16];
	BFont fCurrentFont;
	
	Keymap				*fCurrentMap;
	BTextView 			*fTextView;
};


class KeymapWindow : public BWindow {
public:
			KeymapWindow( BRect frame );
	bool	QuitRequested();
	void	MessageReceived( BMessage* message );

protected:
	BListView			*fSystemListView;
	BListView			*fUserListView;
	// the map that's currently highlighted
	BButton				*fUseButton;
	BButton				*fRevertButton;
	BMenu				*fFontMenu;
	
	MapView				*fMapView;

	BMenuBar			*AddMenuBar();
	void				AddMaps(BView *placeholderView);
	//KeymapListItem* 	ItemFromEntry( BEntry *entry );
	void				UseKeymap();
	
	void FillSystemMaps();
	void FillUserMaps();
	
	BEntry* 			CurrentMap();
		
	Keymap				fCurrentMap;
};

#endif // KEYMAP_WINDOW_H
