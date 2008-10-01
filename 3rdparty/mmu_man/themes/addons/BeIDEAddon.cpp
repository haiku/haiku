/*
 * BeIDE Color ThemesAddon class
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
#define instantiate_themes_addon instantiate_themes_addon_beide
#endif

#define A_NAME "BeIDE Colors"
#define A_MSGNAME NULL //Z_THEME_BEIDE_SETTINGS
#define A_DESCRIPTION "Make BeIDE use system colors"

#define BEIDE_SETTINGS_NAME "Metrowerks/BeIDE_Prefs"


// __PACKED
struct beide_editor_pref {
	uint32 dummy; /* BIG ENDIAN */
	rgb_color bg;
	rgb_color selbg;
	uint32 flashing_delay; /* BIG ENDIAN */
	uint8 balance_while_typing;
	uint8 relaxed_c_popup;
	uint8 sort_func_popup;
	uint8 dummy2;
	uint8 remember_sel_pos;
	uint8 dummy3;
	uint8 dummy4;
	uint8 dummy5;
};


class BeIDEThemesAddon : public ThemesAddon {
public:
	BeIDEThemesAddon();
	~BeIDEThemesAddon();
	
const char *Description();

status_t	RunPreferencesPanel();

status_t	AddNames(BMessage &names);

status_t	ApplyTheme(BMessage &theme, uint32 flags=0L);
status_t	MakeTheme(BMessage &theme, uint32 flags=0L);

status_t	ApplyDefaultTheme(uint32 flags=0L);
};


BeIDEThemesAddon::BeIDEThemesAddon()
	: ThemesAddon(A_NAME, A_MSGNAME)
{
}


BeIDEThemesAddon::~BeIDEThemesAddon()
{
}


const char *
BeIDEThemesAddon::Description()
{
	return A_DESCRIPTION;
}


status_t
BeIDEThemesAddon::RunPreferencesPanel()
{
	return B_OK;
}


status_t
BeIDEThemesAddon::AddNames(BMessage &names)
{
	names.AddString(Z_THEME_BEIDE_SETTINGS, "BeIDE Settings");
	return B_OK;
}


status_t
BeIDEThemesAddon::ApplyTheme(BMessage &theme, uint32 flags)
{
	BMessage uisettings;
	status_t err;
	struct beide_editor_pref bp;
	BPath beideSPath;
	rgb_color col;
	
	err = theme.FindMessage(Z_THEME_UI_SETTINGS, &uisettings);
	if (err)
		return err;
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &beideSPath) < B_OK)
		return B_ERROR;
	beideSPath.Append(BEIDE_SETTINGS_NAME);
	BFile beideSettings(beideSPath.Path(), B_READ_WRITE);
	if (beideSettings.InitCheck() < B_OK)
		return beideSettings.InitCheck();
	if (beideSettings.ReadAttr("AppEditorPrefs", 'rPWM', 0LL, &bp, 
								sizeof(struct beide_editor_pref)) < B_OK)
		return B_ERROR;
	if (FindRGBColor(uisettings, B_UI_DOCUMENT_BACKGROUND_COLOR, 0, &col) >= B_OK)
		bp.bg = col;
	if (FindRGBColor(uisettings, B_UI_MENU_SELECTED_BACKGROUND_COLOR, 0, &col) >= B_OK)
		bp.selbg = col;
	
	if (flags & UI_THEME_SETTINGS_SAVE && AddonFlags() & Z_THEME_ADDON_DO_SAVE) {
		err = beideSettings.WriteAttr("AppEditorPrefs", 'rPWM', 0LL, &bp, 
									sizeof(struct beide_editor_pref));
	}
	if (flags & UI_THEME_SETTINGS_APPLY && AddonFlags() & Z_THEME_ADDON_DO_APPLY) {
		BList teamList;
		app_info ainfo;
		int32 count, i;
		be_roster->GetAppList(&teamList);
		count = teamList.CountItems();
		for (i = 0; i < count; i++) {
			if (be_roster->GetRunningAppInfo((team_id)(teamList.ItemAt(i)), &ainfo) == B_OK) {
				if (!strcmp(ainfo.signature, "application/x-mw-BeIDE")) {
					BMessenger msgr(NULL, ainfo.team);
					BMessage msg(B_PASTE);
					BMessage click(B_MOUSE_DOWN);
					/* first get the prefs window to map the correct view */
					/* hey BeIDE '_MDN' of View listview of Window 0 with 'be:view_where=BPoint(58.0, 103.0)' */
					click.AddSpecifier("View", "listview");
					click.AddSpecifier("Window", "Environment Preferences");
					click.AddPoint("be:view_where", BPoint(58.0,103.0));
					err = msgr.SendMessage(&click);
					
					msg.AddSpecifier("View", "Background");
					msg.AddSpecifier("View", "colorsview");
					msg.AddSpecifier("Window", "Environment Preferences");
					msg.AddPoint("_drop_point_", BPoint(0,0));
					msg.AddData("RGBColor", B_RGB_COLOR_TYPE, &bp.bg, 4);
					err = msgr.SendMessage(&msg);
					
					msg.MakeEmpty();
					msg.AddSpecifier("View", "Hilite");
					msg.AddSpecifier("View", "colorsview");
					msg.AddSpecifier("Window", "Environment Preferences");
					msg.AddPoint("_drop_point_", BPoint(0,0));
					msg.AddData("RGBColor", B_RGB_COLOR_TYPE, &bp.selbg, 4);
					err = msgr.SendMessage(&msg);
					
					msg.MakeEmpty();
					msg.what = 'SSav';
					msg.AddSpecifier("Window", "Environment Preferences");
					err = msgr.SendMessage(&msg);
				}
			}
		}
	}
	return B_OK;
}


status_t
BeIDEThemesAddon::MakeTheme(BMessage &theme, uint32 flags)
{
	(void)theme; (void)flags;
	return B_OK;
}


status_t
BeIDEThemesAddon::ApplyDefaultTheme(uint32 flags)
{
	BMessage theme;
	BMessage uisettings;
	rgb_color bg = {255, 255, 255, 255};
	rgb_color selbg = {216, 216, 216, 255};
	AddRGBColor(uisettings, B_UI_DOCUMENT_BACKGROUND_COLOR, bg);
	AddRGBColor(uisettings, B_UI_MENU_SELECTED_BACKGROUND_COLOR, selbg);
	theme.AddMessage(Z_THEME_UI_SETTINGS, &uisettings);
	return ApplyTheme(theme, flags);
}


ThemesAddon *
instantiate_themes_addon()
{
	return (ThemesAddon *) new BeIDEThemesAddon;
}

