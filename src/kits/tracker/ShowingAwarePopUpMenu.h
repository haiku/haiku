/*
 * Copyright 2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _SHOWING_AWARE_POPUP_MENU_H
#define _SHOWING_AWARE_POPUP_MENU_H

#include <PopUpMenu.h>


namespace BPrivate {

class BShowingAwarePopUpMenu : public BPopUpMenu {
public:
	BShowingAwarePopUpMenu(const char *title, bool radioMode = true,
		bool autoRename = true, menu_layout layout = B_ITEMS_IN_COLUMN);
	virtual ~BShowingAwarePopUpMenu();

	virtual	void AttachedToWindow();
	virtual	void DetachedFromWindow();

	const bool IsShowing() const;

private:
	bool fIsShowing;
};

} // namespace BPrivate

using namespace BPrivate;

inline const bool
BShowingAwarePopUpMenu::IsShowing() const
{
	return fIsShowing;
}

#endif
