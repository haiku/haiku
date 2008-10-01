/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
 * ui_settings ThemesAddon class
 */

#include <BeBuild.h>
#ifdef B_BEOS_VERSION_DANO

#include <Alert.h>
#include <Application.h>
#include <InterfaceDefs.h>
#include <Entry.h>
#include <Font.h>
#include <Message.h>
#include <Roster.h>

#include <stdio.h>
#include <string.h>

#include "ThemesAddon.h"
#include "UITheme.h"

#ifdef SINGLE_BINARY
#define instantiate_themes_addon instantiate_themes_addon_ui_settings
#endif

//#define A_NAME "UI Settings"
#define A_NAME "System Colors and Fonts"
#define A_MSGNAME Z_THEME_UI_SETTINGS
#define A_DESCRIPTION "System colors, fonts and other goodies"


class UISettingsThemesAddon : public ThemesAddon {
public:
	UISettingsThemesAddon();
	~UISettingsThemesAddon();
	
const char *Description();

status_t	RunPreferencesPanel();

status_t	AddNames(BMessage &names);

status_t	ApplyTheme(BMessage &theme, uint32 flags=0L);
status_t	MakeTheme(BMessage &theme, uint32 flags=0L);

status_t	ApplyDefaultTheme(uint32 flags=0L);
};


UISettingsThemesAddon::UISettingsThemesAddon()
	: ThemesAddon(A_NAME, A_MSGNAME)
{
}


UISettingsThemesAddon::~UISettingsThemesAddon()
{
}


const char *
UISettingsThemesAddon::Description()
{
	return A_DESCRIPTION;
}


status_t
UISettingsThemesAddon::RunPreferencesPanel()
{
	status_t err = B_OK;
	entry_ref ref;
	BEntry ent;
	/*
	err = ent.SetTo("/boot/beos/preferences/Colors");
	if (!err) {
		err = ent.GetRef(&ref);
		if (!err) {
			err = be_roster->Launch(&ref);
		}
	}
	*/
	if (!err)
		return B_OK;
	err = ent.SetTo("/system/add-ons/Preferences/Appearance");
	if (!err) {
		err = ent.GetRef(&ref);
		if (!err) {
			err = be_roster->Launch(&ref);
			if (err) {
				BMessage msg(B_REFS_RECEIVED);
				msg.AddRef("refs", &ref);
				be_app_messenger.SendMessage(&msg);
			}
		}
	}
	return err;
}


status_t
UISettingsThemesAddon::AddNames(BMessage &names)
{
	BMessage uisettings;
	BMessage uinames;
	status_t err;
	const char *str, *value;
	type_code code;
	int32 i;
	
	err = get_ui_settings(&uisettings, &uinames);
	if (err)
		return err;
	names.AddString(Z_THEME_UI_SETTINGS, "UI Settings");
	// hack for legacy fonts
	names.AddString("be:f:be_plain_font", "System Plain");
	names.AddString("be:f:be_bold_font", "System Bold");
	names.AddString("be:f:be_fixed_font", "System Fixed");
	for (i = 0; uinames.GetInfo(B_STRING_TYPE, i, &str, &code) == B_OK;i++)
		if (uinames.FindString(str, &value) == B_OK)
			names.AddString(str, value);
	return B_OK;
}


status_t
UISettingsThemesAddon::ApplyTheme(BMessage &theme, uint32 flags)
{
	BMessage uisettings;
	BFont fnt;
	status_t err;
	uint32 uiflags = 0;

	(void)flags;
	err = MyMessage(theme, uisettings);
	if (err)
		return err;

	if (flags & UI_THEME_SETTINGS_SAVE && AddonFlags() & Z_THEME_ADDON_DO_SAVE)
		uiflags |= B_SAVE_UI_SETTINGS;
	if (flags & UI_THEME_SETTINGS_APPLY && AddonFlags() & Z_THEME_ADDON_DO_APPLY)
		uiflags |= B_APPLY_UI_SETTINGS;
	if (!uiflags)
		return B_OK;

	// hack for legacy fonts
	err = uisettings.FindFlat("be:f:be_plain_font", &fnt);
	uisettings.RemoveName("be:f:be_plain_font");
	if (err == B_OK)
		BFont::SetStandardFont(B_PLAIN_FONT, &fnt);

	err = uisettings.FindFlat("be:f:be_bold_font", &fnt);
	uisettings.RemoveName("be:f:be_bold_font");
	if (err == B_OK)
		BFont::SetStandardFont(B_BOLD_FONT, &fnt);

	err = uisettings.FindFlat("be:f:be_fixed_font", &fnt);
	uisettings.RemoveName("be:f:be_fixed_font");
 	if (err == B_OK)
		BFont::SetStandardFont(B_FIXED_FONT, &fnt);
	
	update_ui_settings(uisettings, uiflags);
	
	return B_OK;
}


status_t
UISettingsThemesAddon::MakeTheme(BMessage &theme, uint32 flags)
{
	BMessage uisettings;
	BMessage names;
	BFont fnt;
	status_t err;
	
	(void)flags;
	err = MyMessage(theme, uisettings);
	if (err)
		uisettings.MakeEmpty();
	
	err = get_ui_settings(&uisettings, &names);
	if (err)
		return err;
	
	// hack for legacy fonts
	err = BFont::GetStandardFont(B_PLAIN_FONT, &fnt);
	uisettings.AddFlat("be:f:be_plain_font", &fnt);
	err = BFont::GetStandardFont(B_BOLD_FONT, &fnt);
	uisettings.AddFlat("be:f:be_bold_font", &fnt);
	err = BFont::GetStandardFont(B_FIXED_FONT, &fnt);
	uisettings.AddFlat("be:f:be_fixed_font", &fnt);

	err = SetMyMessage(theme, uisettings);
	return err;
}


status_t
UISettingsThemesAddon::ApplyDefaultTheme(uint32 flags)
{
	BMessage theme;
	BMessage uisettings;
	BFont fnt;
	status_t err;
	
	err = get_default_settings(&uisettings);
	if (err)
		return err;

	// hack for legacy fonts
	err = BFont::GetStandardFont((font_which)(B_PLAIN_FONT-100), &fnt);
	uisettings.AddFlat("be:f:be_plain_font", &fnt);
	err = BFont::GetStandardFont((font_which)(B_BOLD_FONT-100), &fnt);
	uisettings.AddFlat("be:f:be_bold_font", &fnt);
	err = BFont::GetStandardFont((font_which)(B_FIXED_FONT-100), &fnt);
	uisettings.AddFlat("be:f:be_fixed_font", &fnt);
	
	theme.AddMessage(A_MSGNAME, &uisettings);
	return ApplyTheme(theme, flags);
}


ThemesAddon *
instantiate_themes_addon()
{
	return (ThemesAddon *) new UISettingsThemesAddon;
}


#endif /* B_BEOS_VERSION_DANO */
