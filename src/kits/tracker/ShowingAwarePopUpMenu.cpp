/*
 * Copyright 2008, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexandre Deckner, alex@zappotek.com
 */

#include "ShowingAwarePopUpMenu.h"


BShowingAwarePopUpMenu::BShowingAwarePopUpMenu(const char *title, bool radioMode,
	bool autoRename, menu_layout layout)
	:	BPopUpMenu(title, radioMode, autoRename, layout),
		fIsShowing(false)
{
}


BShowingAwarePopUpMenu::~BShowingAwarePopUpMenu()
{
}


void
BShowingAwarePopUpMenu::AttachedToWindow()
{
	fIsShowing = true;
	BPopUpMenu::AttachedToWindow();
}


void
BShowingAwarePopUpMenu::DetachedFromWindow()
{
	fIsShowing = false;
}
