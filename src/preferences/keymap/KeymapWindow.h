/*
 * Copyright 2004-2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Sandor Vroemisse
 *		Jérôme Duval
 *		Alexandre Deckner, alex@zappotek.com
 */

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
class BBitmap;

class MapView : public BControl
{
public:
	MapView(BRect rect, const char *name, Keymap *keymap);
	~MapView();
	
	void Draw(BRect rect);
	void AttachedToWindow();
	void KeyDown(const char* bytes, int32 numBytes);
	void KeyUp(const char* bytes, int32 numBytes);
	void MessageReceived(BMessage *msg);
	void SetFontFamily(const font_family family);
	void MouseDown(BPoint point);
	void MouseUp(BPoint point);
	void MouseMoved(BPoint point, uint32 transit, const BMessage *msg);
		
private:	
	void _InvalidateKey(uint32 keyCode);	
	void _InvalidateKeys();
	void _DrawKey(uint32 keyCode);
	void _DrawBackground();
	void _DrawKeysBackground();
	void _DrawLocksBackground();
	void _DrawBorder(BView *view, const BRect &borderRect);
	void _DrawLocksLights();	
	void _InitOffscreen();
	
	BBitmap		*fBitmap;
	BView		*fOffscreenView;
	
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
