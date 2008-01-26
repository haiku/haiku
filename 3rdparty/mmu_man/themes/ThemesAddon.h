/*
 * ThemesAddon class header
 */

#include <image.h>
class BView;
class BMessage;
class BDirectory;

namespace Z {
namespace ThemeManager {

class ThemeManager;

class ThemesAddon {
public:
			ThemesAddon(const char *name, const char *message_name);
virtual		~ThemesAddon();
	
	/* presentation */
	
virtual	const char	*Name(); /* pretty name */
virtual const char	*Description(); /* tooltip... */
	
virtual BView		*OptionsView();
virtual status_t	RunPreferencesPanel();
					// if you override, call it first
virtual status_t	LoadSettings(BMessage &settings);
					// if you override, call it last
virtual status_t	SaveSettings(BMessage &settings);
void		SetAddonFlags(uint32 flags);
uint32		AddonFlags();
virtual status_t	AddNames(BMessage &names);
	
	/* Theme manipulation */
	
					// name of the submessage in the theme, if any
virtual	const char	*MessageName();
					// retrieve the addons specific message if any
virtual	status_t	MyMessage(BMessage &theme, BMessage &mine);
virtual	status_t	SetMyMessage(BMessage &theme, BMessage &mine);

virtual status_t	ApplyTheme(BMessage &theme, uint32 flags=0L);
virtual status_t	MakeTheme(BMessage &theme, uint32 flags=0L);

virtual status_t	ApplyDefaultTheme(uint32 flags=0L);

virtual status_t	BackupCurrent(BMessage &theme);
virtual status_t	RestoreCurrent(BMessage &theme);

virtual status_t	CompareToCurrent(BMessage &theme);
	
	/* Theme installation */
	
virtual status_t	InstallFiles(BMessage &theme, BDirectory &folder);
virtual status_t	BackupFiles(BMessage &theme, BDirectory &folder);
	
	/*  */
	
private:

friend class Z::ThemeManager::ThemeManager;
	void		SetImageId(image_id id);
	image_id	ImageId();

	image_id	fImageId;
	char	*fName;
	char	*fMsgName;
	uint32	fFlags;
	BMessage fSettings;
};

} // ns ThemeManager
} // ns Z

extern "C" Z::ThemeManager::ThemesAddon *instantiate_themes_addon();

#ifdef SINGLE_BINARY
extern "C" Z::ThemeManager::ThemesAddon *instantiate_themes_addon_backgrounds();
extern "C" Z::ThemeManager::ThemesAddon *instantiate_themes_addon_beide();
extern "C" Z::ThemeManager::ThemesAddon *instantiate_themes_addon_deskbar();
extern "C" Z::ThemeManager::ThemesAddon *instantiate_themes_addon_eddie();
extern "C" Z::ThemeManager::ThemesAddon *instantiate_themes_addon_pe();
extern "C" Z::ThemeManager::ThemesAddon *instantiate_themes_addon_screensaver();
extern "C" Z::ThemeManager::ThemesAddon *instantiate_themes_addon_soundplay();
extern "C" Z::ThemeManager::ThemesAddon *instantiate_themes_addon_sounds();
extern "C" Z::ThemeManager::ThemesAddon *instantiate_themes_addon_terminal();
extern "C" Z::ThemeManager::ThemesAddon *instantiate_themes_addon_ui_settings();
extern "C" Z::ThemeManager::ThemesAddon *instantiate_themes_addon_winamp_skin();
extern "C" Z::ThemeManager::ThemesAddon *instantiate_themes_addon_window_decor();
#endif

/* in B_*_ADDONS_DIRECTORY */
#define Z_THEMES_ADDON_FOLDER "ThemeManager"

using namespace Z::ThemeManager;

