/*
 * Pulse colors ThemesAddon class
 */

#include <Alert.h>
#include <Application.h>
#include <Directory.h>
#include <Message.h>
#include <Messenger.h>
#include <Roster.h>

#include <string.h>

#include "ThemesAddon.h"
#include "UITheme.h"

#ifdef SINGLE_BINARY
#define instantiate_themes_addon instantiate_themes_addon_pulse
#endif

#define A_NAME "Pulse"
#define A_MSGNAME Z_THEME_PULSE_SETTINGS
#define A_DESCRIPTION "Pulse colors"

class PulseThemesAddon : public ThemesAddon {
public:
	PulseThemesAddon();
	~PulseThemesAddon();
	
const char *Description();

status_t	RunPreferencesPanel();

status_t	AddNames(BMessage &names);

status_t	ApplyTheme(BMessage &theme, uint32 flags=0L);
status_t	MakeTheme(BMessage &theme, uint32 flags=0L);

status_t	ApplyDefaultTheme(uint32 flags=0L);
};

PulseThemesAddon::PulseThemesAddon()
	: ThemesAddon(A_NAME, A_MSGNAME)
{
}

PulseThemesAddon::~PulseThemesAddon()
{
}

const char *PulseThemesAddon::Description()
{
	return A_DESCRIPTION;
}

status_t	PulseThemesAddon::RunPreferencesPanel()
{
	status_t err;
	entry_ref ref;
	BEntry ent;
	err = ent.SetTo("/boot/beos/preferences/Pulse");
	if (!err) {
		err = ent.GetRef(&ref);
		if (!err) {
			err = be_roster->Launch(&ref);
		}
	}
	if (!err)
		return B_OK;
	err = ent.SetTo("/system/add-ons/Preferences/Pulse");
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

status_t PulseThemesAddon::AddNames(BMessage &names)
{
	names.AddString(Z_THEME_PULSE_SETTINGS, "Pulse colors");
	//names.AddString("db:location", "Pulse on-screen position");
	//names.AddString("db:expanded", "Pulse is expanded");
	return B_OK;
}

status_t PulseThemesAddon::ApplyTheme(BMessage &theme, uint32 flags)
{
	BMessage deskbar;
	status_t err;
	int32 loc = 5;
	bool expanded = true;
	BPulse db;
	
	if (!(flags & UI_THEME_SETTINGS_SET_ALL) || !(AddonFlags() & Z_THEME_ADDON_DO_SET_ALL))
		return B_OK;
	
	err = MyMessage(theme, deskbar);
	if (err)
		return err;
	
	if (deskbar.FindInt32("db:location", &loc) != B_OK)
		return ENOENT;
	deskbar.FindBool("db:expanded", &expanded);
	return db.SetLocation((deskbar_location)loc, expanded);
}

status_t PulseThemesAddon::MakeTheme(BMessage &theme, uint32 flags)
{
	BMessage deskbar;
	status_t err;
	
	(void)flags;
	err = MyMessage(theme, deskbar);
	if (err)
		deskbar.MakeEmpty();
	
	deskbar_location loc;
	bool expanded;
	BPulse db;

	loc = db.Location(&expanded);
	deskbar.AddInt32("db:location", (int32)loc);
	deskbar.AddBool("db:expanded", expanded);
	
	err = SetMyMessage(theme, deskbar);
	return err;
}

status_t PulseThemesAddon::ApplyDefaultTheme(uint32 flags)
{
	BMessage theme;
	BMessage deskbar;
	deskbar_location loc = B_DESKBAR_RIGHT_TOP;
	bool expanded = true;
	deskbar.AddInt32("db:location", (int32)loc);
	deskbar.AddBool("db:expanded", expanded);
	theme.AddMessage(A_MSGNAME, &deskbar);
	return ApplyTheme(theme, flags);
}


ThemesAddon *instantiate_themes_addon()
{
	return (ThemesAddon *) new PulseThemesAddon;
}
