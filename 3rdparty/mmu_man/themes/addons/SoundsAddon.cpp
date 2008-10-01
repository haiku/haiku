/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
 * sounds ThemesAddon class
 */

#include <Alert.h>
#include <Application.h>
#include <Directory.h>
#include <Entry.h>
#include <MediaFiles.h>
#include <Message.h>
#include <Messenger.h>
#include <Path.h>
#include <Roster.h>

#include <stdio.h>
#include <string.h>

#include "ThemesAddon.h"
#include "UITheme.h"
#include "Utils.h"

#ifdef SINGLE_BINARY
#define instantiate_themes_addon instantiate_themes_addon_sounds
#endif

#define A_NAME "Sounds"
#define A_MSGNAME Z_THEME_SOUNDS_SETTINGS
#define A_DESCRIPTION "System sound events"


class SoundsThemesAddon : public ThemesAddon {
public:
	SoundsThemesAddon();
	~SoundsThemesAddon();
	
const char *Description();

status_t	RunPreferencesPanel();

status_t	AddNames(BMessage &names);

status_t	ApplyTheme(BMessage &theme, uint32 flags=0L);
status_t	MakeTheme(BMessage &theme, uint32 flags=0L);

status_t	ApplyDefaultTheme(uint32 flags=0L);
};


SoundsThemesAddon::SoundsThemesAddon()
	: ThemesAddon(A_NAME, A_MSGNAME)
{
}


SoundsThemesAddon::~SoundsThemesAddon()
{
}


const char *
SoundsThemesAddon::Description()
{
	return A_DESCRIPTION;
}


status_t
SoundsThemesAddon::RunPreferencesPanel()
{
	status_t err;
	entry_ref ref;
	BEntry ent;
	err = ent.SetTo("/boot/beos/preferences/Sounds");
	if (!err) {
		err = ent.GetRef(&ref);
		if (!err) {
			err = be_roster->Launch(&ref);
		}
	}
	if (!err)
		return B_OK;
	err = ent.SetTo("/system/add-ons/Preferences/Sounds");
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


status_t
SoundsThemesAddon::AddNames(BMessage &names)
{
	names.AddString(Z_THEME_SOUNDS_SETTINGS, "Sounds Settings");
	names.AddString("sounds:file", "Filename for this sound event");
	names.AddString("sounds:volume", "Volume for this sound event");
	return B_OK;
}


status_t
SoundsThemesAddon::ApplyTheme(BMessage &theme, uint32 flags)
{
	BMessage sounds;
	status_t err;
	BMediaFiles bmfs;
	BString item;
	entry_ref entry;
	BEntry ent;
	BPath path;
	const char *p;
	float gain;
	int32 index;
	char *field_name;
	type_code field_code;
	int32 field_count;
	BMessage msg;

	if (!(flags & UI_THEME_SETTINGS_SET_ALL) || !(AddonFlags() & Z_THEME_ADDON_DO_SET_ALL))
		return B_OK;
	
	err = MyMessage(theme, sounds);
	if (err)
		return err;

	bmfs.RewindRefs(BMediaFiles::B_SOUNDS);
	for (index = 0; sounds.GetInfo(B_ANY_TYPE, index,  
							GET_INFO_NAME_PTR(&field_name), 
							&field_code, 
							&field_count) == B_OK; index++) {
		if (field_code != B_MESSAGE_TYPE)
			continue;
		if (sounds.FindMessage(field_name, &msg) < B_OK)
			continue;
		/* remove old ref if any */
		if ((bmfs.GetRefFor(BMediaFiles::B_SOUNDS, field_name, &entry) >= B_OK)
				 && (entry.device >= 0))
			bmfs.RemoveRefFor(BMediaFiles::B_SOUNDS, field_name, entry);
		if (msg.FindString("sounds:file", &p) < B_OK)
			continue;
		path.SetTo(p);
		if (ent.SetTo(path.Path()) < B_OK)
			continue;
		if (ent.GetRef(&entry) < B_OK)
			continue;
		if (bmfs.SetRefFor(BMediaFiles::B_SOUNDS, field_name, entry) < B_OK)
			continue;
		if (msg.FindFloat("sounds:volume", &gain) < B_OK)
			continue;
#if defined(__HAIKU__) || defined(B_BEOS_VERSION_DANO)
		if (bmfs.SetAudioGainFor(BMediaFiles::B_SOUNDS, field_name, gain) < B_OK)
			continue;
#endif
	}

	return B_OK;
}


status_t
SoundsThemesAddon::MakeTheme(BMessage &theme, uint32 flags)
{
	BMessage sounds;
	status_t err;
	BMediaFiles bmfs;
	BString item;
	entry_ref entry;
	BEntry ent;
	BPath path;
	float gain;
	
	(void)flags;
	err = MyMessage(theme, sounds);
	if (err)
		sounds.MakeEmpty();
	
	bmfs.RewindRefs(BMediaFiles::B_SOUNDS);
	while (bmfs.GetNextRef(&item, &entry) == B_OK) {
		BMessage msg('SndI');
		path.Unset();
		ent.SetTo(&entry);
		ent.GetPath(&path);
		//printf("\t%s: %s\n", item.String(), path.Path());
		if (path.Path()) {
			msg.AddString("sounds:file", path.Path());
			gain = 1.0;
#if defined(__HAIKU__) || defined(B_BEOS_VERSION_DANO)
			bmfs.GetAudioGainFor(BMediaFiles::B_SOUNDS, item.String(), &gain);
#endif
			msg.AddFloat("sounds:volume", gain);
		}
		sounds.AddMessage(item.String(), &msg);
	}
	
	err = SetMyMessage(theme, sounds);
	return err;
}


status_t
SoundsThemesAddon::ApplyDefaultTheme(uint32 flags)
{
	BMessage theme;
	BMessage sounds;
	(void)flags;
	//theme.AddMessage(A_MSGNAME, &deskbar);
	//return ApplyTheme(theme, flags);
	// TODO
	return B_OK;
}


ThemesAddon *
instantiate_themes_addon()
{
	return (ThemesAddon *) new SoundsThemesAddon;
}

