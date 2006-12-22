/*
 * Copyright 2002-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		<unknown, please fill in who knows>
 */
#ifndef FONT_MENU_H
#define FONT_MENU_H


#include "AutoSettingsMenu.h"


class FontSizeMenu : public AutoSettingsMenu {
	public:
		FontSizeMenu();
		virtual void AttachedToWindow();
};
	

class FontMenu : public AutoSettingsMenu {
	public:
		FontMenu();

		virtual void AttachedToWindow();
		void GetFonts();

		status_t PlaceCheckMarkOnFont(font_family family, font_style style);
		void ClearAllMarkedItems();
};

#endif	// FONT_MENU_H
