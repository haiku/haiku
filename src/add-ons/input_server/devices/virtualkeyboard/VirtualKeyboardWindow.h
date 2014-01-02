/*
 * Copyright 2014 Freeman Lou <freemanlou2430@yahoo.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef VIRTUAL_KEYBOARD_WINDOW_H
#define VIRTUAL_KEYBOARD_WINDOW_H

#include <Window.h>

#include <vector.h>

#include "Keymap.h"

class KeyboardLayoutView;
class Keymap;
class BDirectory;
class BListView;
class BMenu;

class VirtualKeyboardWindow : public BWindow{
public:
							VirtualKeyboardWindow();
		virtual void		MessageReceived(BMessage* message);			
							
private:
		KeyboardLayoutView* fKeyboardView;
		BListView*			fMapListView;
		BMenu*				fFontMenu;
		BMenu*				fLayoutMenu;
		Keymap				fCurrentKeymap;
		
private:
				void		_LoadLayouts(BMenu* menu);
				void		_LoadLayoutMenu(BMenu* menu, BDirectory directory);
				void		_LoadMaps();
				void		_LoadFonts();
};

#endif // VIRTUAL_KEYBOARD_WINDOW_H
