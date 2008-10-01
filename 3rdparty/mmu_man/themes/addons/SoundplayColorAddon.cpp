/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
 * SoundPlay Color ThemesAddon class
 */

#include <InterfaceDefs.h>
#include <Message.h>
#include <Messenger.h>
#include <List.h>
#include <String.h>
#include <Roster.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "ThemesAddon.h"
#include "UITheme.h"
#include "Utils.h"

#ifdef SINGLE_BINARY
#define instantiate_themes_addon instantiate_themes_addon_soundplay
#endif

#define A_NAME "SoundPlay Color"
#define A_MSGNAME NULL //Z_THEME_SOUNDPLAY_SETTINGS
#define A_DESCRIPTION "Make SoundPlay use system colors"


class SoundplayThemesAddon : public ThemesAddon {
public:
	SoundplayThemesAddon();
	~SoundplayThemesAddon();
	
const char *Description();

status_t	RunPreferencesPanel();

status_t	AddNames(BMessage &names);

status_t	ApplyTheme(BMessage &theme, uint32 flags=0L);
status_t	MakeTheme(BMessage &theme, uint32 flags=0L);

status_t	ApplyDefaultTheme(uint32 flags=0L);
};


SoundplayThemesAddon::SoundplayThemesAddon()
	: ThemesAddon(A_NAME, A_MSGNAME)
{
}


SoundplayThemesAddon::~SoundplayThemesAddon()
{
}


const char *
SoundplayThemesAddon::Description()
{
	return A_DESCRIPTION;
}


status_t
SoundplayThemesAddon::RunPreferencesPanel()
{
	return B_OK;
}


status_t
SoundplayThemesAddon::AddNames(BMessage &names)
{
	//names.AddString(Z_THEME_BEIDE_SETTINGS, "BeIDE Settings");
	(void)names;
	return B_OK;
}


status_t
SoundplayThemesAddon::ApplyTheme(BMessage &theme, uint32 flags)
{
	BMessage uisettings;
	status_t err;
	rgb_color panelcol;
	int32 wincnt = 1;
	
	err = theme.FindMessage(Z_THEME_UI_SETTINGS, &uisettings);
	if (err)
		return err;
	
	if (FindRGBColor(uisettings, B_UI_PANEL_BACKGROUND_COLOR, 0, &panelcol) < B_OK)
		panelcol = make_color(216,216,216,255);
	
	if (flags & UI_THEME_SETTINGS_SAVE && AddonFlags() & Z_THEME_ADDON_DO_SAVE) {
		// WRITEME
	}

	if (flags & UI_THEME_SETTINGS_APPLY && AddonFlags() & Z_THEME_ADDON_DO_APPLY) {
		BMessenger msgr("application/x-vnd.marcone-soundplay");
		BMessage command(B_COUNT_PROPERTIES);
		BMessage answer;
		command.AddSpecifier("Window");
		err = msgr.SendMessage(&command, &answer,2000000LL,2000000LL);
		if(B_OK == err) {
			if (answer.FindInt32("result", &wincnt) != B_OK)
				wincnt = 1;
		}
		BMessage msg(B_PASTE);
		AddRGBColor(msg, "RGBColor", panelcol);
		msg.AddPoint("_drop_point_", BPoint(0,0));
		// send to every window (the Playlist window needs it too)
		for (int32 i = 0; i < wincnt; i++) {
			BMessage wmsg(msg);
			wmsg.AddSpecifier("Window", i);
			msgr.SendMessage(&wmsg, (BHandler *)NULL, 2000000LL);
		}
	}
	
	return B_OK;
}


status_t
SoundplayThemesAddon::MakeTheme(BMessage &theme, uint32 flags)
{
	(void)theme; (void)flags;
	return B_OK;
}


status_t
SoundplayThemesAddon::ApplyDefaultTheme(uint32 flags)
{
	BMessage theme;
	BMessage uisettings;
	rgb_color bg = {216, 216, 216, 255};
	AddRGBColor(uisettings, B_UI_PANEL_BACKGROUND_COLOR, bg);
	theme.AddMessage(Z_THEME_UI_SETTINGS, &uisettings);
	return ApplyTheme(theme, flags);
}


ThemesAddon *
instantiate_themes_addon()
{
	return (ThemesAddon *) new SoundplayThemesAddon;
}

