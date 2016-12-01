/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "SettingsMenu.h"

#include <new>

#include <Looper.h>
#include <Menu.h>

#include <AutoDeleter.h>

#include "AppMessageCodes.h"
#include "SettingsDescription.h"


// #pragma mark - SettingsMenu


SettingsMenu::SettingsMenu()
{
}


SettingsMenu::~SettingsMenu()
{
}


// #pragma mark - SettingMenuItem


SettingMenuItem::SettingMenuItem(Setting* setting, const char* label,
	BMessage* message, char shortcut, uint32 modifiers)
	:
	BMenuItem(label, message, shortcut, modifiers),
	fSetting(setting)
{
	fSetting->AcquireReference();
}


SettingMenuItem::SettingMenuItem(Setting* setting, BMenu* menu,
	BMessage* message)
	:
	BMenuItem(menu, message),
	fSetting(setting)
{
	fSetting->AcquireReference();
}


SettingMenuItem::~SettingMenuItem()
{
	fSetting->ReleaseReference();
}


void
SettingMenuItem::PrepareToShow(BLooper* parentLooper, BHandler* targetHandler,
		Settings* settings)
{
}


bool
SettingMenuItem::Finish(BLooper* parentLooper, BHandler* targetHandler,
	bool force)
{
	return false;
}


void
SettingMenuItem::ItemSelected(Settings* settings)
{
}


// #pragma mark - BoolMenuItem


class SettingsMenuImpl::MenuItem : public SettingMenuItem {
public:
	MenuItem(Setting* setting,
		const char* label, BMessage* message,
		char shortcut = 0, uint32 modifiers = 0)
		:
		SettingMenuItem(setting, label, message, shortcut, modifiers)
	{
	}

	MenuItem(Setting* setting, BMenu* menu, BMessage* message = NULL)
		:
		SettingMenuItem(setting, menu, message)
	{
	}

	virtual void PrepareToShow(BLooper* parentLooper, BHandler* targetHandler,
		Settings* settings)
	{
		BMessage* message = new BMessage(
			MSG_SETTINGS_MENU_IMPL_ITEM_SELECTED);
		if (message == NULL)
			return;
		message->AddPointer("setting", fSetting);

		SetMessage(message);
		SetTarget(targetHandler);
	}
};


class SettingsMenuImpl::BoolMenuItem : public MenuItem {
public:
	BoolMenuItem(BoolSetting* setting)
		:
		MenuItem(setting, setting->Name(), NULL)
	{
	}
};


// #pragma mark - OptionMenuItem


class SettingsMenuImpl::OptionMenuItem : public BMenuItem {
public:
	OptionMenuItem(SettingsOption* option)
		:
		BMenuItem(option->Name(), NULL),
		fOption(option)
	{
	}

	SettingsOption* Option() const
	{
		return fOption;
	}

	void PrepareToShow(BLooper* parentLooper, BHandler* targetHandler,
		Settings* settings, OptionsSetting* setting)
	{
		BMessage* message = new BMessage(
			MSG_SETTINGS_MENU_IMPL_OPTION_ITEM_SELECTED);
		if (message == NULL)
			return;
		message->AddPointer("setting", static_cast<Setting*>(setting));
		message->AddPointer("option", fOption);

		SetMessage(message);
		SetTarget(targetHandler);
	}

private:
	SettingsOption*	fOption;
};


// #pragma mark - OptionsMenuItem


class SettingsMenuImpl::OptionsMenuItem : public MenuItem {
public:
	OptionsMenuItem(OptionsSetting* setting, BMenu* menu)
		:
		MenuItem(setting, menu)
	{
	}

	virtual void PrepareToShow(BLooper* parentLooper, BHandler* targetHandler,
		Settings* settings)
	{
		SettingsOption* selectedOption = settings->OptionValue(
			dynamic_cast<OptionsSetting*>(GetSetting()));

		for (int32 i = 0; BMenuItem* item = Submenu()->ItemAt(i); i++) {
			OptionMenuItem* optionItem = dynamic_cast<OptionMenuItem*>(item);
			if (optionItem != NULL)
				optionItem->PrepareToShow(parentLooper, targetHandler,
					settings, dynamic_cast<OptionsSetting*>(GetSetting()));

			optionItem->SetMarked(optionItem->Option() == selectedOption);
		}
	}

	void OptionItemSelected(Settings* settings, SettingsOption* option)
	{
		settings->SetValue(fSetting,
			BVariant(option->ID(), B_VARIANT_DONT_COPY_DATA));
	}
};


// #pragma mark - SettingsMenuImpl


SettingsMenuImpl::SettingsMenuImpl(Settings* settings)
	:
	fSettings(settings),
	fMenu(NULL)
{
	fSettings->AcquireReference();
}


SettingsMenuImpl::~SettingsMenuImpl()
{
	RemoveFromMenu();
	fSettings->ReleaseReference();
}


bool
SettingsMenuImpl::AddItem(SettingMenuItem* item)
{
	return fMenuItems.AddItem(item);
}


bool
SettingsMenuImpl::AddBoolItem(BoolSetting* setting)
{
	SettingMenuItem* item = new(std::nothrow) BoolMenuItem(setting);
	if (item == NULL || !AddItem(item)) {
		delete item;
		return false;
	}

	return true;
}


bool
SettingsMenuImpl::AddOptionsItem(OptionsSetting* setting)
{
	// create the submenu
	BMenu* menu = new(std::nothrow) BMenu(setting->Name());
	if (menu == NULL)
		return false;

	// create the menu item
	OptionsMenuItem* item = new(std::nothrow) OptionsMenuItem(setting, menu);
	if (item == NULL) {
		delete menu;
		return false;
	}
	ObjectDeleter<OptionsMenuItem> itemDeleter(item);

	// create sub menu items for the options
	int32 count = setting->CountOptions();
	for (int32 i = 0; i < count; i++) {
		SettingsOption* option = setting->OptionAt(i);
		BMenuItem* optionItem = new(std::nothrow) OptionMenuItem(option);
		if (optionItem == NULL || !menu->AddItem(optionItem)) {
			delete optionItem;
			return false;
		}
	}

	if (!AddItem(item))
		return false;

	itemDeleter.Detach();
	return true;
}


status_t
SettingsMenuImpl::AddToMenu(BMenu* menu, int32 index)
{
	if (fMenu != NULL)
		return B_BAD_VALUE;

	fMenu = menu;

	for (int32 i = 0; SettingMenuItem* item = fMenuItems.ItemAt(i); i++) {
		if (!fMenu->AddItem(item, index + i)) {
			for (i--; i >= 0; i--)
				fMenu->RemoveItem(fMenuItems.ItemAt(i));

			menu = NULL;
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}


void
SettingsMenuImpl::RemoveFromMenu()
{
	if (fMenu == NULL)
		return;

	for (int32 i = 0; SettingMenuItem* item = fMenuItems.ItemAt(i); i++)
		fMenu->RemoveItem(item);

	fMenu = NULL;
}


void
SettingsMenuImpl::PrepareToShow(BLooper* parentLooper)
{
	parentLooper->AddHandler(this);

	for (int32 i = 0; SettingMenuItem* item = fMenuItems.ItemAt(i); i++)
		item->PrepareToShow(parentLooper, this, fSettings);
}


bool
SettingsMenuImpl::Finish(BLooper* parentLooper, bool force)
{
	bool stillActive = false;

	for (int32 i = 0; SettingMenuItem* item = fMenuItems.ItemAt(i); i++)
		stillActive |= item->Finish(parentLooper, this, force);

	parentLooper->RemoveHandler(this);

	return stillActive;
}


void
SettingsMenuImpl::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SETTINGS_MENU_IMPL_ITEM_SELECTED:
		{
			Setting* setting;
			if (message->FindPointer("setting", (void**)&setting) != B_OK)
				break;

			if (SettingMenuItem* item = _FindMenuItem(setting))
				item->ItemSelected(fSettings);

			break;
		}

		case MSG_SETTINGS_MENU_IMPL_OPTION_ITEM_SELECTED:
		{
			Setting* setting;
			SettingsOption* option;
			if (message->FindPointer("setting", (void**)&setting) != B_OK
				|| message->FindPointer("option", (void**)&option) != B_OK) {
				break;
			}

			// get the options setting item
			OptionsMenuItem* optionsItem = dynamic_cast<OptionsMenuItem*>(
				_FindMenuItem(setting));
			if (optionsItem == NULL)
				break;

			optionsItem->OptionItemSelected(fSettings, option);
			break;
		}

		default:
			BHandler::MessageReceived(message);
			break;
	}
}


SettingMenuItem*
SettingsMenuImpl::_FindMenuItem(Setting* setting) const
{
	for (int32 i = 0; SettingMenuItem* item = fMenuItems.ItemAt(i); i++) {
		if (item->GetSetting() == setting)
			return item;
	}

	return NULL;
}
