/*
 * Copyright 2002-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		<unknown, please fill in who knows>
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 */

#ifndef MENU_WINDOW_H
#define MENU_WINDOW_H

#include "MenuSettings.h"
#include <Menu.h>
#include <Window.h>


class ColorWindow;
class BButton;
class MenuBar;
class MenuWindow : public BWindow {
	public:
		MenuWindow(BRect frame);		
		
		virtual void MessageReceived(BMessage *message);
		virtual bool QuitRequested();
		
	private:		
		void _UpdateAll();
		
	private:	
		ColorWindow*	fColorWindow;
		MenuBar*		fMenuBar;
		BButton*		fDefaultsButton;
		BButton*		fRevertButton;
		MenuSettings*	fSettings;
};

#endif	// MENU_WINDOW_H
