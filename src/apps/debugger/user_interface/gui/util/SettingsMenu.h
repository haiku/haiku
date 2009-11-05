/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SETTINGS_MENU_H
#define SETTINGS_MENU_H


#include <MenuItem.h>

#include <Referenceable.h>

#include "Settings.h"


class BoolSetting;
class OptionsSetting;


class SettingsMenu : public BReferenceable {
public:
								SettingsMenu();
	virtual						~SettingsMenu();

	virtual	status_t			AddToMenu(BMenu* menu, int32 index) = 0;
	virtual	void				RemoveFromMenu() = 0;

	virtual	void				PrepareToShow(BLooper* parentLooper) = 0;
	virtual	bool				Finish(BLooper* parentLooper, bool force) = 0;
};


class SettingMenuItem : public BMenuItem {
public:
								SettingMenuItem(Setting* setting,
									const char* label, BMessage* message,
									char shortcut = 0, uint32 modifiers = 0);
								SettingMenuItem(Setting* setting, BMenu* menu,
									BMessage* message = NULL);
	virtual						~SettingMenuItem();

			Setting*			GetSetting() const	{ return fSetting; }

	virtual	void				PrepareToShow(BLooper* parentLooper,
									BHandler* targetHandler,
									Settings* settings);
	virtual	bool				Finish(BLooper* parentLooper,
									BHandler* targetHandler, bool force);

	virtual	void				ItemSelected(Settings* settings);

protected:
			Setting*			fSetting;
};


class SettingsMenuImpl : public SettingsMenu, private BHandler {
public:
								SettingsMenuImpl(Settings* settings);
	virtual						~SettingsMenuImpl();

			bool				AddItem(SettingMenuItem* item);
									// takes over ownership
			bool				AddBoolItem(BoolSetting* setting);
			bool				AddOptionsItem(OptionsSetting* setting);

			BMenu*				Menu() const			{ return fMenu; }
	virtual	status_t			AddToMenu(BMenu* menu, int32 index);
	virtual	void				RemoveFromMenu();

	virtual	void				PrepareToShow(BLooper* parentLooper);
	virtual	bool				Finish(BLooper* parentLooper, bool force);

			Settings*			GetSettings() const		{ return fSettings; }

private:
	virtual	void				MessageReceived(BMessage* message);

private:
			class MenuItem;
			class BoolMenuItem;
			class OptionMenuItem;
			class OptionsMenuItem;

			typedef BObjectList<SettingMenuItem> MenuItemList;

private:
			SettingMenuItem*	_FindMenuItem(Setting* setting) const;

private:
			Settings*			fSettings;
			BMenu*				fMenu;
			MenuItemList		fMenuItems;
};


#endif	// SETTINGS_MENU_H
