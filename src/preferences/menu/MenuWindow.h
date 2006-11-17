/*
 * Copyright 2002-2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors in chronological order:
 *		<unknown, please fill in who knows>
 */
#ifndef __MENU_WINDOW_H
#define __MENU_WINDOW_H


#include <Menu.h>
#include <Window.h>


class ColorWindow;
class BMenuItem;
class BBox;
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
		bool		revert;
		ColorWindow	*colorWindow;
		BMenuItem 	*toggleItem;
		BMenu		*menu;
		MenuBar		*menuBar;
		BBox		*menuView;
		BButton 	*revertButton;
		BButton 	*defaultButton;
};

#endif	// __MENU_WINDOW_H
