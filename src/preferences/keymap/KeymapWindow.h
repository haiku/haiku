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


class BMenu;
class BMenuBar;
class BMenuField;
class BTextControl;
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
			void				_AddKeyboardLayouts(BMenu* menu);
			void				_AddKeyboardLayoutMenu(BMenu* menu,
									BDirectory directory);
			status_t			_SetKeyboardLayout(const char* path);
			void				_MarkKeyboardLayoutItem(const char* path,
									BMenu* menu);

			void				_UpdateSwitchShortcutButton();
			void				_UpdateButtons();
			void				_SwitchShortcutKeys();

			void				_UseKeymap();
			void				_RevertKeymap();

			BMenuField*			_CreateDeadKeyMenuField();
			void				_UpdateDeadKeyMenu();

			void 				_FillSystemMaps();
			void				_FillUserMaps();
			void				_SetListViewSize(BListView* listView);

			status_t			_GetCurrentKeymap(entry_ref& ref);
			BString				_GetActiveKeymapName();
			bool				_SelectCurrentMap(BListView *list);
			void				_SelectCurrentMap();

			status_t			_GetSettings(BFile& file, int mode) const;
			status_t			_LoadSettings(BRect& frame,
									BString& keyboardLayout);
			status_t			_SaveSettings();
			BPath				_GetMarkedKeyboardLayoutPath(BMenu* menu);

private:
			BListView*			fSystemListView;
			BListView*			fUserListView;
			BButton*			fRevertButton;
			BMenu*				fLayoutMenu;
			BMenu*				fFontMenu;
			KeyboardLayoutView*	fKeyboardLayoutView;
			BTextControl*		fTextControl;
			BButton*			fSwitchShortcutsButton;
			BMenu*				fAcuteMenu;
			BMenu*				fCircumflexMenu;
			BMenu*				fDiaeresisMenu;
			BMenu*				fGraveMenu;
			BMenu*				fTildeMenu;

			Keymap				fCurrentMap;
			Keymap				fPreviousMap;
			Keymap				fAppliedMap;
			BString				fCurrentMapName;

			BFilePanel*			fOpenPanel;
			BFilePanel*			fSavePanel;
};

#endif	// KEYMAP_WINDOW_H
