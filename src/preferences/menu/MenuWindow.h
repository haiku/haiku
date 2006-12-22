/*
 * Copyright 2002-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		<unknown, please fill in who knows>
 */
#ifndef MENU_WINDOW_H
#define MENU_WINDOW_H


#include <Menu.h>
#include <Window.h>


class ColorWindow;
class BButton;
class MenuBar;


class MenuWindow : public BWindow {
	public:
		MenuWindow(BRect frame);
		virtual ~MenuWindow();

		virtual void MessageReceived(BMessage *message);
		virtual bool QuitRequested();

		void Update();
		void Defaults();

	private:	
		bool		fRevert;
		ColorWindow* fColorWindow;
		MenuBar*	fMenuBar;
		BButton*	fRevertButton;
};

#endif	// MENU_WINDOW_H
