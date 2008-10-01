#ifndef _Z_UI_THEME_H
#define _Z_UI_THEME_H

// the global message what code
#define Z_THEME_MESSAGE_WHAT 'ZThm'

// reserved field names at top level
#define Z_THEME_NAME	"z:theme:name"
#define Z_THEME_DESCRIPTION	"z:theme:description"
#define Z_THEME_SCREENSHOT_FILENAME	"z:theme:screenshot"
#define Z_THEME_KEYWORDS	"z:theme:keywords"
		// path of the theme folder/zip
#define Z_THEME_LOCATION	"z:theme:location"
		// identifies a module this theme has info for.
#define Z_THEME_MODULE_TAG	"z:theme:moduletag"
		// if the theme is imported, where from
#define Z_THEME_IMPORTER	"z:theme:importer"

// the names of the global BMessages
#define Z_THEME_INFO_MESSAGE "z:theme:infos"
#define Z_THEME_UI_SETTINGS "ui_settings"
#define Z_THEME_WINDOW_DECORATIONS "window_decorations"
#define Z_THEME_BACKGROUND_SETTINGS B_BACKGROUND_INFO
#define Z_THEME_DESKBAR_SETTINGS "deskbar_settings"
#define Z_THEME_TERMINAL_SETTINGS "terminal_prefs"
#define Z_THEME_DIRCOLOURS_SETTINGS "dircolours_settings"
#define Z_THEME_BEIDE_SETTINGS "BeIDE_settings"
#define Z_THEME_EDDIE_SETTINGS "Eddie_settings"
#define Z_THEME_PE_SETTINGS "Pe_settings"
#define Z_THEME_SCREENSAVER_SETTINGS "screensaver_settings"
#define Z_THEME_SOUNDS_SETTINGS "sounds_settings"
#define Z_THEME_WINAMP_SKIN_SETTINGS "winamp_skin_settings"

// Addon Flags
		// allow this addon to apply themes
#define Z_THEME_ADDON_DO_APPLY		0x00000001
#define Z_THEME_ADDON_DO_SAVE		0x00000002 /* not sure about the semantics */
#define Z_THEME_ADDON_DO_SET_ALL	(Z_THEME_ADDON_DO_APPLY|Z_THEME_ADDON_DO_SAVE)
		// allow this addon to save things to themes
#define Z_THEME_ADDON_DO_RETRIEVE	0x00000004

// Theme Manager flags
		// try to apply settings to running applications
#define UI_THEME_SETTINGS_APPLY		0x00000001
		// save settings to native storage
#define UI_THEME_SETTINGS_SAVE		0x00000002
		// attempt to do both
#define UI_THEME_SETTINGS_SET_ALL	(UI_THEME_SETTINGS_APPLY|UI_THEME_SETTINGS_SAVE)
		// do read settings from native storage
#define UI_THEME_SETTINGS_RETRIEVE	0x00000004

#define Z_THEMES_FOLDER_NAME "UIThemes"
#define Z_THEME_FILE_NAME "Theme"

/* compatibility stuff ahead */

/* some field names not always defined */
// ui_colors
#ifndef B_BEOS_VERSION_DANO
#define B_UI_PANEL_BACKGROUND_COLOR "be:c:PanBg"
#define B_UI_PANEL_TEXT_COLOR "be:c:PanTx"
#define B_UI_PANEL_LINK_COLOR "be:c:PanLn"
#define B_UI_DOCUMENT_BACKGROUND_COLOR "be:c:DocBg"
#define B_UI_DOCUMENT_TEXT_COLOR "be:c:DocTx"
#define B_UI_DOCUMENT_LINK_COLOR "be:c:DocLn"
#define B_UI_CONTROL_BACKGROUND_COLOR "be:c:CtlBg"
#define B_UI_CONTROL_TEXT_COLOR "be:c:CtlTx"
#define B_UI_CONTROL_BORDER_COLOR "be:c:CtlBr"
#define B_UI_CONTROL_HIGHLIGHT_COLOR "be:c:CtlHg"
#define B_UI_NAVIGATION_BASE_COLOR "be:c:NavBs"
#define B_UI_NAVIGATION_PULSE_COLOR "be:c:NavPl"
#define B_UI_SHINE_COLOR "be:c:Shine"
#define B_UI_SHADOW_COLOR "be:c:Shadow"
#define B_UI_TOOLTIP_BACKGROUND_COLOR "be:c:TipBg"
#define B_UI_TOOLTIP_TEXT_COLOR "be:c:TipTx"
#define B_UI_MENU_BACKGROUND_COLOR "be:c:MenBg"
#define B_UI_MENU_SELECTED_BACKGROUND_COLOR "be:c:MenSBg"
#define B_UI_MENU_ITEM_TEXT_COLOR "be:c:MenTx"
#define B_UI_MENU_SELECTED_ITEM_TEXT_COLOR "be:c:MenSTx"
#define B_UI_MENU_SELECTED_BORDER_COLOR "be:c:MenSBr"
#define B_UI_SUCCESS_COLOR "be:c:Success"
#define B_UI_FAILURE_COLOR "be:c:Failure"
// fonts
// other
#define B_UI_MENU_ZSNAKE "be:MenZSnake"
// broadcasted on update
#define B_UI_SETTINGS_CHANGED '_UIC'
#endif

#define R5_DECOR_BEOS 0
#define R5_DECOR_WIN95 1
#define R5_DECOR_AMIGA 2
#define R5_DECOR_MAC 3

#ifndef __HAIKU__
#ifndef B_BEOS_VERSION_DANO
// R5 compatibility
#define B_PANEL_TEXT_COLOR B_MENU_ITEM_TEXT_COLOR
inline rgb_color make_color(uint8 red, uint8 green, uint8 blue, uint8 alpha=255)
{
	rgb_color c;
	c.red = red;
	c.green = green;
	c.blue = blue;
	c.alpha = alpha;
	return c;
}
#endif
#endif


#endif /* _Z_UI_THEME_H */
