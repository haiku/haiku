/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
 * screensaver ThemesAddon class
 */

#include <Alert.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Messenger.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>
#include <File.h>

#include <string.h>
#include <unistd.h>

#include "ThemesAddon.h"
#include "UITheme.h"

#ifdef SINGLE_BINARY
#define instantiate_themes_addon instantiate_themes_addon_screensaver
#endif

#define A_NAME "Screensaver"
#define A_MSGNAME Z_THEME_SCREENSAVER_SETTINGS
#define A_DESCRIPTION "Screensaver settings"

#define Z_THEME_SS_MODULE "screensaver:modulename"
#define Z_THEME_SS_MODULE_SETTINGS "screensaver:modulesettings"

#define MS_NAME "modulesettings_"


class ScreensaverThemesAddon : public ThemesAddon {
public:
	ScreensaverThemesAddon();
	~ScreensaverThemesAddon();
	
const char *Description();

status_t	RunPreferencesPanel();

status_t	AddNames(BMessage &names);

status_t	ApplyTheme(BMessage &theme, uint32 flags=0L);
status_t	MakeTheme(BMessage &theme, uint32 flags=0L);

status_t	ApplyDefaultTheme(uint32 flags=0L);
};


ScreensaverThemesAddon::ScreensaverThemesAddon()
	: ThemesAddon(A_NAME, A_MSGNAME)
{
}


ScreensaverThemesAddon::~ScreensaverThemesAddon()
{
}


const char *
ScreensaverThemesAddon::Description()
{
	return A_DESCRIPTION;
}


status_t
ScreensaverThemesAddon::RunPreferencesPanel()
{
	status_t err;
	entry_ref ref;
	BEntry ent;
	err = ent.SetTo("/boot/beos/preferences/ScreenSaver");
	if (!err) {
		err = ent.GetRef(&ref);
		if (!err) {
			err = be_roster->Launch(&ref);
		}
	}
	if (!err)
		return B_OK;
	err = ent.SetTo("/system/add-ons/Preferences/ScreenSaver");
	if (!err) {
		err = ent.GetRef(&ref);
		if (!err) {
			err = be_roster->Launch(&ref);
		}
	}
	return err;
}


status_t
ScreensaverThemesAddon::AddNames(BMessage &names)
{
	names.AddString(Z_THEME_SCREENSAVER_SETTINGS, "Screensaver settings");
	names.AddString(Z_THEME_SS_MODULE, "Screensaver active module");
	names.AddString(Z_THEME_SS_MODULE_SETTINGS, "Screensaver active module settings");
	return B_OK;
}


status_t
ScreensaverThemesAddon::ApplyTheme(BMessage &theme, uint32 flags)
{
	BMessage screensaver;
	status_t err;
	BPath path;
	BString str;
	BMessage settings;
	BMessage modsettings;
	
	if (!(flags & UI_THEME_SETTINGS_SET_ALL) || !(AddonFlags() & Z_THEME_ADDON_DO_SET_ALL))
		return B_OK;
	
	err = MyMessage(theme, screensaver);
	if (err)
		return err;
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) < B_OK)
		return B_ERROR;
	path.Append("ScreenSaver_settings");
	BFile f(path.Path(), B_READ_WRITE);
	if (settings.Unflatten(&f) < B_OK)
		return B_ERROR;
	if (screensaver.FindString(Z_THEME_SS_MODULE, &str) >= B_OK) {
		if (settings.ReplaceString("modulename", str.String()) < B_OK)
			if (settings.AddString("modulename", str.String()) < B_OK)
				return B_ERROR;
		if (screensaver.FindMessage(Z_THEME_SS_MODULE_SETTINGS, &modsettings) >= B_OK) {
			BString msname(MS_NAME);
			msname += str.String();
			if (settings.ReplaceMessage(msname.String(), &modsettings) < B_OK)
				settings.AddMessage(msname.String(), &modsettings);
		}
		f.Seek(0LL, SEEK_SET);
		if (settings.Flatten(&f) < B_OK)
			return B_ERROR;
	}
	
	return err;
}


status_t
ScreensaverThemesAddon::MakeTheme(BMessage &theme, uint32 flags)
{
	BMessage screensaver;
	status_t err;
	BPath path;
	BString str;
	BMessage settings;
	BMessage modsettings;
	
	(void)flags;
	err = MyMessage(theme, screensaver);
	if (err)
		screensaver.MakeEmpty();
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) < B_OK)
		return B_ERROR;
	path.Append("ScreenSaver_settings");
	BFile f(path.Path(), B_READ_ONLY);
	if (settings.Unflatten(&f) < B_OK)
		return B_ERROR;
	if (settings.FindString("modulename", &str) >= B_OK) {
		screensaver.AddString(Z_THEME_SS_MODULE, str.String());
		BString msname(MS_NAME);
		msname += str.String();
		if (settings.FindMessage(msname.String(), &modsettings) >= B_OK) {
		screensaver.AddMessage(Z_THEME_SS_MODULE_SETTINGS, &modsettings);
		}
	}
	
	err = SetMyMessage(theme, screensaver);
	return err;
}


status_t
ScreensaverThemesAddon::ApplyDefaultTheme(uint32 flags)
{
	BMessage theme;
	BMessage screensaver;
	screensaver.AddString("screensaver:modulename", "Blackness");
	theme.AddMessage(A_MSGNAME, &screensaver);
	return ApplyTheme(theme, flags);
}


ThemesAddon *
instantiate_themes_addon()
{
	return (ThemesAddon *) new ScreensaverThemesAddon;
}

