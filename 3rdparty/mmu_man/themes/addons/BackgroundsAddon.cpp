/*
 * backgrounds ThemesAddon class
 */

#include <Application.h>
#include <fs_attr.h>
#include <File.h>
#include <FindDirectory.h>
#include <InterfaceDefs.h>
#include <Directory.h>
#include <Message.h>
#include <Messenger.h>
#include <Roster.h>
#include <Screen.h>
#include <String.h>
#include <TypeConstants.h>
#include <be_apps/Tracker/Background.h>
#include <storage/Path.h>

#include <string.h>

#include "ThemesAddon.h"
#include "UITheme.h"
#include "Utils.h"

#ifdef SINGLE_BINARY
#define instanciate_themes_addon instanciate_themes_addon_backgrounds
#endif

#define A_NAME "Backgrounds"
#define A_MSGNAME Z_THEME_BACKGROUND_SETTINGS
#define A_DESCRIPTION "Desktop backdrops"

#define B__DESKTOP_COLOR "be:desktop_color"

class BackgroundThemesAddon : public ThemesAddon {
public:
	BackgroundThemesAddon();
	~BackgroundThemesAddon();
	
const char *Description();

status_t	RunPreferencesPanel();

status_t	AddNames(BMessage &names);

status_t	ApplyTheme(BMessage &theme, uint32 flags=0L);
status_t	MakeTheme(BMessage &theme, uint32 flags=0L);

status_t	ApplyDefaultTheme(uint32 flags=0L);

//status_t	CompareToCurrent(BMessage &theme);
	
	/* Theme installation */
	
status_t	InstallFiles(BMessage &theme, BDirectory &folder);
status_t	BackupFiles(BMessage &theme, BDirectory &folder);
	
};

BackgroundThemesAddon::BackgroundThemesAddon()
	: ThemesAddon(A_NAME, A_MSGNAME)
{
}

BackgroundThemesAddon::~BackgroundThemesAddon()
{
}

const char *BackgroundThemesAddon::Description()
{
	return A_DESCRIPTION;
}

status_t	BackgroundThemesAddon::RunPreferencesPanel()
{
/*	if (be_roster->Launch("application/x-vnd.obos-Backgrounds") < B_OK)
		if (be_roster->Launch("application/x-vnd.Be-Backgrounds") < B_OK)
			return B_ERROR;*/
	status_t err;
	entry_ref ref;
	BEntry ent;
	err = ent.SetTo("/boot/beos/preferences/Backgrounds");
	if (!err) {
		err = ent.GetRef(&ref);
		if (!err) {
			err = be_roster->Launch(&ref);
		}
	}
	if (!err)
		return B_OK;
	err = ent.SetTo("/system/add-ons/Preferences/Backgrounds");
	if (!err) {
		err = ent.GetRef(&ref);
		if (!err) {
			err = be_roster->Launch(&ref);
			if (err) {
				BMessage msg(B_REFS_RECEIVED);
				msg.AddRef("refs", &ref);
				be_app_messenger.SendMessage(&msg);
			}
		}
	}
	return err;
}

status_t BackgroundThemesAddon::AddNames(BMessage &names)
{
	names.AddString(Z_THEME_BACKGROUND_SETTINGS, "Desktop backgrounds and color; please respect count and ordering of all fields!");
	names.AddString(B_BACKGROUND_WORKSPACES, "Workspaces each backdrop apply to");
	names.AddString(B_BACKGROUND_IMAGE, "Actual backdrop image file");
	names.AddString(B_BACKGROUND_MODE, "image mode: 0=use_origin, 1=centered, 2=scaled, 3=tiled");
	names.AddString(B_BACKGROUND_ORIGIN, "Origin point for mode 0");
	names.AddString(B_BACKGROUND_ERASE_TEXT, "If true, use desktop color as icon text background");
	names.AddString(B__DESKTOP_COLOR, "one per workspace, up to the last changing one");
	return B_OK;
}

status_t BackgroundThemesAddon::ApplyTheme(BMessage &theme, uint32 flags)
{
	BMessage backgrounds;
	status_t err = B_OK;
	BPath pDesktop;
	ssize_t flatSize;
	char *pAttr;

	if (!(flags & UI_THEME_SETTINGS_SET_ALL) || !(AddonFlags() & Z_THEME_ADDON_DO_SET_ALL))
		return B_OK;

	err = MyMessage(theme, backgrounds);
	if (err)
		return err;

	// set desktop colors
	BScreen bs;
	rgb_color col;
	//const rgb_color *c;
	col = bs.DesktopColor(0); // by
	// should work as the rest of the fields (grouping)
	for (int i = 0; i < count_workspaces(); i++) {
		//ssize_t sz = 4;
		if (FindRGBColor(backgrounds, B__DESKTOP_COLOR, i, &col) != B_OK && i == 0)
		//if (backgrounds.FindData(B__DESKTOP_COLOR, (type_code)'RGBC', i, (const void **)&c, &sz) != B_OK && i == 0)
			break; // if no color at all, don't set them
		bs.SetDesktopColor(col, i, true);
	}
	// make sure we don't leave our garbage in the message
	// as it would offset the next call by adding rgb_colors on existing ones
	backgrounds.RemoveName(B__DESKTOP_COLOR);

	BMessenger tracker("application/x-vnd.Be-TRAK");
	BMessage m(B_RESTORE_BACKGROUND_IMAGE);

	if (find_directory(B_DESKTOP_DIRECTORY, &pDesktop) < B_OK)
		return EINVAL;
	BNode nDesktop(pDesktop.Path());
	if (nDesktop.InitCheck() < B_OK)
		return nDesktop.InitCheck();
	flatSize = backgrounds.FlattenedSize();
	pAttr = new char[flatSize];
	if (pAttr == NULL)
		return ENOMEM;
	err = backgrounds.Flatten(pAttr, flatSize);
	if (err < B_OK)
		goto getout;
	err = nDesktop.WriteAttr(B_BACKGROUND_INFO, B_MESSAGE_TYPE, 0LL, pAttr, flatSize);
	if (err < B_OK)
		goto getout;

	//m.AddSpecifier("Window", "Desktop");
	tracker.SendMessage(&m);

	return B_OK;

getout:
	delete [] pAttr;
	return err;
}

status_t BackgroundThemesAddon::MakeTheme(BMessage &theme, uint32 flags)
{
	BMessage backgrounds;
	status_t err = B_OK;
	BPath pDesktop;
	struct attr_info ai;
	char *pAttr;
	BScreen bs;
	rgb_color last_color = {0, 0, 0, 254};
	rgb_color col;
	int last_change = 0;
	int i;
	
	(void)flags;
	err = MyMessage(theme, backgrounds);
	if (err)
		backgrounds.MakeEmpty();
	
	if (find_directory(B_DESKTOP_DIRECTORY, &pDesktop) < B_OK)
		return EINVAL;
	BNode nDesktop(pDesktop.Path());
	if (nDesktop.InitCheck() < B_OK)
		return nDesktop.InitCheck();
	if (nDesktop.GetAttrInfo(B_BACKGROUND_INFO, &ai) < B_OK)
		return EIO;
	pAttr = new char[ai.size];
	if (pAttr == NULL)
		return ENOMEM;
	err = nDesktop.ReadAttr(B_BACKGROUND_INFO, ai.type, 0LL, pAttr, ai.size);
	if (err < B_OK)
		goto getout;
	err = backgrounds.Unflatten(pAttr);
	if (err < B_OK)
		goto getout;

	// make sure other colors are removed
	backgrounds.RemoveName(B__DESKTOP_COLOR);

	// get desktop color... XXX: move it in ui_settings ?
	// should work as the rest of the fields (grouping)
	for (i = 0; i < count_workspaces(); i++) {
		col = bs.DesktopColor(i);
		if (memcmp(&last_color, &col, sizeof(rgb_color))) {
			last_change = i;
			last_color = col;
		}
	}
	//printf("ws col cnt %d\n", last_change);
	for (i = 0; i <= last_change; i++) {
		col = bs.DesktopColor(i);
		AddRGBColor(backgrounds, B__DESKTOP_COLOR, col);
		//backgrounds->AddData(B__DESKTOP_COLOR, (type_code)'RGBC', &col, 4);
	}

	err = SetMyMessage(theme, backgrounds);
	return err;

getout:
	delete [] pAttr;
	return err;
}

status_t BackgroundThemesAddon::ApplyDefaultTheme(uint32 flags)
{
	BMessage theme;
	BMessage backgrounds;
	rgb_color col = {51,102,152,255}; // Be Blues... ;)
	backgrounds.AddBool(B_BACKGROUND_ERASE_TEXT, true);
	AddRGBColor(backgrounds, B__DESKTOP_COLOR, col);
	theme.AddMessage(A_MSGNAME, &backgrounds);
	return ApplyTheme(theme, flags);
}

status_t BackgroundThemesAddon::InstallFiles(BMessage &theme, BDirectory &folder)
{
	(void)theme; (void)folder;
	return B_OK;
}

status_t BackgroundThemesAddon::BackupFiles(BMessage &theme, BDirectory &folder)
{
	(void)theme; (void)folder;
	return B_OK;
}


ThemesAddon *instanciate_themes_addon()
{
	return (ThemesAddon *) new BackgroundThemesAddon;
}
