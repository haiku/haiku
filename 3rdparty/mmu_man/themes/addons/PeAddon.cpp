/*
 * Pe Color ThemesAddon class
 */

#include <Directory.h>
#include <Message.h>
#include <Messenger.h>
#include <Font.h>
#include <List.h>
#include <String.h>
#include <Roster.h>
#include <storage/Path.h>
#include <storage/File.h>
#include <storage/NodeInfo.h>
#include <storage/FindDirectory.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "ThemesAddon.h"
#include "UITheme.h"

#ifdef SINGLE_BINARY
#define instanciate_themes_addon instanciate_themes_addon_pe
#endif

#define A_NAME "Pe Colors"
#define A_MSGNAME NULL //Z_THEME_PE_SETTINGS
#define A_DESCRIPTION "Make Pe use system colors"

#define PE_SETTINGS_NAME "pe/settings"

class PeThemesAddon : public ThemesAddon {
public:
	PeThemesAddon();
	~PeThemesAddon();
	
const char *Description();

status_t	RunPreferencesPanel();

status_t	AddNames(BMessage &names);

status_t	ApplyTheme(BMessage &theme, uint32 flags=0L);
status_t	MakeTheme(BMessage &theme, uint32 flags=0L);

status_t	ApplyDefaultTheme(uint32 flags=0L);
};

PeThemesAddon::PeThemesAddon()
	: ThemesAddon(A_NAME, A_MSGNAME)
{
}

PeThemesAddon::~PeThemesAddon()
{
}

const char *PeThemesAddon::Description()
{
	return A_DESCRIPTION;
}

status_t	PeThemesAddon::RunPreferencesPanel()
{
	return B_OK;
}

status_t PeThemesAddon::AddNames(BMessage &names)
{
	names.AddString(Z_THEME_PE_SETTINGS, "Pe Settings");
	return B_OK;
}

status_t PeThemesAddon::ApplyTheme(BMessage &theme, uint32 flags)
{
	BMessage uisettings;
	status_t err;
	BPath PeSPath;
	rgb_color col;
	BString text;
	char buffer[10];
	
	(void)flags;
	err = theme.FindMessage(Z_THEME_UI_SETTINGS, &uisettings);
	if (err)
		return err;
	
	if (uisettings.FindRGBColor(B_UI_DOCUMENT_BACKGROUND_COLOR, &col) >= B_OK) {
		sprintf(buffer, "%02x%02x%02x", col.red, col.green, col.blue);
		text << "low color=#" << buffer << "\n";
	}
	if (uisettings.FindRGBColor(B_UI_DOCUMENT_TEXT_COLOR, &col) >= B_OK) {
		sprintf(buffer, "%02x%02x%02x", col.red, col.green, col.blue);
		text << "text color=#" << buffer << "\n";
	}
	if (uisettings.FindRGBColor("be:c:DocSBg", &col) >= B_OK) {
		sprintf(buffer, "%02x%02x%02x", col.red, col.green, col.blue);
		text << "selection color=#" << buffer << "\n";
	} else if (uisettings.FindRGBColor(B_UI_MENU_SELECTED_BACKGROUND_COLOR, &col) >= B_OK) {
		sprintf(buffer, "%02x%02x%02x", col.red, col.green, col.blue);
		text << "selection color=#" << buffer << "\n";
	}
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &PeSPath) < B_OK)
		return B_ERROR;
	PeSPath.Append(PE_SETTINGS_NAME);
	BFile PeSettings(PeSPath.Path(), B_WRITE_ONLY|B_OPEN_AT_END);
	if (PeSettings.InitCheck() < B_OK)
		return PeSettings.InitCheck();
	if (true/* || flags & UI_THEME_SETTINGS_SAVE*/) {
		if (PeSettings.Write(text.String(), strlen(text.String())) < B_OK)
			return B_ERROR;
	}
	
	if (true/* || flags & UI_THEME_SETTINGS_APPLY*/) {
		
	}
	return B_OK;
}

status_t PeThemesAddon::MakeTheme(BMessage &theme, uint32 flags)
{
	(void)theme; (void)flags;
	return B_OK;
}

status_t PeThemesAddon::ApplyDefaultTheme(uint32 flags)
{
	BMessage theme;
	BMessage uisettings;
	rgb_color bg = {255, 255, 255, 255};
	rgb_color fg = {0, 0, 0, 255};
	rgb_color selbg = {180, 200, 240, 255};
	uisettings.AddRGBColor(B_UI_DOCUMENT_BACKGROUND_COLOR, bg);
	uisettings.AddRGBColor(B_UI_DOCUMENT_TEXT_COLOR, fg);
	uisettings.AddRGBColor("be:c:DocSBg", selbg);
	theme.AddMessage(Z_THEME_UI_SETTINGS, &uisettings);
	return ApplyTheme(theme, flags);
}


ThemesAddon *instanciate_themes_addon()
{
	return (ThemesAddon *) new PeThemesAddon;
}
