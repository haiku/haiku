/*
 * window_decor ThemesAddon class
 */

#include <BeBuild.h>
#ifdef B_BEOS_VERSION_DANO

#include <Alert.h>
#include <Application.h>
#include <Directory.h>
#include <Entry.h>
#include <InterfaceDefs.h>
#include <MediaFiles.h>
#include <Message.h>
#include <Messenger.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>

#include <stdio.h>
#include <string.h>

#include "ThemesAddon.h"
#include "UITheme.h"

#ifdef SINGLE_BINARY
#define instanciate_themes_addon instanciate_themes_addon_window_decor
#endif

#define A_NAME "Window Decor"
#define A_MSGNAME Z_THEME_WINDOW_DECORATIONS
#define A_DESCRIPTION "Window decorations and scrollbars"

class DecorThemesAddon : public ThemesAddon {
public:
	DecorThemesAddon();
	~DecorThemesAddon();
	
const char *Description();

status_t	RunPreferencesPanel();

status_t	AddNames(BMessage &names);

status_t	ApplyTheme(BMessage &theme, uint32 flags=0L);
status_t	MakeTheme(BMessage &theme, uint32 flags=0L);

status_t	ApplyDefaultTheme(uint32 flags=0L);
};

DecorThemesAddon::DecorThemesAddon()
	: ThemesAddon(A_NAME, A_MSGNAME)
{
}

DecorThemesAddon::~DecorThemesAddon()
{
}

const char *DecorThemesAddon::Description()
{
	return A_DESCRIPTION;
}

status_t	DecorThemesAddon::RunPreferencesPanel()
{
	status_t err;
	entry_ref ref;
	BEntry ent;
	err = ent.SetTo("/system/add-ons/Preferences/Appearance");
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

status_t DecorThemesAddon::AddNames(BMessage &names)
{
	names.AddString(Z_THEME_WINDOW_DECORATIONS, "Window decorations and scrollbars");
	names.AddString("window:decor", "Window decor");
	names.AddString("window:decor_globals", "Window decor parameters");
	return B_OK;
}

status_t DecorThemesAddon::ApplyTheme(BMessage &theme, uint32 flags)
{
	BMessage window_decor;
	BMessage globals;
	BString decorName;
	status_t err;

	if (!(flags & UI_THEME_SETTINGS_SET_ALL) || !(AddonFlags() & Z_THEME_ADDON_DO_SET_ALL))
		return B_OK;
	
	err = MyMessage(theme, window_decor);
	if (err)
		return err;
	
#ifdef B_BEOS_VERSION_DANO
	if (window_decor.FindString("window:decor", &decorName) == B_OK)
		set_window_decor(decorName.String(), 
			(window_decor.FindMessage("window:decor_globals", &globals) == B_OK)?&globals:NULL);
#else
extern void __set_window_decor(int32 theme);
	int32 decorNr = 0;
	if (window_decor.FindInt32("window:R5:decor", &decorNr) == B_OK)
		__set_window_decor(decorNr);
#endif
	// XXX: add colors à la WindowShade ?
	
	return B_OK;
}

status_t DecorThemesAddon::MakeTheme(BMessage &theme, uint32 flags)
{
	BMessage window_decor;
	BMessage globals;
	BString decorName;
	status_t err;
	
	(void)flags;
	err = MyMessage(theme, window_decor);
	if (err)
		window_decor.MakeEmpty();
	
#ifdef B_BEOS_VERSION_DANO
	err = get_window_decor(&decorName, &globals);
	if (err == B_OK) {
		window_decor.AddString("window:decor", decorName.String());
		window_decor.AddMessage("window:decor_globals", &globals);
	}
#else
	window_decor.AddInt32("window:R5:decor", 0);
#endif
	
	err = SetMyMessage(theme, window_decor);
	return err;
}

status_t DecorThemesAddon::ApplyDefaultTheme(uint32 flags)
{
	BMessage theme;
	BMessage window_decor;
	window_decor.AddString("window:decor", "R5");
	window_decor.AddInt32("window:R5:decor", 0L);
	theme.AddMessage(A_MSGNAME, &window_decor);
	return ApplyTheme(theme, flags);
}


ThemesAddon *instanciate_themes_addon()
{
	return (ThemesAddon *) new DecorThemesAddon;
}

#endif /* B_BEOS_VERSION_DANO */
