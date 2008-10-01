/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _THEMEMANAGER_H_
#define _THEMEMANAGER_H_

/*
 * ThemeManager class header
 */

#include <List.h>
#include <Message.h>
#include <String.h>

#include "UITheme.h"

/* backup of current settings when applying a theme */
#define THEME_ID_BACKUP -1

namespace Z {
namespace ThemeManager {
	class ThemeManager;
	class ThemesAddon;
} // ns ThemeManager
} // ns Z
using Z::ThemeManager::ThemeManager;
using Z::ThemeManager::ThemesAddon;

class BDirectory;
class BBitmap;
class BView;

namespace Z {
namespace ThemeManager {

class ThemeManager {
public:
			ThemeManager();
virtual		~ThemeManager();
	
	/* addon list */
status_t	LoadAddons();
status_t	UnloadAddons();
int32		FindAddon(const char *name);
int32		CountAddons();
ThemesAddon	*AddonAt(int32 addon);
	
	/* addon presentation */
	
const char	*AddonName(int32 addon);
const char	*AddonDescription(int32 addon);
BView		*OptionsView(int32 addon);
status_t	RunPreferencesPanel(int32 addon);
status_t	LoadSettings();
status_t	SaveSettings();
void		SetAddonFlags(int32 addon, uint32 flags);
uint32		AddonFlags(int32 addon);
			/* add pretty names */
status_t	AddNames(BMessage &names);
status_t	GetNames(BMessage &names);
	
	/* Theme enumeration - INTERNAL */
	
status_t	LoadThemes();
status_t	AddTheme(BMessage *theme);
status_t	UnloadThemes(); /* just for cleanup */
int32		CountThemes();
BMessage	*ThemeAt(int32 id, bool allowbackup = false);
status_t	SetThemeAt(int32 id, BMessage &copyfrom);
int32		FindTheme(const char *name);
status_t	CurrentTheme(BMessage &copyto);
	
	/* Theme manipulation */
	
status_t	ApplyTheme(int32 id, uint32 flags=UI_THEME_SETTINGS_SET_ALL);
int32		MakeTheme(uint32 flags=UI_THEME_SETTINGS_RETRIEVE);
status_t	UpdateTheme(int32 id, uint32 flags=UI_THEME_SETTINGS_RETRIEVE);
status_t	DeleteTheme(int32 id);
int32		SelectedTheme();

status_t	ApplyDefaultTheme(uint32 flags=UI_THEME_SETTINGS_SET_ALL);

status_t	BackupCurrent();
status_t	RestoreCurrent();
bool		CanRevert();
status_t	CompareToCurrent(BMessage &theme);
	
	/* Theme installation */
	
status_t	InstallFiles(BMessage &theme, BDirectory &folder);
status_t	BackupFiles(BMessage &theme, BDirectory &folder);

			/* save to disk */
status_t	SaveTheme(int32 id, bool excl = false);
			/* make a zip */
status_t	PackageTheme(BMessage &theme);
			/* load from disk (zip / folder) */
status_t	LoadTheme(const char *path, BMessage **to=NULL);
	
	/* Theme importation */
int32		CountThemeImporters();
const char	*ThemeImporterAt(int32 index);
status_t	ImportThemesFor(int32 index, const char *path=NULL);
	
	/* Theme properties */
	
bool		ThemeHasInfoFor(int32 id, BString &module);
status_t	ThemeName(int32 id, BString &copyto);
status_t	ThemeLocation(int32 id, BString &copyto);
bool		ThemeIsReadOnly(int32 id);
status_t	ThemeDescription(int32 id, BString &copyto);
status_t	ThemeKeywords(int32 id, BString &copyto);
status_t	ThemeScreenShot(int32 id, BBitmap **copyto);

status_t	SetThemeHasInfoFor(int32 id, BString module);
status_t	SetThemeName(int32 id, BString name);
status_t	SetThemeDescription(int32 id, BString description);
status_t	SetThemeKeywords(int32 id, BString keywords);
status_t	SetThemeScreenShot(int32 id, BBitmap *bitmap);
			/* create the screenshot from current and add it */
status_t	MakeThemeScreenShot(int32 id);
	
	/* utilities */
	
			/* remove all bad chars */
void		NormalizeThemeFolderName(BString &name);
	
private:
	BMessage	fSettings;
	BMessage	fBackupTheme; /* as in last selected */
	BString		fCurrentThemeName;
	int32		fSelectedTheme;
	BList		fAddonList;
	int32		fAddonCount;
	BList		fThemeList;
	BMessage	fNames; /* pretty names for fields */
	BList		fThemeImporters;
};

} // ns ThemeManager
} // ns Z

#define Z_THEME_MANAGER_SETTINGS_FILE "x-vnd.yT-ThemeManager"

using namespace Z::ThemeManager;


#endif	// _THEMEMANAGER_H_
