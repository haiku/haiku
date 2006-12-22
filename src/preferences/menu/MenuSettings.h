/*
 * Copyright 2002-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		<unknown, please fill in who knows>
 */
#ifndef MENU_SETTINGS_H
#define MENU_SETTINGS_H


#include <Menu.h>


class MenuSettings {
	public:
		static MenuSettings *GetInstance();

		void Get(menu_info &info) const;
		void Set(menu_info &info);

		void Revert();
		void ResetToDefaults();

	private:
		MenuSettings();
		~MenuSettings();

		menu_info fPreviousSettings;
		menu_info fDefaultSettings;	
};

#endif	// MENU_SETTINGS_H
