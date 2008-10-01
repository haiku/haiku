/*
 * BeIDE Color ThemesAddon class
 */

#include <Alert.h>
#include <Directory.h>
#include <Message.h>
#include <Messenger.h>
#include <Font.h>
#include <String.h>
#include <Roster.h>
#include <storage/Path.h>
#include <storage/File.h>
#include <storage/NodeInfo.h>
#include <storage/FindDirectory.h>

#define DEBUG 1
#include <Debug.h>

#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "ThemesAddon.h"
#include "UITheme.h"
#include "Utils.h"

#ifdef SINGLE_BINARY
#define instantiate_themes_addon instantiate_themes_addon_terminal
#endif

#define A_NAME "Terminal"
#define A_MSGNAME Z_THEME_TERMINAL_SETTINGS
#define A_DESCRIPTION "Terminal color, font and size"

/* definitions for R5 Terminal settings file */

#define TP_MAGIC 0xf1f2f3f4
#define TP_VERSION 0x02
#define TP_FONT_NAME_SZ 128

// __PACKED

struct tpref {
	uint32 cols;
	uint32 rows;
	uint32 tab_width;
	uint32 font_size;
	char font[TP_FONT_NAME_SZ]; // "Family/Style"
	uint32 cursor_blink_rate; // blinktime in µs = 1000000
	uint32 refresh_rate; // ??? = 0
	rgb_color bg;
	rgb_color fg;
	rgb_color curbg;
	rgb_color curfg;
	rgb_color selbg;
	rgb_color selfg;
	char encoding; // index in the menu (0 = UTF-8)
	char unknown[3];
}; /* this is what is sent to the Terminal window... actually not ! It's sent by Terminal to itself. */

struct termprefs {
	uint32 magic;
	uint32 version;
	float x;
	float y;
	struct tpref p;
};

#define MSG_R5_SET_PREF 'pref'

/* definitions for Haiku Terminal settings file */
/* from TermConst.h */

#define PREF_HALF_FONT_FAMILY "Half Font Family"
#define PREF_HALF_FONT_STYLE "Half Font Style"
#define PREF_HALF_FONT_SIZE "Half Font Size"

#define PREF_TEXT_FORE_COLOR "Text Foreground Color"
#define PREF_TEXT_BACK_COLOR "Text Background Color"
#define PREF_SELECT_FORE_COLOR "Selection Foreground Color"
#define PREF_SELECT_BACK_COLOR "Selection Background Color"
#define PREF_CURSOR_FORE_COLOR "Cursor Foreground Color"
#define PREF_CURSOR_BACK_COLOR "Cursor Background Color"

#define PREF_IM_FORE_COLOR "IM Foreground Color"
#define PREF_IM_BACK_COLOR "IM Background Color"
#define PREF_IM_SELECT_COLOR "IM Selection Color"

#define PREF_HISTORY_SIZE "History Size"
#define PREF_CURSOR_BLINKING "Cursor Blinking rate"

#define PREF_IM_AWARE = "Input Method Aware"

#define PREF_COLS "Cols"
#define PREF_ROWS "Rows"
#define PREF_SHELL "Shell"

#define PREF_TEXT_ENCODING "Text encoding"
#define PREF_GUI_LANGUAGE "Language"


/* theme field names */

#define TP_COLS "term:cols"
#define TP_ROWS "term:rows"
#define TP_TABWIDTH "term:tab"
#define TP_FONT "term:font"
#define TP_BG "term:c:bg"
#define TP_FG "term:c:fg"
#define TP_CURBG "term:c:curbg"
#define TP_CURFG "term:c:curfg"
#define TP_SELBG "term:c:selbg"
#define TP_SELFG "term:c:selfg"
#define TP_ENCODING "term:encoding"

static const char *kHaikuTerminalAppSig = "application/x-vnd.Haiku-Terminal";
static const char *kBeOSTerminalAppSig = "application/x-vnd.Be-SHEL";


class TerminalThemesAddon : public ThemesAddon {
public:
	TerminalThemesAddon();
	~TerminalThemesAddon();
	
const char *Description();

status_t	RunPreferencesPanel();

status_t	AddNames(BMessage &names);

status_t	ApplyTheme(BMessage &theme, uint32 flags=0L);
status_t	MakeTheme(BMessage &theme, uint32 flags=0L);

status_t	ApplyDefaultTheme(uint32 flags=0L);

	/* Theme installation */
status_t	InstallFiles(BMessage &theme, BDirectory &folder);
status_t	BackupFiles(BMessage &theme, BDirectory &folder);

private:
status_t	ApplyThemeR5(BMessage &theme, uint32 flags);
status_t	MakeThemeR5(BMessage &theme, uint32 flags);
status_t	ApplyThemeHaiku(BMessage &theme, uint32 flags);
status_t	MakeThemeHaiku(BMessage &theme, uint32 flags);
status_t	LoadHaikuTerminalSettings(BMessage &into);
status_t	SaveHaikuTerminalSettings(BMessage &from);
};


TerminalThemesAddon::TerminalThemesAddon()
	: ThemesAddon(A_NAME, A_MSGNAME)
{
}


TerminalThemesAddon::~TerminalThemesAddon()
{
}


const char *
TerminalThemesAddon::Description()
{
	return A_DESCRIPTION;
}


status_t
TerminalThemesAddon::RunPreferencesPanel()
{
	BAlert *alert;
	alert = new BAlert("info", "Please use the Settings menu in the Terminal application.", "Gotcha");
	alert->Go();
	return B_OK;
}


status_t
TerminalThemesAddon::AddNames(BMessage &names)
{
	names.AddString(Z_THEME_TERMINAL_SETTINGS, "Terminal Preferences");
	names.AddString(TP_COLS, "Terminal column count");
	names.AddString(TP_ROWS, "Terminal row count");
	names.AddString(TP_TABWIDTH, "Terminal tab width");
	names.AddString(TP_FONT, "Terminal font");
	names.AddString(TP_BG, "Terminal background color");
	names.AddString(TP_FG, "Terminal foreground color");
	names.AddString(TP_CURBG, "Terminal cursor background color");
	names.AddString(TP_CURFG, "Terminal cursor foreground color");
	names.AddString(TP_SELBG, "Terminal selection background color");
	names.AddString(TP_SELFG, "Terminal selection foreground color");
	names.AddString(TP_ENCODING, "Terminal text encoding");
	return B_OK;
}


status_t
TerminalThemesAddon::ApplyTheme(BMessage &theme, uint32 flags)
{
	status_t err;

#ifndef __HAIKU__
	err = ApplyThemeR5(theme, flags);
#endif
	// Some people use the Haiku terminal in BeOS.
	err = ApplyThemeHaiku(theme, flags);
	if (err)
		return err;
	return B_OK;
}


status_t
TerminalThemesAddon::MakeTheme(BMessage &theme, uint32 flags)
{
#ifdef __HAIKU__
	return MakeThemeHaiku(theme, flags);
#else
	return MakeThemeR5(theme, flags);
#endif
}


status_t
TerminalThemesAddon::ApplyDefaultTheme(uint32 flags)
{
	BMessage theme;
	BMessage termpref;
	// XXX: add font and stuff...
	AddRGBColor(termpref, "term:c:bg",make_color(255,255,255,0));
	AddRGBColor(termpref, "term:c:fg",make_color(0,0,0,0));
	AddRGBColor(termpref, "term:c:curbg",make_color(0,0,0,0));
	AddRGBColor(termpref, "term:c:curfg",make_color(255,255,255,0));
	AddRGBColor(termpref, "term:c:selbg",make_color(0,0,0,0));
	AddRGBColor(termpref, "term:c:selfg",make_color(255,255,255,0));
	theme.AddMessage(Z_THEME_TERMINAL_SETTINGS, &termpref);
	return ApplyTheme(theme, flags);
}


status_t
TerminalThemesAddon::InstallFiles(BMessage &theme, BDirectory &folder)
{
	BMessage termpref;
	status_t err;
	
	(void)folder;
	err = MyMessage(theme, termpref);
	if (err)
		termpref.MakeEmpty();
	
	return B_OK;
}


status_t
TerminalThemesAddon::BackupFiles(BMessage &theme, BDirectory &folder)
{
	BMessage termpref;
	status_t err;
	
	(void)folder;
	err = MyMessage(theme, termpref);
	if (err)
		termpref.MakeEmpty();
	
	entry_ref ref;
	//find_font_file(&ref, ff, fs);
	return B_OK;
}


status_t
TerminalThemesAddon::ApplyThemeR5(BMessage &theme, uint32 flags)
{
	BMessage termpref;
	status_t err;
	struct termprefs tp;
	
	err = MyMessage(theme, termpref);
	if (err)
		return err;
	tp.magic = TP_MAGIC;
	tp.version = TP_VERSION;
	tp.x = 0; // don't open at specific coords
	tp.y = 0;
	if (termpref.FindInt32(TP_COLS, (int32 *)&tp.p.cols) != B_OK)
		tp.p.cols = 80;
	if (termpref.FindInt32(TP_ROWS, (int32 *)&tp.p.rows) != B_OK)
		tp.p.rows = 25;
	if (termpref.FindInt32(TP_TABWIDTH, (int32 *)&tp.p.tab_width) != B_OK)
		tp.p.tab_width = 8;
	BFont tFont;
	tp.p.font_size = 12;
	strcpy(tp.p.font, "Courier10 BT/Roman");
	if (FindFont(termpref, TP_FONT, 0, &tFont) == B_OK) {
		font_family ff;
		font_style fs;
		tFont.GetFamilyAndStyle(&ff, &fs);
		strcpy(tp.p.font, ff);
		strcat(tp.p.font, "/");
		strcat(tp.p.font, fs);
		tp.p.font_size = (uint32)tFont.Size();
	}
	tp.p.cursor_blink_rate = 1000000;
	tp.p.refresh_rate = 0;
	
	if (FindRGBColor(termpref, TP_BG, 0, &tp.p.bg) != B_OK)
		tp.p.bg = make_color(255,255,255,255);
	if (FindRGBColor(termpref, TP_FG, 0, &tp.p.fg) != B_OK)
		tp.p.fg = make_color(0,0,0,255);
	if (FindRGBColor(termpref, TP_CURBG, 0, &tp.p.curbg) != B_OK)
		tp.p.curbg = make_color(255,255,255,255);
	if (FindRGBColor(termpref, TP_CURFG, 0, &tp.p.curfg) != B_OK)
		tp.p.curfg = make_color(0,0,0,255);
	if (FindRGBColor(termpref, TP_SELBG, 0, &tp.p.selbg) != B_OK)
		tp.p.selbg = make_color(0,0,0,255);
	if (FindRGBColor(termpref, TP_SELFG, 0, &tp.p.selfg) != B_OK)
		tp.p.selfg = make_color(255,255,255,255);
	
	if (termpref.FindInt32(TP_ENCODING, (int32 *)&tp.p.encoding) != B_OK)
		tp.p.encoding = 0; // UTF-8
	
	if (flags & UI_THEME_SETTINGS_SAVE && AddonFlags() & Z_THEME_ADDON_DO_SAVE) {
		BPath pTermPref;
		if (find_directory(B_USER_SETTINGS_DIRECTORY, &pTermPref) < B_OK)
			return EINVAL;
		pTermPref.Append("Terminal");
		BFile fTermPref(pTermPref.Path(), B_WRITE_ONLY|B_CREATE_FILE|B_ERASE_FILE);
		if (fTermPref.InitCheck() != B_OK) {
			return fTermPref.InitCheck();
		}
		fTermPref.Write(&tp, sizeof(struct termprefs));
		BNodeInfo ni(&fTermPref);
		if (ni.InitCheck() == B_OK)
			ni.SetType("application/x-vnd.Be-pref");
	}

	if (flags & UI_THEME_SETTINGS_APPLY && AddonFlags() & Z_THEME_ADDON_DO_APPLY) {
		BList teamList;
		app_info ainfo;
		int32 count, i;
		be_roster->GetAppList(&teamList);
		count = teamList.CountItems();
		for (i = 0; i < count; i++) {
			if (be_roster->GetRunningAppInfo((team_id)(teamList.ItemAt(i)), &ainfo) == B_OK) {
				if (!strcmp(ainfo.signature, kBeOSTerminalAppSig)) {
					err = B_OK;
					BMessage msg(MSG_R5_SET_PREF);
					BMessenger msgr(NULL, ainfo.team);
					tp.x = 0;
					tp.y = 0;
					
					//msg.AddData("", 'UBYT', &(tp.p), sizeof(struct tpref));
					msg.AddData("", 'UBYT', &(tp), sizeof(struct termprefs));
					msg.AddSpecifier("Window", 0L);
					err = msgr.SendMessage(&msg);
				}
			}
		}
	}
	
	return B_OK;
}


status_t
TerminalThemesAddon::MakeThemeR5(BMessage &theme, uint32 flags)
{
	BMessage termpref;
	status_t err;
	struct termprefs tp;
	BPath pTermPref;
	
	(void)flags;
	err = MyMessage(theme, termpref);
	if (err)
		termpref.MakeEmpty();
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &pTermPref) < B_OK)
		return EINVAL;
	pTermPref.Append("Terminal");
	BFile fTermPref(pTermPref.Path(), B_READ_ONLY);
	if (fTermPref.InitCheck() != B_OK)
		return fTermPref.InitCheck();
	if (fTermPref.Read(&tp, sizeof(struct termprefs)) < (ssize_t)sizeof(struct termprefs))
		return EIO;
	if ((tp.magic != TP_MAGIC) || (tp.version != TP_VERSION))
		return EINVAL;
	termpref.AddInt32(TP_COLS, tp.p.cols);
	termpref.AddInt32(TP_ROWS, tp.p.rows);
	termpref.AddInt32(TP_TABWIDTH, tp.p.tab_width);
	BFont tFont;
	font_family ff;
	font_style fs;
	BString str(tp.p.font);
	str.Truncate(str.FindFirst('/'));
	strncpy(ff, str.String(), sizeof(ff));
	str.SetTo(tp.p.font);
	str.Remove(0, str.FindFirst('/')+1);
	strncpy(fs, str.String(), sizeof(fs));
	tFont.SetFamilyAndStyle(ff, fs);
	tFont.SetSize(tp.p.font_size);
	AddFont(termpref, TP_FONT, &tFont);
	AddRGBColor(termpref, TP_BG, tp.p.bg);
	AddRGBColor(termpref, TP_FG, tp.p.fg);
	AddRGBColor(termpref, TP_CURBG, tp.p.curbg);
	AddRGBColor(termpref, TP_CURFG, tp.p.curfg);
	AddRGBColor(termpref, TP_SELBG, tp.p.selbg);
	AddRGBColor(termpref, TP_SELFG, tp.p.selfg);
	termpref.AddInt32(TP_ENCODING, tp.p.encoding);

	err = SetMyMessage(theme, termpref);
	return B_OK;
}


status_t
TerminalThemesAddon::ApplyThemeHaiku(BMessage &theme, uint32 flags)
{
	BMessage termpref;
	BMessage lines;
	status_t err;
	struct termprefs tp;
	int32 ival;
	rgb_color color;
	BString s;

	err = MyMessage(theme, termpref);
	if (err)
		return err;

	if (termpref.FindInt32(TP_COLS, &ival) < B_OK)
		ival = 80;
	s = "";
	s << ival;
	lines.AddString(PREF_COLS, s.String());

	if (termpref.FindInt32(TP_ROWS, &ival) < B_OK)
		ival = 25;
	s = "";
	s << ival;
	lines.AddString(PREF_ROWS, s.String());

	if (termpref.FindInt32(TP_TABWIDTH, &ival) >= B_OK) {
		//XXX: handle that ?
	}

	BFont tFont;
	tp.p.font_size = 12;
	strcpy(tp.p.font, "Courier10 BT/Roman");
	if (FindFont(termpref, TP_FONT, 0, &tFont) == B_OK) {
		font_family ff;
		font_style fs;
		tFont.GetFamilyAndStyle(&ff, &fs);
		s = "";
		s << ff;
		lines.AddString(PREF_HALF_FONT_FAMILY, s.String());
		s = "";
		s << fs;
		lines.AddString(PREF_HALF_FONT_STYLE, s.String());
		s = "";
		s << tFont.Size();
		lines.AddString(PREF_HALF_FONT_SIZE, s.String());
	}
	
	if (FindRGBColor(termpref, TP_BG, 0, &color) != B_OK)
		color = make_color(255,255,255,255);
	s = "";
	s << color.red << ", " << color.green << ", " << color.blue;
	lines.AddString(PREF_TEXT_BACK_COLOR, s.String());
	
	if (FindRGBColor(termpref, TP_FG, 0, &color) != B_OK)
		color = make_color(0,0,0,255);
	s = "";
	s << color.red << ", " << color.green << ", " << color.blue;
	lines.AddString(PREF_TEXT_FORE_COLOR, s.String());

	if (FindRGBColor(termpref, TP_CURBG, 0, &color) != B_OK)
		color = make_color(255,255,255,255);
	s = "";
	s << color.red << ", " << color.green << ", " << color.blue;
	lines.AddString(PREF_CURSOR_BACK_COLOR, s.String());

	if (FindRGBColor(termpref, TP_CURFG, 0, &color) != B_OK)
		color = make_color(0,0,0,255);
	s = "";
	s << color.red << ", " << color.green << ", " << color.blue;
	lines.AddString(PREF_CURSOR_FORE_COLOR, s.String());

	if (FindRGBColor(termpref, TP_SELBG, 0, &color) != B_OK)
		color = make_color(0,0,0,255);
	s = "";
	s << color.red << ", " << color.green << ", " << color.blue;
	lines.AddString(PREF_SELECT_BACK_COLOR, s.String());

	if (FindRGBColor(termpref, TP_SELFG, 0, &color) != B_OK)
		color = make_color(255,255,255,255);
	s = "";
	s << color.red << ", " << color.green << ", " << color.blue;
	lines.AddString(PREF_SELECT_FORE_COLOR, s.String());
	
	/* XXX: handle PREF_IM_FORE_COLOR PREF_IM_BACK_COLOR PREF_IM_SELECT_COLOR */

	
	if (termpref.FindInt32(TP_ENCODING, &ival) != B_OK)
		ival = 0; // UTF-8
	s = "";
	s << ival;
	//XXX: shouldn't really be touched...
	//lines.AddString(, s.String());
	
	if (flags & UI_THEME_SETTINGS_SAVE && AddonFlags() & Z_THEME_ADDON_DO_SAVE) {
		SaveHaikuTerminalSettings(lines);
	}

	if (flags & UI_THEME_SETTINGS_APPLY && AddonFlags() & Z_THEME_ADDON_DO_APPLY) {
		BList teamList;
		app_info ainfo;
		int32 count, i;
		be_roster->GetAppList(&teamList);
		count = teamList.CountItems();
		for (i = 0; i < count; i++) {
			if (be_roster->GetRunningAppInfo((team_id)(teamList.ItemAt(i)), &ainfo) == B_OK) {
				if (!strcmp(ainfo.signature, kHaikuTerminalAppSig)) {
					err = B_OK;
					//XXX: WRITEME
/*					BMessage msg(MSG_R5_SET_PREF);
					BMessenger msgr(NULL, ainfo.team);
					tp.x = 0;
					tp.y = 0;
					
					//msg.AddData("", 'UBYT', &(tp.p), sizeof(struct tpref));
					msg.AddData("", 'UBYT', &(tp), sizeof(struct termprefs));
					msg.AddSpecifier("Window", 0L);
					err = msgr.SendMessage(&msg);
*/
				}
			}
		}
	}
	
	return B_OK;
}


status_t
TerminalThemesAddon::MakeThemeHaiku(BMessage &theme, uint32 flags)
{
	BMessage termpref;
	BMessage lines;
	status_t err;
	BPath pTermPref;
	char buffer[1024];
	char key[B_FIELD_NAME_LENGTH], data[512];
	int n;
	FILE *file;
	
	(void)flags;
	err = MyMessage(theme, termpref);
	if (err)
		termpref.MakeEmpty();
	
	err = LoadHaikuTerminalSettings(lines);
	
	//XXX: WRITEME

/*
	while (fgets(buffer, sizeof(buffer), file) != NULL) {
		if (*buffer == '#')
			continue;

		n = sscanf(buffer, "%*[\"]%[^\"]%*[\"]%*[^\"]%*[\"]%[^\"]", key, data);
		if (n == 2) {
			if (strstr(key, " Color")) {
				rgb_color color;
				int r, g, b;
				n = sscanf(data, "%3d, %3d, %3d", &r, &g, &b);
				color.red = r;
				color.green = g;
				color.blue = b;
				color.alpha = 255;
				
			} else
				lines.AddString(key, data);
		}
	}
*/
/*
	BFile fTermPref(pTermPref.Path(), B_READ_ONLY);
	if (fTermPref.InitCheck() != B_OK)
		return fTermPref.InitCheck();
	if (fTermPref.Read(&tp, sizeof(struct termprefs)) < (ssize_t)sizeof(struct termprefs))
		return EIO;
	if ((tp.magic != TP_MAGIC) || (tp.version != TP_VERSION))
		return EINVAL;
	termpref.AddInt32(TP_COLS, tp.p.cols);
	termpref.AddInt32(TP_ROWS, tp.p.rows);
	termpref.AddInt32(TP_TABWIDTH, tp.p.tab_width);
	BFont tFont;
	font_family ff;
	font_style fs;
	BString str(tp.p.font);
	str.Truncate(str.FindFirst('/'));
	strncpy(ff, str.String(), sizeof(ff));
	str.SetTo(tp.p.font);
	str.Remove(0, str.FindFirst('/')+1);
	strncpy(fs, str.String(), sizeof(fs));
	tFont.SetFamilyAndStyle(ff, fs);
	tFont.SetSize(tp.p.font_size);
	AddFont(termpref, TP_FONT, &tFont);
	AddRGBColor(termpref, TP_BG, tp.p.bg);
	AddRGBColor(termpref, TP_FG, tp.p.fg);
	AddRGBColor(termpref, TP_CURBG, tp.p.curbg);
	AddRGBColor(termpref, TP_CURFG, tp.p.curfg);
	AddRGBColor(termpref, TP_SELBG, tp.p.selbg);
	AddRGBColor(termpref, TP_SELFG, tp.p.selfg);
	termpref.AddInt32(TP_ENCODING, tp.p.encoding);
*/
	err = SetMyMessage(theme, termpref);
	return B_OK;
}


status_t
TerminalThemesAddon::LoadHaikuTerminalSettings(BMessage &into)
{
	status_t err;
	BPath pTermPref;
	char buffer[1024];
	char key[B_FIELD_NAME_LENGTH], data[512];
	int n;
	FILE *file;
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &pTermPref) < B_OK)
		return EINVAL;
#ifdef __HAIKU__
	pTermPref.Append("Terminal_settings");
#else
	pTermPref.Append("HaikuTerminal_settings");
#endif
	// cf PrefHandler.cpp
	file = fopen(pTermPref.Path(), "r");
	if (file == NULL)
		return B_ENTRY_NOT_FOUND;

	while (fgets(buffer, sizeof(buffer), file) != NULL) {
		if (*buffer == '#')
			continue;

		n = sscanf(buffer, "%*[\"]%[^\"]%*[\"]%*[^\"]%*[\"]%[^\"]", key, data);
		if (n == 2) {
			into.AddString(key, data);
		}
	}
	fclose(file);
	return B_OK;
}


status_t
TerminalThemesAddon::SaveHaikuTerminalSettings(BMessage &from)
{
	BMessage settings;
	status_t err;
	BPath pTermPref;
	
	// load existing
	err = LoadHaikuTerminalSettings(settings);
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &pTermPref) < B_OK)
		return EINVAL;
#ifdef __HAIKU__
	pTermPref.Append("Terminal_settings");
#else
	pTermPref.Append("HaikuTerminal_settings");
#endif

	BFile file(pTermPref.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	char buffer[512];
	type_code type;
	char *key;
	err = file.InitCheck();
	if (err < B_OK)
		return err;

	// merge new values
	for (int32 i = 0;
			from.GetInfo(B_STRING_TYPE, i, GET_INFO_NAME_PTR(&key), &type) == B_OK;
			i++) {
		BString s;
		if (from.FindString(key, &s) < B_OK)
			continue;
		settings.RemoveName(key);
		settings.AddString(key, s.String());
	}

	for (int32 i = 0;
			settings.GetInfo(B_STRING_TYPE, i, GET_INFO_NAME_PTR(&key), &type) == B_OK;
			i++) {
		BString s;
		if (settings.FindString(key, &s) < B_OK)
			continue;
		int len = snprintf(buffer, sizeof(buffer), "\"%s\" , \"%s\"\n", key, s.String());
		file.Write(buffer, len);
	}
	return B_OK;
}


ThemesAddon *
instantiate_themes_addon()
{
	return (ThemesAddon *) new TerminalThemesAddon;
}

