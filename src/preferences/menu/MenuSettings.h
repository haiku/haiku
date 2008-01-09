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
		
		bool 		AltAsShortcut() const;
		void 		SetAltAsShortcut(bool altAsShortcut);	
		
		rgb_color	BackgroundColor() const;		
		rgb_color	PreviousBackgroundColor() const;
		rgb_color	DefaultBackgroundColor() const;
	

		void Revert();
		void ResetToDefaults();
		bool IsDefaultable();
		bool IsRevertable();
				
	private:
		MenuSettings();
		~MenuSettings();

		menu_info fPreviousSettings;
		menu_info fDefaultSettings;
		bool	  fPreviousAltAsShortcut;
		bool	  fDefaultAltAsShortcut;
};

#endif	// MENU_SETTINGS_H
