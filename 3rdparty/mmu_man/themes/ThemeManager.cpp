/*
 * ThemeManager class
 */

#include <Bitmap.h>
#include <BitmapStream.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>
#include <StorageDefs.h>
#include <String.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>

#include <image.h>

#include <string.h>
#include <stdio.h>

#include "UITheme.h"
#include "ThemeManager.h"
#include "ThemesAddon.h"
#include "ThemeImporter.h"
#include "BeThemeImporter.h"
#include "MSThemeImporter.h"

#include "ParseMessage.h"
#include "DumpMessage.h"

extern status_t MakeScreenshot(BBitmap **here);

// addons used in the prefs in Zeta (some were disabled)
//#define ZETA_ADDONS

//#define DEBUG_TM
#ifdef DEBUG_TM
#define FENTRY PRINT(("ThemeManager::%s()\n", __FUNCTION__))
#else
#define FENTRY
#endif


ThemeManager::ThemeManager()
	: fAddonCount(0)
{
	FENTRY;
	fSettings.MakeEmpty();
	fBackupTheme.MakeEmpty();
	fBackupTheme.what = 'null';//Z_THEME_MESSAGE_WHAT;
	fSelectedTheme = -1;
	fAddonList.MakeEmpty();
	fNames.MakeEmpty();
	LoadAddons();
	AddNames(fNames);
	LoadSettings();

	// XXX: add more
	fThemeImporters.AddItem(new BeThemeImporter());
	fThemeImporters.AddItem(new MSThemeImporter());
	
	// XXX test
/*
	BMessage test('plop');
	BMessage *to;
	test.AddString("plop-plip", "this is a test...\"line cont'd.");
	test.PrintToStream();
	BFile tfile("/tmp/testmsg", B_READ_WRITE|B_CREATE_FILE);
	DumpMessageToStream(&test, tfile, 0, NULL);
	tfile.Seek(0LL, SEEK_SET);
	ParseMessageFromStream(&to, tfile);
	if (to)
		to->PrintToStream();
	else
		PRINT(("no to\n"));
*/
}


ThemeManager::~ThemeManager()
{
	FENTRY;
	SaveSettings();
	if (CountThemes())
		UnloadThemes();
	if (CountAddons())
		UnloadAddons();
}


status_t
ThemeManager::LoadAddons()
{
	FENTRY;
	ThemesAddon *ta;
	BPath path;
	BDirectory dir;
	entry_ref ref;
#ifndef SINGLE_BINARY
	uint32 addonFlags;
	int dirwhich;
	int32 i;
	status_t err;
	image_id img;
	ThemesAddon *(*instantiate_func)();
#endif
	
#ifndef SINGLE_BINARY
	for (dirwhich = 0; dirwhich < 2; dirwhich++) {
		if (!dirwhich)	/* find system settings dir */
			err = find_directory(B_BEOS_ADDONS_DIRECTORY, &path);
		else			/* find user settings dir */
			err = find_directory(B_USER_ADDONS_DIRECTORY, &path);
		if (err)	return err;
		
		path.Append(Z_THEMES_ADDON_FOLDER);
		
		err = dir.SetTo(path.Path());
		if (err)	continue;
		
		err = dir.Rewind();
		if (err)	continue;
		
		while ((err = dir.GetNextRef(&ref)) == B_OK) {
			bool screwed = false;
			BPath imgpath(&ref);
			BString p(Z_THEMES_ADDON_FOLDER);
			p << "/" << ref.name;
			PRINT(("ThemeManager: trying to load_add_on(%s)\n", p.String()));
			img = load_add_on(imgpath.Path());
			if (img < B_OK)
				fprintf(stderr, "ThemeManager: load_add_on 0x%08lx\n", img);
			if (img < B_OK)
				continue;
			err = get_image_symbol(img, "instantiate_themes_addon", 
							B_SYMBOL_TYPE_TEXT, (void **)&instantiate_func);
			if (err)
				fprintf(stderr, "ThemeManager: get_image_symbol 0x%08lx\n", err);
			if (err)	continue;
			ta = instantiate_func();
			if (!ta)
				fprintf(stderr, "ThemeManager: instantiate_themes_addon returned NULL\n");
			if (!ta)
				continue;
			ta->SetImageId(img);
			/* check for existing names */
			for (i = 0; i < fAddonList.CountItems(); i++) {
				ThemesAddon *a = AddonAt(i);
				if (!a)
					fprintf(stderr, "ThemeManager: screwed! addon@%ld null\n", i);
				if (a->MessageName() && ta->MessageName() && 
							!strcmp(a->MessageName(), ta->MessageName())) {
					fprintf(stderr, "ThemeManager: screwed! addon@%ld has same msgname\n", i);
					screwed = true;
					break;
				}
				if (a->Name() && ta->Name() && 
							!strcmp(a->Name(), ta->Name())) {
					fprintf(stderr, "ThemeManager: screwed! addon@%ld has same name\n", i);
					screwed = true;
					break;
				}
			}
			if (screwed)
				continue;
			/* add it to the list */
			fAddonList.AddItem((void *)ta);
			PRINT(("ThemeManager: Added addon %ld '%s', msgname '%s'\n", ta->ImageId(), ta->Name(), ta->MessageName()));
		}
		//if (err)	return err;
	}
#else

#define ADDA(a) \
	if (ta) { \
		fAddonList.AddItem((void *)ta); \
		PRINT(("ThemeManager: Added addon %ld '%s', msgname '%s'\n", ta->ImageId(), ta->Name(), ta->MessageName())); \
	}


	ta = instantiate_themes_addon_backgrounds();
	ADDA(ta);
	ta = instantiate_themes_addon_beide();
	ADDA(ta);
	ta = instantiate_themes_addon_deskbar();
	ADDA(ta);
#ifndef ZETA_ADDONS
	ta = instantiate_themes_addon_eddie();
	ADDA(ta);
#endif
	ta = instantiate_themes_addon_pe();
	ADDA(ta);
	ta = instantiate_themes_addon_screensaver();
	ADDA(ta);
#ifndef ZETA_ADDONS
	ta = instantiate_themes_addon_soundplay();
	ADDA(ta);
#endif
	ta = instantiate_themes_addon_sounds();
	ADDA(ta);
	ta = instantiate_themes_addon_terminal();
	ADDA(ta);
#if defined(__HAIKU__) || defined(B_BEOS_VERSION_DANO)
	ta = instantiate_themes_addon_ui_settings();
	ADDA(ta);
#endif
#ifndef ZETA_ADDONS
	ta = instantiate_themes_addon_winamp_skin();
	ADDA(ta);
#endif
	ta = instantiate_themes_addon_window_decor();
	ADDA(ta);
#endif
	//if (err)	return err;
	fAddonCount = fAddonList.CountItems();
	PRINT(("ThemeManager: %ld addons loaded\n", fAddonCount));
	return B_OK;
}


status_t
ThemeManager::UnloadAddons()
{
	FENTRY;
	int32 i;
	ThemesAddon *ta;
	image_id img;
	for (i = 0; i < fAddonCount; i++) {
		ta = (ThemesAddon *)fAddonList.ItemAt(i);
		img = ta->ImageId();
		delete ta;
#ifndef SINGLE_BINARY
		unload_add_on(img);
#endif
	}
	fAddonList.MakeEmpty();
	fAddonCount = 0;
	return B_OK;
}


int32
ThemeManager::FindAddon(const char *name)
{
	FENTRY;
	int32 i;
	ThemesAddon *ta;
	for (i = 0; i < fAddonCount; i++) {
		ta = (ThemesAddon *)fAddonList.ItemAt(i);
		if (!ta)
			continue;
		if (!strcmp(ta->Name(), name))
			return i;
	}
	return -1;
}


int32
ThemeManager::CountAddons()
{
	FENTRY;
	return fAddonCount;
}


ThemesAddon *
ThemeManager::AddonAt(int32 addon)
{
	FENTRY;
	if (addon < 0)
		return NULL;
	return (ThemesAddon *)fAddonList.ItemAt(addon);
}


const char *
ThemeManager::AddonName(int32 addon)
{
	FENTRY;
	if (addon < 0)
		return NULL;
	return ((ThemesAddon *)fAddonList.ItemAt(addon))->Name();
}


const char *
ThemeManager::AddonDescription(int32 addon)
{
	FENTRY;
	if (addon < 0)
		return NULL;
	return ((ThemesAddon *)fAddonList.ItemAt(addon))->Description();
}


const char	*AddonName(int32 addon);


BView *
ThemeManager::OptionsView(int32 addon)
{
	FENTRY;
	ThemesAddon *ta;
	ta = AddonAt(addon);
	if (ta)
		return ta->OptionsView();
	return NULL;
}


status_t
ThemeManager::RunPreferencesPanel(int32 addon)
{
	FENTRY;
	ThemesAddon *ta;
	ta = AddonAt(addon);
	if (ta)
		return ta->RunPreferencesPanel();
	return B_OK;
}


status_t
ThemeManager::LoadSettings()
{
	FENTRY;
	BMessage addonSettings;
	int32 i;
	ThemesAddon *ta;
	uint32 addonFlags;
	BPath path;
	BFile sFile;
	status_t err;
	
	/* load prefs */
	err = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (!err) {
		path.Append(Z_THEME_MANAGER_SETTINGS_FILE);
		err = sFile.SetTo(path.Path(), B_READ_ONLY);
		if (!err) {
			err = fSettings.Unflatten(&sFile);
		}
	}
	
	/* tell addons */
	for (i = 0; i < fAddonCount; i++) {
		ta = AddonAt(i);
		if (!ta || !ta->Name())
			continue;
		if (fSettings.FindMessage(ta->Name(), &addonSettings) < B_OK) {
			addonSettings.MakeEmpty();
		}
		fSettings.RemoveName(ta->Name());
		if (addonSettings.FindInt32("ta:flags", (int32 *)&addonFlags) < B_OK) {
			addonFlags = Z_THEME_ADDON_DO_SET_ALL | Z_THEME_ADDON_DO_RETRIEVE;
			addonSettings.AddInt32("ta:flags", addonFlags);
		}
		ta->LoadSettings(addonSettings);
	}
	return B_OK;
}


status_t
ThemeManager::SaveSettings()
{
	FENTRY;
	BMessage addonSettings;
	int32 i;
	ThemesAddon *ta;
	BPath path;
	BFile sFile;
	status_t err;
	
	/* ask addons for their prefs */
	for (i = 0; i < fAddonCount; i++) {
		ta = AddonAt(i);
		if (!ta)
			continue;
		addonSettings.MakeEmpty();
		if (ta->SaveSettings(addonSettings) == B_OK)
			fSettings.AddMessage(ta->Name(), &addonSettings);
	}
	
	/* save the prefs */
	err = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (err)	return err;
	
	path.Append(Z_THEME_MANAGER_SETTINGS_FILE);
	err = sFile.SetTo(path.Path(), B_WRITE_ONLY|B_CREATE_FILE);
	if (err)	return err;
	
	err = fSettings.Flatten(&sFile);
	if (err)	return err;

	return B_OK;
}


void
ThemeManager::SetAddonFlags(int32 addon, uint32 flags)
{
	FENTRY;
	ThemesAddon *ta;
	ta = AddonAt(addon);
	if (ta)
		ta->SetAddonFlags(flags);
}


uint32
ThemeManager::AddonFlags(int32 addon)
{
	FENTRY;
	ThemesAddon *ta;
	ta = AddonAt(addon);
	if (ta)
		return ta->AddonFlags();
	return 0L;
}


status_t
ThemeManager::AddNames(BMessage &names)
{
	FENTRY;
	int32 i;
	ThemesAddon *ta;
	status_t err, gerr = B_OK;
	for (i = 0; i < fAddonCount; i++) {
		ta = (ThemesAddon *)fAddonList.ItemAt(i);
		if (!ta)
			continue;
		err = ta->AddNames(names);
		if (err)
			gerr = err;
	}
	return gerr;
}


status_t
ThemeManager::GetNames(BMessage &names)
{
	FENTRY;
	names = fNames;
	return B_OK;
}


status_t
ThemeManager::LoadThemes()
{
	FENTRY;
	int dirwhich;
	BPath path;
	BDirectory dir;
	entry_ref ref;
	status_t err;
	
	for (dirwhich = 0; dirwhich < 2; dirwhich++) {
		if (!dirwhich)	/* find system settings dir */
			err = find_directory(B_BEOS_ETC_DIRECTORY, &path);
		else			/* find user settings dir */
			err = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
		if (err)	return err;
		
		err = dir.SetTo(path.Path());
		if (err)	return err;
		BEntry ent;
		if (dir.FindEntry(Z_THEMES_FOLDER_NAME, &ent) < B_OK) {
			dir.CreateDirectory(Z_THEMES_FOLDER_NAME, NULL);
		}
		
		path.Append(Z_THEMES_FOLDER_NAME);
		
		err = dir.SetTo(path.Path());
		if (err)	return err;
		
		err = dir.Rewind();
		if (err)	return err;
		
		while ((err = dir.GetNextRef(&ref)) == B_OK) {
			BPath themepath(&ref);
			BDirectory tdir(themepath.Path());
			err = tdir.InitCheck();
			if (err) /* not a dir */
				continue;
			err = LoadTheme(themepath.Path());
		}
	}
	return B_OK;
}


status_t
ThemeManager::AddTheme(BMessage *theme)
{
	FENTRY;
	BString name;

	if (!theme)
		return EINVAL;
/*	err = ThemeName(*theme, name);
	if (err)
		return err;*/
	if (FindTheme(name.String()) >= 0)
		return B_FILE_EXISTS;
	fThemeList.AddItem(theme);
	return B_OK;
}


status_t
ThemeManager::UnloadThemes()
{
	FENTRY;
	int32 i;
	BMessage *theme;

	for (i = 0; i < CountThemes(); i++) {
		theme = (BMessage *)fThemeList.ItemAt(i);
		delete theme;
	}
	fThemeList.MakeEmpty();
	return B_OK;
}


int32
ThemeManager::CountThemes()
{
	FENTRY;

	return fThemeList.CountItems();
}


BMessage *
ThemeManager::ThemeAt(int32 id, bool allowbackup)
{
	FENTRY;
	BMessage *theme;

	if (id < 0) {
		if (allowbackup && (id == THEME_ID_BACKUP) && (!fBackupTheme.IsEmpty()))
			return &fBackupTheme;
		return NULL;
	}
	theme = (BMessage *)fThemeList.ItemAt(id);
	return theme;
}


status_t
ThemeManager::SetThemeAt(int32 id, BMessage &copyfrom)
{
	FENTRY;
	BMessage *theme;
	BMessage finfos;
	BMessage tinfos;
	BString s, s2;
	int32 i;

	if (id < 0)
		return EINVAL;
	theme = (BMessage *)fThemeList.ItemAt(id);
	if (!theme)
		return ENOENT;
	copyfrom.FindMessage(Z_THEME_INFO_MESSAGE, &finfos);
	copyfrom.RemoveName(Z_THEME_INFO_MESSAGE);
	copyfrom.RemoveName(Z_THEME_LOCATION);
	theme->FindMessage(Z_THEME_INFO_MESSAGE, &tinfos);
	tinfos.RemoveName(Z_THEME_MODULE_TAG);
	for (i = 0; finfos.FindString(Z_THEME_MODULE_TAG, i, &s); i++)
		tinfos.AddString(Z_THEME_MODULE_TAG, s);
	*theme = copyfrom;
	return B_OK;
}


int32
ThemeManager::FindTheme(const char *name)
{
	FENTRY;
	int32 i;
	BMessage *theme;
	BString tname;

	for (i = 0; i < CountThemes(); i++) {
		theme = (BMessage *)fThemeList.ItemAt(i);
		if (!theme)
			continue;
		if (ThemeName(i, tname) < B_OK)
			return -1L;
		if (!strcmp(tname.String(), name))
			return i;
	}
	return -1L;
}


status_t
ThemeManager::CurrentTheme(BMessage &copyto)
{
	FENTRY;

	if (!strlen(fCurrentThemeName.String()))
		return B_NAME_NOT_FOUND;
	if (!fBackupTheme.IsEmpty()) {
		copyto = fBackupTheme;
		return B_OK;
	}
/*	i = FindTheme(fCurrentThemeName.String());
	if (i < 0)
		return B_NAME_NOT_FOUND;
	err = ThemeAt(i, fBackupTheme);
	if (err)
		return err;
	copyto = fBackupTheme;*/
	// FIXME
	return B_OK;
}


status_t
ThemeManager::ApplyTheme(int32 id, uint32 flags)
{
	FENTRY;
	int32 i;
	ThemesAddon *ta;
	status_t err, gerr = B_OK;
	BMessage *theme = ThemeAt(id);
	if (!theme)
		return EINVAL;
	err = BackupCurrent();
	/*if (err)
		return err;*/
	for (i = 0; i < fAddonCount; i++) {
		ta = (ThemesAddon *)fAddonList.ItemAt(i);
		if (!ta)
			continue;
		PRINT(("ThemeManager: addon[%s]->Flags = 0x%08lx\n", ta->Name(), ta->AddonFlags()));
		if (!(ta->AddonFlags() & Z_THEME_ADDON_DO_SET_ALL))
			continue;
		err = ta->ApplyTheme(*theme, flags);
		if (err)
			gerr = err;
	}
	return gerr;
}


int32
ThemeManager::MakeTheme(uint32 flags)
{
	FENTRY;
	int32 i, id;
	ThemesAddon *ta;
	status_t err, gerr = B_OK;
	BMessage *theme;
	theme = new BMessage(Z_THEME_MESSAGE_WHAT);
	fThemeList.AddItem(theme);
	id = fThemeList.IndexOf(theme);
	if (id < 0) {
		delete theme;
		return -1;
	}
	for (i = 0; i < fAddonCount; i++) {
		ta = (ThemesAddon *)fAddonList.ItemAt(i);
		if (!ta)
			continue;
		if (!(ta->AddonFlags() & Z_THEME_ADDON_DO_RETRIEVE))
			continue;
		err = ta->MakeTheme(*theme, flags);
		if (err)
			gerr = err;
	}
	return id;
}


status_t
ThemeManager::UpdateTheme(int32 id, uint32 flags)
{
	FENTRY;
	int32 i;
	ThemesAddon *ta;
	status_t err, gerr = B_OK;
	BMessage *theme;
	BMessage infos;
	BString loc;
	if ((id < 0) || (id >= fThemeList.CountItems()))
		return EINVAL;
	theme = (BMessage *)fThemeList.ItemAt(id);
	if (!theme)
		return -1;
	err = theme->FindMessage(Z_THEME_INFO_MESSAGE, &infos);
	err = theme->FindString(Z_THEME_LOCATION, &loc);
	if (err)
		loc = "";
	theme->MakeEmpty();
	err = theme->AddMessage(Z_THEME_INFO_MESSAGE, &infos);
	err = theme->AddString(Z_THEME_LOCATION, loc.String());
	if (err)
		return err;
	for (i = 0; i < fAddonCount; i++) {
		ta = (ThemesAddon *)fAddonList.ItemAt(i);
		if (!ta)
			continue;
		if (!(ta->AddonFlags() & Z_THEME_ADDON_DO_RETRIEVE))
			continue;
		err = ta->MakeTheme(*theme, flags);
		if (err)
			gerr = err;
	}
	return B_OK;//gerr;
}


status_t
ThemeManager::DeleteTheme(int32 id)
{
	status_t err;
	BMessage *theme;
	BString loc;
	if ((id < 0) || (id >= fThemeList.CountItems()))
		return EINVAL;
	theme = (BMessage *)fThemeList.RemoveItem(id);
	fThemeList.AddItem(NULL, id); /* keep the place occupied */
	if (!theme)
		return -1;
	
	err = theme->FindString(Z_THEME_LOCATION, &loc);
	if (!err) {
		// move the files to trash...
		BEntry ent(loc.String());
		if (ent.InitCheck() == B_OK) {
			BPath trash;
			err = find_directory(B_TRASH_DIRECTORY, &trash);
			if (!err) {
				BDirectory trashDir(trash.Path());
				if (trashDir.InitCheck() == B_OK) {
					BNode nod(&ent);
					if (nod.InitCheck() == B_OK) {
						BPath path;
						err = ent.GetPath(&path);
						if (!err) {
							nod.WriteAttr("_trk/original_path", B_STRING_TYPE, 0LL, path.Path(), strlen(path.Path())+1);
						}
					}
					err = ent.MoveTo(&trashDir, NULL);  // don't clober
					if (err) {
						BString newname("Theme-renamed-");
						newname << system_time();
						err = ent.MoveTo(&trashDir, newname.String(), true);
					}
				}
			}
		}
	}
	
	if (id == fSelectedTheme)
		fSelectedTheme = -1;
	delete theme;
	return B_OK;
}


int32
ThemeManager::SelectedTheme()
{
	BMessage *theme;
	theme = ThemeAt(fSelectedTheme);
	if (!theme)
		return -1;
	// XXX: should really check the diff with current.
	return fSelectedTheme;
}


status_t
ThemeManager::ApplyDefaultTheme(uint32 flags)
{
	FENTRY;
	int32 i;
	ThemesAddon *ta;
	status_t err, gerr = B_OK;
	err = BackupCurrent();
	if (err)
		return err;
	for (i = 0; i < fAddonCount; i++) {
		ta = (ThemesAddon *)fAddonList.ItemAt(i);
		if (!ta)
			continue;
		if (!(ta->AddonFlags() & Z_THEME_ADDON_DO_SET_ALL))
			continue;
		err = ta->ApplyDefaultTheme(flags);
		if (err)
			gerr = err;
	}
	return gerr;
}


status_t
ThemeManager::BackupCurrent()
{
	FENTRY;
	int32 i;
	ThemesAddon *ta;
	status_t err, gerr = B_OK;
	fBackupTheme.MakeEmpty();
	for (i = 0; i < fAddonCount; i++) {
		ta = (ThemesAddon *)fAddonList.ItemAt(i);
		if (!ta)
			continue;
		if (!(ta->AddonFlags() & Z_THEME_ADDON_DO_RETRIEVE))
			continue;
		err = ta->BackupCurrent(fBackupTheme);
		if (err)
			gerr = err;
	}
	return gerr;
}


status_t
ThemeManager::RestoreCurrent()
{
	FENTRY;
	int32 i;
	ThemesAddon *ta;
	status_t err, gerr = B_OK;
	if (fBackupTheme.IsEmpty())
		return ENOENT;
	for (i = 0; i < fAddonCount; i++) {
		ta = (ThemesAddon *)fAddonList.ItemAt(i);
		if (!ta)
			continue;
		if (!(ta->AddonFlags() & Z_THEME_ADDON_DO_SET_ALL))
			continue;
		err = ta->RestoreCurrent(fBackupTheme);
		if (err)
			gerr = err;
	}
	fBackupTheme.MakeEmpty();
	return gerr;
}


bool
ThemeManager::CanRevert()
{
	FENTRY;
	return !fBackupTheme.IsEmpty();
}


status_t
ThemeManager::CompareToCurrent(BMessage &theme)
{
	FENTRY;
	int32 i;
	ThemesAddon *ta;
	status_t err;

	for (i = 0; i < fAddonCount; i++) {
		ta = (ThemesAddon *)fAddonList.ItemAt(i);
		if (!ta)
			continue;
		if (!(ta->AddonFlags() & Z_THEME_ADDON_DO_RETRIEVE))
			continue;
		err = ta->CompareToCurrent(theme);
		if (err)
			return err;
	}
	return B_OK;
}

	
status_t
ThemeManager::InstallFiles(BMessage &theme, BDirectory &folder)
{
	FENTRY;
	int32 i;
	ThemesAddon *ta;
	status_t err, gerr = B_OK;
	for (i = 0; i < fAddonCount; i++) {
		ta = (ThemesAddon *)fAddonList.ItemAt(i);
		if (!ta)
			continue;
		if (!(ta->AddonFlags() & Z_THEME_ADDON_DO_SET_ALL))
			continue;
		err = ta->InstallFiles(theme, folder);
		if (err)
			gerr = err;
	}
	return gerr;
}


status_t
ThemeManager::BackupFiles(BMessage &theme, BDirectory &folder)
{
	FENTRY;
	int32 i;
	ThemesAddon *ta;
	status_t err, gerr = B_OK;
	for (i = 0; i < fAddonCount; i++) {
		ta = (ThemesAddon *)fAddonList.ItemAt(i);
		if (!ta)
			continue;
		if (!(ta->AddonFlags() & Z_THEME_ADDON_DO_RETRIEVE))
			continue;
		err = ta->BackupFiles(theme, folder);
		if (err)
			gerr = err;
	}
	return gerr;
}


status_t
ThemeManager::SaveTheme(int32 id, bool excl)
{
	FENTRY;
	status_t err;
	BString name, fname;
	BPath path;
	BDirectory dir;
	BDirectory tdir;
	BFile tfile;
	BMessage names;
	BString location;
	BMessage *theme;
	theme = ThemeAt(id);
	if (!theme)
		return EINVAL;
	err = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (err)	return err;
	path.Append(Z_THEMES_FOLDER_NAME);
	err = dir.SetTo(path.Path());
	if (err)	return err;
	err = ThemeName(id, name);
	if (err)	return err;
	fname = name;
	NormalizeThemeFolderName(fname);

	err = ThemeLocation(id, location);
	if (!err) {
		if (location.FindFirst("/boot/beos") >= 0) {
			PRINT(("trying to save theme '%s' to system dir!\n", name.String()));
			return B_PERMISSION_DENIED;
		}
	}
	path.Append(fname.String());
	err = theme->ReplaceString(Z_THEME_LOCATION, path.Path());
	if (err)
		err = theme->AddString(Z_THEME_LOCATION, path.Path());
	
	if (dir.CreateDirectory(fname.String(), NULL) < B_OK) {
		if (excl)
			return B_FILE_EXISTS;
	}
	err = tdir.SetTo(&dir, fname.String());
	if (err)	return err;
	err = tdir.CreateFile(Z_THEME_FILE_NAME, &tfile);
	if (err)	return err;
	
	BMessage tosave(*theme);
	err = tosave.RemoveName(Z_THEME_LOCATION);
	err = GetNames(names);
	
	err = DumpMessageToStream(&tosave, tfile, 0, &names);
	if (err)	return err;
	return B_OK;
}


status_t
ThemeManager::PackageTheme(BMessage &theme)
{
	PRINT(("UNIMPLEMENTED %s()\n", __FUNCTION__));

	(void)theme;
	return B_OK;
}


status_t
ThemeManager::LoadTheme(const char *path, BMessage **to)
{
	status_t err;
	BDirectory dir;
	BFile f;
	BMessage *theme;
#ifdef DEBUG_TM
	PRINT(("ThemeManager::%s(%s)\n", __FUNCTION__, path));
#endif
	
	err = dir.SetTo(path);
	if (err)
		fprintf(stderr, "ThemeManager:: BDirectory::SetTo 0x%08lx\n", err);
	if (err) { /* not a dir, check for a zip */
		// TODO
		// unzip + recursive call
		return err;
	}
	err = f.SetTo(&dir, Z_THEME_FILE_NAME, B_READ_ONLY);
	if (err)
		fprintf(stderr, "ThemeManager:: BFile::SetTo 0x%08lx\n", err);
	if (err)
		return err;
	err = ParseMessageFromStream(&theme, f);
	if (err)
		fprintf(stderr, "ThemeManager:: ParseMessageFromStream 0x%08lx\n", err);
	if (err)
		return err;
	err = theme->RemoveName(Z_THEME_LOCATION);
	err = theme->AddString(Z_THEME_LOCATION, path);
	//theme->PrintToStream();
	err = AddTheme(theme);
	if (err)
		fprintf(stderr, "ThemeManager:: AddTheme 0x%08lx\n", err);
	if (err)
		delete theme;
	if (!err && to)
		*to = theme;
	return err;
}


int32
ThemeManager::CountThemeImporters()
{
	FENTRY;
	return fThemeImporters.CountItems();
}


const char *
ThemeManager::ThemeImporterAt(int32 index)
{
	FENTRY;
	ThemeImporter *importer;
	importer = static_cast<ThemeImporter *>(fThemeImporters.ItemAt(index));
	if (!importer)
		return NULL;
	return importer->Name();
}


status_t
ThemeManager::ImportThemesFor(int32 index, const char *path)
{
	FENTRY;
	status_t err;
	int32 count;
	BString m;
	int32 i;
	ThemeImporter *importer;
	BMessage msg;
	BMessage *theme;
	
	importer = static_cast<ThemeImporter *>(fThemeImporters.ItemAt(index));
	if (!importer)
		return ENOENT;

	err = importer->FetchThemes();
	if (err < 0)
		return err;
	while ((err = importer->ImportNextTheme(&theme)) != ENOENT) {
		if (err >= 0)
			AddTheme(theme);
	}
	importer->EndImports();
	
	return B_OK;
}


bool
ThemeManager::ThemeHasInfoFor(int32 id, BString &module)
{
	FENTRY;
	status_t err;
	BString m;
	int32 i;
	BMessage msg;
	BMessage *theme;
	
	if (id < 0)
		return EINVAL;
	theme = (BMessage *)fThemeList.ItemAt(id);
	if (!theme)
		return EINVAL;
	
	err = theme->FindMessage(Z_THEME_INFO_MESSAGE, &msg);
	if (err)
		return false;
	for (i = 0; msg.FindString(Z_THEME_MODULE_TAG, i, &m) == B_OK; i++)
		if (m == module)
			return true;
	return false;
}


status_t
ThemeManager::ThemeName(int32 id, BString &copyto)
{
	FENTRY;
	status_t err;
	BString s;
	BMessage msg;
	BMessage *theme;
	
	if (id < 0)
		return EINVAL;
	theme = (BMessage *)fThemeList.ItemAt(id);
	if (!theme)
		return EINVAL;
	
	err = theme->FindMessage(Z_THEME_INFO_MESSAGE, &msg);
	if (err)
		return err;
	err = msg.FindString(Z_THEME_NAME, &s);
	if (err)
		return err;
	copyto = s;
	return B_OK;
}


status_t
ThemeManager::ThemeLocation(int32 id, BString &copyto)
{
	FENTRY;
	status_t err;
	BString s;
	BMessage *theme;
	
	if (id < 0)
		return EINVAL;
	theme = (BMessage *)fThemeList.ItemAt(id);
	if (!theme)
		return EINVAL;
	
	err = theme->FindString(Z_THEME_LOCATION, &s);
	if (err)
		return err;
	copyto = s;
	return B_OK;
}


bool
ThemeManager::ThemeIsReadOnly(int32 id)
{
	FENTRY;
	status_t err;
	BString s;
	BMessage *theme;
	
	if (id < 0)
		return true;
	
	theme = (BMessage *)fThemeList.ItemAt(id);
	if (!theme)
		return true;
	// imported themes are always RO for now
	if (theme->FindString(Z_THEME_IMPORTER, &s) >= B_OK)
		return true;
	
	err = ThemeLocation(id, s);
	if (err)
		return true;
	if (s.FindFirst("/boot/beos") >= 0)
		return true;
	return false;
}


status_t
ThemeManager::ThemeDescription(int32 id, BString &copyto)
{
	FENTRY;
	status_t err;
	BString s;
	BMessage msg;
	BMessage *theme;
	
	if (id < 0)
		return EINVAL;
	theme = (BMessage *)fThemeList.ItemAt(id);
	if (!theme)
		return EINVAL;
	
	err = theme->FindMessage(Z_THEME_INFO_MESSAGE, &msg);
	if (err)
		return err;
	err = msg.FindString(Z_THEME_DESCRIPTION, &s);
	if (err)
		return err;
	copyto = s;
	return B_OK;
}


status_t
ThemeManager::ThemeKeywords(int32 id, BString &copyto)
{
	FENTRY;
	status_t err;
	BString s;
	BMessage msg;
	BMessage *theme;
	
	if (id < 0)
		return EINVAL;
	theme = (BMessage *)fThemeList.ItemAt(id);
	if (!theme)
		return EINVAL;
	
	err = theme->FindMessage(Z_THEME_INFO_MESSAGE, &msg);
	if (err)
		return err;
	err = msg.FindString(Z_THEME_KEYWORDS, &s);
	if (err)
		return err;
	copyto = s;
	return B_OK;
}


status_t
ThemeManager::ThemeScreenShot(int32 id, BBitmap **copyto)
{
	FENTRY;
	status_t err;
	BString s, loc;
	BMessage msg;
	BEntry ent;
	entry_ref ref;
	BMessage *theme;
	
	if (id < 0)
		return EINVAL;
	theme = (BMessage *)fThemeList.ItemAt(id);
	if (!theme)
		return EINVAL;
	
	if (!copyto)
		return EINVAL;
	err = theme->FindMessage(Z_THEME_INFO_MESSAGE, &msg);
	if (err)
		return err;
	err = msg.FindString(Z_THEME_SCREENSHOT_FILENAME, &s);
	if (err)
		return err;
	// TODO
	err = ThemeLocation(id, loc);
	if (err)
		return err;
	loc << "/" << s;
	err = ent.SetTo(loc.String());
	if (err)
		return err;
	err = ent.GetRef(&ref);
	if (err)
		return err;
	*copyto = BTranslationUtils::GetBitmap(&ref);
	if (!(*copyto))
		return ENOENT;
	return B_OK;
}


status_t
ThemeManager::SetThemeHasInfoFor(int32 id, BString module)
{
	FENTRY;
	status_t err;
	BMessage msg;
	BMessage *theme;
	
	if (id < 0)
		return EINVAL;
	theme = (BMessage *)fThemeList.ItemAt(id);
	if (!theme)
		return EINVAL;
	
	err = theme->FindMessage(Z_THEME_INFO_MESSAGE, &msg);
	if (err) {
		msg.MakeEmpty();
		theme->AddMessage(Z_THEME_INFO_MESSAGE, &msg);
	}
	err = theme->AddString(Z_THEME_MODULE_TAG, module);
	if (err)
		return err;
	err = theme->ReplaceMessage(Z_THEME_INFO_MESSAGE, &msg);
	return err;
}


status_t
ThemeManager::SetThemeName(int32 id, BString name)
{
	FENTRY;
	status_t err;
	BMessage msg;
	BMessage *theme;
	
	if (id < 0)
		return EINVAL;
	theme = (BMessage *)fThemeList.ItemAt(id);
	if (!theme)
		return EINVAL;
	
	err = theme->FindMessage(Z_THEME_INFO_MESSAGE, &msg);
	if (err) {
		msg.MakeEmpty();
		theme->AddMessage(Z_THEME_INFO_MESSAGE, &msg);
	}
	err = msg.ReplaceString(Z_THEME_NAME, name);
	if (err)
		err = msg.AddString(Z_THEME_NAME, name);
	if (err)
		return err;
	err = theme->ReplaceMessage(Z_THEME_INFO_MESSAGE, &msg);
	return err;
}


status_t
ThemeManager::SetThemeDescription(int32 id, BString description)
{
	FENTRY;
	status_t err;
	BMessage msg;
	BMessage *theme;
	
	if (id < 0)
		return EINVAL;
	theme = (BMessage *)fThemeList.ItemAt(id);
	if (!theme)
		return EINVAL;
	
	err = theme->FindMessage(Z_THEME_INFO_MESSAGE, &msg);
	if (err) {
		msg.MakeEmpty();
		theme->AddMessage(Z_THEME_INFO_MESSAGE, &msg);
	}
	err = msg.ReplaceString(Z_THEME_DESCRIPTION, description);
	if (err)
		err = msg.AddString(Z_THEME_DESCRIPTION, description);
	if (err)
		return err;
	err = theme->ReplaceMessage(Z_THEME_INFO_MESSAGE, &msg);
	return err;
}


status_t
ThemeManager::SetThemeKeywords(int32 id, BString keywords)
{
	FENTRY;
	status_t err;
	BMessage msg;
	BMessage *theme;
	
	if (id < 0)
		return EINVAL;
	theme = (BMessage *)fThemeList.ItemAt(id);
	if (!theme)
		return EINVAL;
	
	err = theme->FindMessage(Z_THEME_INFO_MESSAGE, &msg);
	if (err) {
		msg.MakeEmpty();
		theme->AddMessage(Z_THEME_INFO_MESSAGE, &msg);
	}
	err = msg.ReplaceString(Z_THEME_KEYWORDS, keywords);
	if (err)
		err = msg.AddString(Z_THEME_KEYWORDS, keywords);
	if (err)
		return err;
	err = theme->ReplaceMessage(Z_THEME_INFO_MESSAGE, &msg);
	return err;
}


status_t
ThemeManager::SetThemeScreenShot(int32 id, BBitmap *bitmap)
{
	FENTRY;
	status_t err;
	BMessage msg;
	BString name;
	BString themepath;
	BMessage *theme;
	
	if (id < 0)
		return EINVAL;
	theme = (BMessage *)fThemeList.ItemAt(id);
	if (!theme)
		return EINVAL;
	
	// TODO
	err = theme->FindMessage(Z_THEME_INFO_MESSAGE, &msg);
	if (err) {
		msg.MakeEmpty();
		theme->AddMessage(Z_THEME_INFO_MESSAGE, &msg);
	}
	err = ThemeLocation(id, themepath);
	if (err)
		return err;
	err = msg.FindString(Z_THEME_SCREENSHOT_FILENAME, &name);
	if (!err) {
		BPath spath(themepath.String());
		spath.Append(name.String());
		BEntry ent(spath.Path());
		if (ent.InitCheck() == B_OK)
			ent.Remove();
	}
	
	name = "screenshot.png";
	err = msg.ReplaceString(Z_THEME_SCREENSHOT_FILENAME, name);
	if (err)
		err = msg.AddString(Z_THEME_SCREENSHOT_FILENAME, name);
	if (err)
		return err;
	
	// save the BBitmap to a png
	BPath spath(themepath.String());
	spath.Append(name.String());
	BFile shotfile(spath.Path(), B_WRITE_ONLY|B_CREATE_FILE);
	if (shotfile.InitCheck() != B_OK)
		return shotfile.InitCheck();
	BTranslatorRoster *troster = BTranslatorRoster::Default();
	BBitmapStream bmstream(bitmap);
	err = troster->Translate(&bmstream, NULL, NULL, &shotfile, 'PNG '/* XXX: hack, should find by mime type */);
	if (err)
		return err;
	
	err = theme->ReplaceMessage(Z_THEME_INFO_MESSAGE, &msg);
	msg.PrintToStream();
	return err;
}


status_t
ThemeManager::MakeThemeScreenShot(int32 id)
{
	FENTRY;
	status_t err;
	BBitmap *bmp = NULL;
	BMessage *theme;
	
	if (id < 0)
		return EINVAL;
	theme = (BMessage *)fThemeList.ItemAt(id);
	if (!theme)
		return EINVAL;
	
	// TODO
	err = MakeScreenshot(&bmp);
	if (err)
		return err;
	err = SetThemeScreenShot(id, bmp);
	
	return err;
}


void
ThemeManager::NormalizeThemeFolderName(BString &name)
{
	FENTRY;
	name.RemoveSet("\"\\'/&:*~#|`^");
}

