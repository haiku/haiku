/*
 * Copyright 2002-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 */

#ifndef AUTO_SETTINGS_MENU_H
#define AUTO_SETTINGS_MENU_H


#include <Menu.h>


class AutoSettingsMenu : public BMenu {
	public:
		AutoSettingsMenu(const char* name, menu_layout layout);

		virtual void AttachedToWindow();
};

#endif	// AUTO_SETTINGS_MENU_H

