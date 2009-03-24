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
#ifndef KEYMAP_WINDOW_H
#define KEYMAP_WINDOW_H


#include <FilePanel.h>
#include <ListView.h>
#include <String.h>
#include <Window.h>

#include "Keymap.h"
#include "KeymapTextView.h"

class BMenuBar;
class KeyboardLayoutView;
class KeymapListItem;


class KeymapWindow : public BWindow {
public:
								KeymapWindow();
								~KeymapWindow();

	virtual	bool				QuitRequested();
	virtual void				MessageReceived(BMessage* message);

protected:
			BMenuBar*			_CreateMenu();
			BView*				_CreateMapLists();

			void				_UseKeymap();
			void				_RevertKeymap();
			void				_UpdateButtons();

			void 				_FillSystemMaps();
			void				_FillUserMaps();
			void				_SetListViewSize(BListView* listView);

			bool				_SelectCurrentMap(BListView *list);
			BString				_GetActiveKeymapName();

			BListView*			fSystemListView;
			BListView*			fUserListView;
			BButton*			fUseButton;
			BButton*			fRevertButton;
			//BMenu*			fFontMenu;
			KeyboardLayoutView*	fKeyboardLayoutView;

			Keymap				fCurrentMap;
			Keymap				fPreviousMap;
			Keymap				fAppliedMap;
			bool				fFirstTime;
			BString				fCurrentMapName;

			BFilePanel*			fOpenPanel;
			BFilePanel*			fSavePanel;
};

#endif	// KEYMAP_WINDOW_H
