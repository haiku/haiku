/*
 * ui_settings ThemesAddon class
 */

#include <BeBuild.h>
#ifdef B_HAIKU_VERSION_1

#include <Alert.h>
#include <Application.h>
#include <InterfaceDefs.h>
#include <Entry.h>
#include <Font.h>
#include <Message.h>
#include <Roster.h>
#include <Debug.h>

#include <stdio.h>
#include <string.h>

#include "ThemesAddon.h"
#include "UITheme.h"
#include "Utils.h"

#ifdef SINGLE_BINARY
#define instanciate_themes_addon instanciate_themes_addon_ui_settings
#endif

#define DEBUG_TA
#ifdef DEBUG_TA
#define FENTRY PRINT(("*ThemesAddon[%s]::%s()\n", Name(), __FUNCTION__))
#else
#define FENTRY
#endif

#define DERR(e) { PRINT(("%s: err: %s\n", __FUNCTION__, strerror(e))); }

// private font API
extern void _set_system_font_(const char *which, font_family family,
	font_style style, float size);
extern status_t _get_system_default_font_(const char* which,
	font_family family, font_style style, float* _size);


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

struct ui_color_map {
	const char *name;
	color_which id;
} gUIColorMap[] = {
	{ B_UI_PANEL_BACKGROUND_COLOR, B_PANEL_BACKGROUND_COLOR },
	{ B_UI_PANEL_TEXT_COLOR, B_PANEL_TEXT_COLOR },
//	{ B_UI_PANEL_LINK_COLOR, B_PANEL_LINK_COLOR },
	{ B_UI_DOCUMENT_BACKGROUND_COLOR, B_DOCUMENT_BACKGROUND_COLOR },
	{ B_UI_DOCUMENT_TEXT_COLOR, B_DOCUMENT_TEXT_COLOR },
//	{ B_UI_DOCUMENT_LINK_COLOR, B_DOCUMENT_LINK_COLOR },
	{ B_UI_CONTROL_BACKGROUND_COLOR, B_CONTROL_BACKGROUND_COLOR },
	{ B_UI_CONTROL_TEXT_COLOR, B_CONTROL_TEXT_COLOR },
	{ B_UI_CONTROL_BORDER_COLOR, B_CONTROL_BORDER_COLOR },
	{ B_UI_CONTROL_HIGHLIGHT_COLOR, B_CONTROL_HIGHLIGHT_COLOR },
	{ B_UI_NAVIGATION_BASE_COLOR, B_NAVIGATION_BASE_COLOR },
	{ B_UI_NAVIGATION_PULSE_COLOR, B_NAVIGATION_PULSE_COLOR },
	{ B_UI_SHINE_COLOR, B_SHINE_COLOR },
	{ B_UI_SHADOW_COLOR, B_SHADOW_COLOR },
	{ B_UI_TOOLTIP_BACKGROUND_COLOR, B_TOOLTIP_BACKGROUND_COLOR },
	{ B_UI_TOOLTIP_TEXT_COLOR, B_TOOLTIP_TEXT_COLOR },
	{ B_UI_MENU_BACKGROUND_COLOR, B_MENU_BACKGROUND_COLOR },
	{ B_UI_MENU_SELECTED_BACKGROUND_COLOR, B_MENU_SELECTED_BACKGROUND_COLOR },
	{ B_UI_MENU_ITEM_TEXT_COLOR, B_MENU_ITEM_TEXT_COLOR },
	{ B_UI_MENU_SELECTED_ITEM_TEXT_COLOR, B_MENU_SELECTED_ITEM_TEXT_COLOR },
	{ B_UI_MENU_SELECTED_BORDER_COLOR, B_MENU_SELECTED_BORDER_COLOR },
	{ B_UI_SUCCESS_COLOR, B_SUCCESS_COLOR },
	{ B_UI_FAILURE_COLOR, B_FAILURE_COLOR },
	{ NULL, (color_which)-1 }
};

UISettingsThemesAddon::UISettingsThemesAddon()
	: ThemesAddon(A_NAME, A_MSGNAME)
{
	FENTRY;
}

UISettingsThemesAddon::~UISettingsThemesAddon()
{
	FENTRY;
}

const char *UISettingsThemesAddon::Description()
{
	return A_DESCRIPTION;
}

status_t	UISettingsThemesAddon::RunPreferencesPanel()
{
	status_t err = B_OK;
	entry_ref ref;
	BEntry ent;
	FENTRY;
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
	err = ent.SetTo("/boot/beos/preferences/Appearance");
	if (!err) {
		err = ent.GetRef(&ref);
		if (!err) {
			err = be_roster->Launch(&ref);
		}
	}
	return err;
}

status_t UISettingsThemesAddon::AddNames(BMessage &names)
{
	FENTRY;
	
	names.AddString(Z_THEME_UI_SETTINGS, "UI Settings");
	// Haiku doesn't know about them, wo add them
	//XXX: use symbolic names.
	names.AddString("be:c:PanBg", "Panel Background");
	names.AddString("be:c:PanTx", "Panel Text");
	names.AddString("be:c:PanLn", "Panel Link");
	names.AddString("be:c:DocBg", "Document Background");
	names.AddString("be:c:DocTx", "Document Text");
	names.AddString("be:c:DocLn", "Document Link");
	names.AddString("be:c:CtlBg", "Control Background");
	names.AddString("be:c:CtlTx", "Control Text");
	names.AddString("be:c:CtlBr", "Control Border");
	names.AddString("be:c:CtlHg", "Control Highlight");
	names.AddString("be:c:NavBs", "Navigation Base");
	names.AddString("be:c:NavPl", "Navigation Pulse");
	names.AddString("be:c:Shine", "Shine");
	names.AddString("be:c:Shadow", "Shadow");
	names.AddString("be:c:TipBg", "ToolTip Background");
	names.AddString("be:c:TipTx", "ToolTip Text");
	names.AddString("be:c:MenBg", "Menu Background");
	names.AddString("be:c:MenSBg", "Menu Selected Background");
	names.AddString("be:c:MenTx", "Menu Item Text");
	names.AddString("be:c:MenSTx", "Menu Selected Item Text");
	names.AddString("be:c:MenSBr", "Menu Selected Border");
	names.AddString("be:c:Success", "Success");
	names.AddString("be:c:Failure", "Failure");
	names.AddString("be:f:MenTx", "Menu Item Text");
	names.AddString("be:MenSep", "Menu Separator");
	names.AddString("be:MenTrig", "Show Menu Triggers");
	names.AddString("be:MenZSnake", "Menu ZSnake");
	names.AddString("be:f:Tip", "ToolTip");
	names.AddString("be:f:be_plain_font", "System Plain");
	names.AddString("be:f:be_bold_font", "System Bold");
	names.AddString("be:f:be_fixed_font", "System Fixed");
	return B_OK;
}

status_t UISettingsThemesAddon::ApplyTheme(BMessage &theme, uint32 flags)
{
	BMessage uisettings;
	BFont fnt;
	status_t err;
	int i;
	FENTRY;

	(void)flags;
	err = MyMessage(theme, uisettings);
	DERR(err);
	if (err)
		return err;

	font_family family;
	font_style style;
	
	err = FindFont(uisettings, "be:f:be_plain_font", 0, &fnt);
	uisettings.RemoveName("be:f:be_plain_font");
	DERR(err);
	if (err == B_OK) {
		fnt.GetFamilyAndStyle(&family, &style);
		_set_system_font_("plain", family, style, fnt.Size());
	}

	err = FindFont(uisettings, "be:f:be_bold_font", 0, &fnt);
	DERR(err);
	uisettings.RemoveName("be:f:be_bold_font");
	if (err == B_OK) {
		fnt.GetFamilyAndStyle(&family, &style);
		_set_system_font_("bold", family, style, fnt.Size());
	}

	err = FindFont(uisettings, "be:f:be_fixed_font", 0, &fnt);
	DERR(err);
	uisettings.RemoveName("be:f:be_fixed_font");
	if (err == B_OK) {
		fnt.GetFamilyAndStyle(&family, &style);
		_set_system_font_("fixed", family, style, fnt.Size());
	}

	for (i = 0; gUIColorMap[i].name; i++) {
		rgb_color c;
		if (FindRGBColor(uisettings, gUIColorMap[i].name, 0, &c) == B_OK) {
			set_ui_color(gUIColorMap[i].id, c);
			fprintf(stderr, "set_ui_color(%d, #%02x%02x%02x)\n", 
				gUIColorMap[i].id, c.red, c.green, c.blue);
		}
	}
	
	
	return B_OK;
}

status_t UISettingsThemesAddon::MakeTheme(BMessage &theme, uint32 flags)
{
	BMessage uisettings;
	BMessage names;
	BFont fnt;
	status_t err;
	int i;
	FENTRY;
	
	(void)flags;
	err = MyMessage(theme, uisettings);
	if (err)
		uisettings.MakeEmpty();
	
	for (i = 0; gUIColorMap[i].name; i++) {
		rgb_color c = ui_color(gUIColorMap[i].id);
		AddRGBColor(uisettings, gUIColorMap[i].name, c);
	}
	
	// hack for legacy fonts
	AddFont(uisettings, "be:f:be_plain_font", (BFont *)be_plain_font);
	AddFont(uisettings, "be:f:be_bold_font", (BFont *)be_bold_font);
	AddFont(uisettings, "be:f:be_fixed_font", (BFont *)be_fixed_font);

	err = SetMyMessage(theme, uisettings);
	return err;
}

status_t UISettingsThemesAddon::ApplyDefaultTheme(uint32 flags)
{
	BMessage theme;
	BMessage uisettings;
	BFont fnt;
	FENTRY;
	
	/*
	status_t err;
	err = get_default_settings(&uisettings);
	if (err)
		return err;
	*/
	
	font_family family;
	font_style style;
	float size;
	BFont f;
	if (_get_system_default_font_("plain", family, style, &size) != B_OK)
		return B_ERROR;
	f.SetFamilyAndStyle(family, style);
	f.SetSize(size);
	AddFont(uisettings, "be:f:be_plain_font", &f);
	
	if (_get_system_default_font_(Name(), family, style, &size) != B_OK)
		return B_ERROR;
	f.SetFamilyAndStyle(family, style);
	f.SetSize(size);
	AddFont(uisettings, "be:f:be_plain_font", &f);
	
	if (_get_system_default_font_(Name(), family, style, &size) != B_OK)
		return B_ERROR;
	f.SetFamilyAndStyle(family, style);
	f.SetSize(size);
	AddFont(uisettings, "be:f:be_plain_font", &f);
	
	theme.AddMessage(A_MSGNAME, &uisettings);
	return ApplyTheme(theme, flags);
}


ThemesAddon *instanciate_themes_addon()
{
	return (ThemesAddon *) new UISettingsThemesAddon;
}

#endif /* B_BEOS_VERSION_DANO */
