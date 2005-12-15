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

#include <Control.h>
#include <FilePanel.h>
#include <ListView.h>
#include <Window.h>
#include <MenuBar.h>
#include "KeymapTextView.h"
#include "Keymap.h"

class KeymapListItem;

class MapView : public BControl
{
public:
	MapView(BRect rect, const char *name, Keymap *keymap);
	void Draw(BRect rect);
	void DrawKey(uint32 keyCode);
	void DrawBorder(BRect borderRect);
	void AttachedToWindow();
	void KeyDown(const char* bytes, int32 numBytes);
	void KeyUp(const char* bytes, int32 numBytes);
	void MessageReceived(BMessage *msg);
	void SetFontFamily(const font_family family);
	void MouseDown(BPoint point);
	void MouseUp(BPoint point);
	void MouseMoved(BPoint point, uint32 transit, const BMessage *msg);
	void InvalidateKeys();
	void DrawLocks();
private:	
	key_info fOldKeyInfo;
	BRect fKeysRect[128];
	bool fKeysVertical[128];
	bool fKeysToDraw[128];
	uint8 fKeyState[16];
	BFont fCurrentFont;
	
	Keymap				*fCurrentMap;
	KeymapTextView		*fTextView;
	uint32 fCurrentMouseKey;
	uint8 fActiveDeadKey;		// 0 : none, 1 : acute , ...
};


class KeymapWindow : public BWindow {
public:
			KeymapWindow();
			~KeymapWindow();
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
	void				UseKeymap();
	
	void 				FillSystemMaps();
	void 				FillUserMaps();
	
	BEntry* 			CurrentMap();
		
	Keymap				fCurrentMap;
	
	BFilePanel 			*fOpenPanel;		// the file panel to open
	BFilePanel 			*fSavePanel;		// the file panel to save
};

#endif // KEYMAP_WINDOW_H
