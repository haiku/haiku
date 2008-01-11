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

#define Z_THEMES_FOLDER_NAME "UIThemes"
#define Z_THEME_FILE_NAME "Theme"

#endif /* _Z_UI_THEME_H */
