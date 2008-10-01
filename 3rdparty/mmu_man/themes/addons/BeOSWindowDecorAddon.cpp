/*
 * Copyright 2000-2008, François Revol, <revol@free.fr>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
 * window_decor ThemesAddon class
 */

#include <BeBuild.h>
#if defined(B_BEOS_VERSION) && !defined(__HAIKU__)

#include <Alert.h>
#include <Application.h>
#include <Debug.h>
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

// for reference:
//
// available decors in Dano:
// (Default R5) Baqua BlueSteel Graphite Origin OriginSimple
//
// available decors in Zeta:
// (Default R5) Cupertino MenloTab Redmond Smoke ZetaDecor Menlo 
// R5 Seattle TV_ ZLight blueBeOS yellowTAB
//
// mine:
// Win2k TextMode TextModeBlue


#ifdef SINGLE_BINARY
#define instantiate_themes_addon instantiate_themes_addon_window_decor
#endif

#define A_NAME "Window Decor"
#define A_MSGNAME Z_THEME_WINDOW_DECORATIONS
#define A_DESCRIPTION "Window decorations and scrollbars"


#ifdef B_BEOS_VERSION_DANO
status_t
set_window_decor_safe(const char *name, BMessage *globals)
{
	// make sure the decor exists (set_window_decor() always returns B_OK)
	//PRINT(("set_window_decor_safe(%s, %p)\n", name, globals));
	BPath p("/etc/decors/");
	p.Append(name);
	BNode n(p.Path());
	if (n.InitCheck() < B_OK || !n.IsFile())
		return ENOENT;
	//PRINT(("set_window_decor_safe: found\n"));
	set_window_decor(name, globals);
	return B_OK;
}
#endif


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


const char *
DecorThemesAddon::Description()
{
	return A_DESCRIPTION;
}


status_t
DecorThemesAddon::RunPreferencesPanel()
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


status_t
DecorThemesAddon::AddNames(BMessage &names)
{
	names.AddString(Z_THEME_WINDOW_DECORATIONS, "Window decorations and scrollbars");
	names.AddString("window:decor", "Window decor");
	names.AddString("window:decor_globals", "Window decor parameters");
	return B_OK;
}


status_t
DecorThemesAddon::ApplyTheme(BMessage &theme, uint32 flags)
{
	BMessage window_decor;
	BMessage globals;
	BString decorName;
	int32 decorId = R5_DECOR_BEOS;
	status_t err;

	if (!(flags & UI_THEME_SETTINGS_SET_ALL) || !(AddonFlags() & Z_THEME_ADDON_DO_SET_ALL))
		return B_OK;
	
	err = MyMessage(theme, window_decor);
	if (err)
		return err;
	
#ifdef B_BEOS_VERSION_DANO
	bool decorDone = false;
	int i;
	// try each name until one works
	for (i = 0; window_decor.FindString("window:decor", i, &decorName) == B_OK; i++) {
		err = set_window_decor_safe(decorName.String(), 
			(window_decor.FindMessage("window:decor_globals", &globals) == B_OK)?&globals:NULL);
		if (err < B_OK)
			continue;
		decorDone = true;
		break;
	}
	// none match but we did have one, force the default with the globals
	if (!decorDone && i > 0) {
		decorDone = true;
		set_window_decor("Default", 
		(window_decor.FindMessage("window:decor_globals", &globals) == B_OK)?&globals:NULL);
	}
	// none... maybe R5 number ?
	if (!decorDone && 
		window_decor.FindInt32("window:R5:decor", &decorId) == B_OK) {
		switch (decorId) {
			case R5_DECOR_BEOS:
			default:
				err = set_window_decor_safe("R5", NULL);
				if (err >= B_OK)
					break;
				err = set_window_decor_safe("BeOS", NULL);
				if (err >= B_OK)
					break;
				set_window_decor("R5", NULL);
				break;
			case R5_DECOR_WIN95:
				err = set_window_decor_safe("Win2k", NULL);
				if (err >= B_OK)
					break;
				err = set_window_decor_safe("Redmond", NULL);
				if (err >= B_OK)
					break;
				err = set_window_decor_safe("Seattle", NULL);
				if (err >= B_OK)
					break;
				set_window_decor("R5", NULL);
				break;
			case R5_DECOR_AMIGA:
				err = set_window_decor_safe("Amiga", NULL);
				if (err >= B_OK)
					break;
				err = set_window_decor_safe("AmigaOS", NULL);
				if (err >= B_OK)
					break;
				err = set_window_decor_safe("AmigaOS4", NULL);
				if (err >= B_OK)
					break;
				set_window_decor("R5", NULL);
				break;
			case R5_DECOR_MAC:
				err = set_window_decor_safe("Mac", NULL);
				if (err >= B_OK)
					break;
				err = set_window_decor_safe("MacOS", NULL);
				if (err >= B_OK)
					break;
				err = set_window_decor_safe("Baqua", NULL);
				if (err >= B_OK)
					break;
				err = set_window_decor_safe("Cupertino", NULL);
				if (err >= B_OK)
					break;
				set_window_decor("R5", NULL);
				break;
		}
		decorDone = true;
	}
	// it might be intentionally there is none...
	//if (!decorDone)
	//	set_window_decor("Default", NULL);
#else
	// best effort with what we have
extern void __set_window_decor(int32 theme);
	if (window_decor.FindInt32("window:R5:decor", &decorId) == B_OK)
		__set_window_decor(decorId);
	else {
		BString name;
		for (int i = 0; window_decor.FindString("window:decor", i, &decorName) == B_OK; i++) {
			if (decorName.IFindFirst("R5") > -1 || 
				decorName.IFindFirst("BeOS") > -1 || 
				decorName.IFindFirst("Be") > -1 || 
				decorName.IFindFirst("yellow") > -1 || 
				decorName.IFindFirst("Menlo") > -1 || 
				decorName.IFindFirst("Default") > -1 || 
				decorName.IFindFirst("Origin") > -1) {
				__set_window_decor(R5_DECOR_BEOS);
				break;
			}
			if (decorName.IFindFirst("Microsoft") > -1 || 
				decorName.IFindFirst("2k") > -1 || 
				decorName.IFindFirst("XP") > -1 || 
				decorName.IFindFirst("Redmond") > -1 || 
				decorName.IFindFirst("Seattle") > -1 || 
				decorName.IFindFirst("Win") > -1) {
				__set_window_decor(R5_DECOR_WIN95);
				break;
			}
			if (decorName.IFindFirst("Amiga") > -1 || 
				decorName.IFindFirst("OS4") > -1) {
				__set_window_decor(R5_DECOR_AMIGA);
				break;
			}
			if (decorName.IFindFirst("Mac") > -1 || 
				decorName.IFindFirst("Apple") > -1 || 
				decorName.IFindFirst("Aqua") > -1 || 
				decorName.IFindFirst("Cupertino") > -1 || 
				decorName.IFindFirst("OSX") > -1) {
				__set_window_decor(R5_DECOR_MAC);
				break;
			}
		}
	}
#endif
	// XXX: add colors à la WindowShade ?
	
	return B_OK;
}


status_t
DecorThemesAddon::MakeTheme(BMessage &theme, uint32 flags)
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


status_t
DecorThemesAddon::ApplyDefaultTheme(uint32 flags)
{
	BMessage theme;
	BMessage window_decor;
	window_decor.AddString("window:decor", "R5");
	window_decor.AddInt32("window:R5:decor", 0L);
	theme.AddMessage(A_MSGNAME, &window_decor);
	return ApplyTheme(theme, flags);
}


ThemesAddon *
instantiate_themes_addon()
{
	return (ThemesAddon *) new DecorThemesAddon;
}


#endif /* B_BEOS_VERSION && !__HAIKU__ */
