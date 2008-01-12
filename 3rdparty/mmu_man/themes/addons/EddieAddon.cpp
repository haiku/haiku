/*
 * Eddie Color ThemesAddon class
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
#include "Utils.h"

#ifdef SINGLE_BINARY
#define instanciate_themes_addon instanciate_themes_addon_eddie
#endif

#define A_NAME "Eddie Colors"
#define A_MSGNAME NULL //Z_THEME_Eddie_SETTINGS
#define A_DESCRIPTION "Make Eddie use system colors"

#define EDDIE_SETTINGS_NAME "Eddie/settings"

class EddieThemesAddon : public ThemesAddon {
public:
	EddieThemesAddon();
	~EddieThemesAddon();
	
const char *Description();

status_t	RunPreferencesPanel();

status_t	AddNames(BMessage &names);

status_t	ApplyTheme(BMessage &theme, uint32 flags=0L);
status_t	MakeTheme(BMessage &theme, uint32 flags=0L);

status_t	ApplyDefaultTheme(uint32 flags=0L);
};

EddieThemesAddon::EddieThemesAddon()
	: ThemesAddon(A_NAME, A_MSGNAME)
{
}

EddieThemesAddon::~EddieThemesAddon()
{
}

const char *EddieThemesAddon::Description()
{
	return A_DESCRIPTION;
}

status_t	EddieThemesAddon::RunPreferencesPanel()
{
	return B_OK;
}

status_t EddieThemesAddon::AddNames(BMessage &names)
{
	names.AddString(Z_THEME_EDDIE_SETTINGS, "Eddie Settings");
	return B_OK;
}

status_t EddieThemesAddon::ApplyTheme(BMessage &theme, uint32 flags)
{
	BMessage uisettings;
	status_t err;
	BPath EddieSPath;
	rgb_color col;
	BString text;
	char buffer[10];
	
	(void)flags;
	err = theme.FindMessage(Z_THEME_UI_SETTINGS, &uisettings);
	if (err)
		return err;
	
	if (FindRGBColor(uisettings, B_UI_DOCUMENT_BACKGROUND_COLOR, 0, &col) >= B_OK) {
		sprintf(buffer, "%02x%02x%02x", col.red, col.green, col.blue);
		text << "BackgroundColor " << buffer << "\n";
	}
	if (FindRGBColor(uisettings, B_UI_DOCUMENT_TEXT_COLOR, 0, &col) >= B_OK) {
		sprintf(buffer, "%02x%02x%02x", col.red, col.green, col.blue);
		text << "TextColor " << buffer << "\n";
	}
	if (FindRGBColor(uisettings, "be:c:DocSBg", 0, &col) >= B_OK) {
		sprintf(buffer, "%02x%02x%02x", col.red, col.green, col.blue);
		text << "SelectionColor " << buffer << "\n";
	} else if (FindRGBColor(uisettings, B_UI_MENU_SELECTED_BACKGROUND_COLOR, 0, &col) >= B_OK) {
		sprintf(buffer, "%02x%02x%02x", col.red, col.green, col.blue);
		text << "SelectionColor " << buffer << "\n";
	}
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &EddieSPath) < B_OK)
		return B_ERROR;
	EddieSPath.Append(EDDIE_SETTINGS_NAME);
	BFile EddieSettings(EddieSPath.Path(), B_WRITE_ONLY|B_OPEN_AT_END);
	if (EddieSettings.InitCheck() < B_OK)
		return EddieSettings.InitCheck();
	if (true/* || flags & UI_THEME_SETTINGS_SAVE*/) {
		if (EddieSettings.Write(text.String(), strlen(text.String())) < B_OK)
			return B_ERROR;
	}
	
	if (true/* || flags & UI_THEME_SETTINGS_APPLY*/) {
		
	}
	return B_OK;
}

status_t EddieThemesAddon::MakeTheme(BMessage &theme, uint32 flags)
{
	(void)theme; (void)flags;
	return B_OK;
}

status_t EddieThemesAddon::ApplyDefaultTheme(uint32 flags)
{
	BMessage theme;
	BMessage uisettings;
	rgb_color bg = {255, 255, 255, 255};
	rgb_color fg = {0, 0, 0, 255};
	rgb_color selbg = {180, 200, 240, 255};
	AddRGBColor(uisettings, B_UI_DOCUMENT_BACKGROUND_COLOR, bg);
	AddRGBColor(uisettings, B_UI_DOCUMENT_TEXT_COLOR, fg);
	AddRGBColor(uisettings, "be:c:DocSBg", selbg);
	theme.AddMessage(Z_THEME_UI_SETTINGS, &uisettings);
	return ApplyTheme(theme, flags);
}


ThemesAddon *instanciate_themes_addon()
{
	return (ThemesAddon *) new EddieThemesAddon;
}
