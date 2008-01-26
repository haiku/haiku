/*
 * WinampSkin ThemesAddon class
 */

#include <Directory.h>
#include <Message.h>
#include <Messenger.h>
#include <Font.h>
#include <List.h>
#include <String.h>
#include <Roster.h>
#include <storage/Path.h>
#include <storage/File.h>
#include <storage/NodeInfo.h>
#include <storage/FindDirectory.h>
#include <Debug.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "ThemesAddon.h"
#include "UITheme.h"

#ifdef SINGLE_BINARY
#define instantiate_themes_addon instantiate_themes_addon_winamp_skin
#endif

#define A_NAME "Winamp Skin"
#define A_MSGNAME Z_THEME_WINAMP_SKIN_SETTINGS
#define A_DESCRIPTION "Handle CL-Amp and SoundPlay (not yet) Skins - requires SkinSelector plugin to be activated in CL-Amp and SoundPlay."

#define CL_SETTINGS_NAME "CL-Amp/CL-Amp.preferences"
#define CL_APP_SIG "application/x-vnd.ClaesL-CLAmp"
#define SP_SETTINGS_NAME "PrefServer/pref0:$USER:application:x-vnd.marcone-soundplay"
//BMessage('save') {
//skinpath = string("/wincrash/Program Files/Winamp/Skins", 37 bytes)
//skinname = string("/wincrash/Program Files/Winamp/Skins/XP_Amp_2.wsz", 50 bytes) }
#define SP_APP_SIG "application/x-vnd.marcone-soundplay"
#define USE_CL 1
#define USE_SP 2


class WinampSkinThemesAddon : public ThemesAddon {
public:
	WinampSkinThemesAddon();
	~WinampSkinThemesAddon();
	
const char *Description();

status_t	RunPreferencesPanel();

status_t	AddNames(BMessage &names);

status_t	ApplyTheme(BMessage &theme, uint32 flags=0L);
status_t	MakeTheme(BMessage &theme, uint32 flags=0L);

status_t	ApplyDefaultTheme(uint32 flags=0L);

int32		WhichApp(); /* which app should I use to repport current skin? */
status_t	CLSkin(BString *to, bool preffile = false);
status_t	SPSkin(BString *to, bool preffile = false);
status_t	CLSkinPath(BPath *to);
status_t	SPSkinPath(BPath *to);
status_t	SetCLSkin(BString *from);
status_t	SetSPSkin(BString *from);
status_t	StripPath(BString *path);
};

WinampSkinThemesAddon::WinampSkinThemesAddon()
	: ThemesAddon(A_NAME, A_MSGNAME)
{
}

WinampSkinThemesAddon::~WinampSkinThemesAddon()
{
}

const char *WinampSkinThemesAddon::Description()
{
	return A_DESCRIPTION;
}

status_t	WinampSkinThemesAddon::RunPreferencesPanel()
{
	return B_OK;
}

status_t WinampSkinThemesAddon::AddNames(BMessage &names)
{
	names.AddString(Z_THEME_WINAMP_SKIN_SETTINGS, "CLAmp/SoundPlay Skin Settings");
	return B_OK;
}

status_t WinampSkinThemesAddon::ApplyTheme(BMessage &theme, uint32 flags)
{
	BMessage skin;
	BString name;
	status_t err;
	
	if (!(flags & UI_THEME_SETTINGS_SET_ALL) || !(AddonFlags() & Z_THEME_ADDON_DO_SET_ALL))
		return B_OK;
	
	err = MyMessage(theme, skin);
	if (err)
		return err;
	
	err = skin.FindString("winamp:skin", &name);
	if (err)
		return err;
	SetCLSkin(&name);
	SetSPSkin(&name);
	return B_OK;
}

status_t WinampSkinThemesAddon::MakeTheme(BMessage &theme, uint32 flags)
{
	BMessage skin;
	BString name;
	status_t err = B_ERROR;
	
	(void)flags;
	err = MyMessage(theme, skin);
	if (err)
		skin.MakeEmpty();

	if (WhichApp() == USE_CL)
		err = CLSkin(&name);
	else
		err = SPSkin(&name);
	/* try the other way round */
	if (err)
		if (WhichApp() == USE_CL)
			err = CLSkin(&name, true);
		else
			err = SPSkin(&name, true);
	if (!err)
		skin.AddString("winamp:skin", name);
	err = SetMyMessage(theme, skin);
	return err;
}

status_t WinampSkinThemesAddon::ApplyDefaultTheme(uint32 flags)
{
	BMessage theme;
	BMessage skin;
	skin.AddString("skin", "none");
	theme.AddMessage(Z_THEME_WINAMP_SKIN_SETTINGS, &skin);
	return ApplyTheme(theme, flags);
}

int32 WinampSkinThemesAddon::WhichApp()
{
	// XXX
/*
	BMessenger spmsgr(SP_APP_SIG);
	if (spmsgr.IsValid()) {
		PRINT(("WinampSkinThemesAddon: SoundPlay BMessenger valid\n"));
		return USE_SP;
	}*/
	BMessenger clmsgr(CL_APP_SIG);
	if (clmsgr.IsValid()) {
		PRINT(("WinampSkinThemesAddon: CL-Amp BMessenger valid\n"));
		return USE_CL;
	}
	return USE_SP;
}

status_t WinampSkinThemesAddon::CLSkin(BString *to, bool preffile)
{
	if (preffile) {
		BPath CLSPath;
		char buffer[B_FILE_NAME_LENGTH+1];
		
		if (!to)
			return EINVAL;
		buffer[B_FILE_NAME_LENGTH] = '\0';
		
		if (find_directory(B_USER_SETTINGS_DIRECTORY, &CLSPath) < B_OK)
			return B_ERROR;
		CLSPath.Append(CL_SETTINGS_NAME);
		BFile CLSettings(CLSPath.Path(), B_READ_ONLY);
		if (CLSettings.InitCheck() < B_OK)
			return CLSettings.InitCheck();
		ssize_t got = CLSettings.ReadAt(0x8LL, buffer, B_FILE_NAME_LENGTH);
		if (got < B_FILE_NAME_LENGTH)
			return EIO;
		*to = buffer;
		StripPath(to);
		return B_ERROR;
	}
	BMessenger msgr(CL_APP_SIG);
	BMessage msg('skin');
	BMessage reply;
	msgr.SendMessage(&msg, &reply, 500000, 500000);
	if (reply.FindString("result", to) >= B_OK) {
		StripPath(to);
		return B_OK;
	}
	return B_ERROR;
}

status_t WinampSkinThemesAddon::SPSkin(BString *to, bool preffile)
{
	if (preffile) {
		status_t err;
		BMessage settings;
		BPath SPSPath;
		
		if (find_directory(B_USER_SETTINGS_DIRECTORY, &SPSPath) < B_OK)
			return B_ERROR;
		BString leaf(SP_SETTINGS_NAME);
		BString user(getenv("USER"));
		user.RemoveFirst("USER=");
		leaf.IReplaceFirst("$USER", user.String());
		SPSPath.Append(leaf.String());
		PRINT(("SPP %s\n", SPSPath.Path()));
		BFile SPSettings(SPSPath.Path(), B_READ_ONLY);
		if (SPSettings.InitCheck() < B_OK)
			return SPSettings.InitCheck();
		if (settings.Unflatten(&SPSettings) < B_OK)
			return EIO;
		err = settings.FindString("skinname", to);
		StripPath(to);
		return err;
	}
	BMessenger msgr(SP_APP_SIG);
	BMessage msg('skin');
	BMessage reply;
	msgr.SendMessage(&msg, (BHandler *)NULL, 500000);
	if (reply.FindString("result", to) >= B_OK) {
		StripPath(to);
		return B_OK;
	}
	return B_ERROR;
}

status_t WinampSkinThemesAddon::CLSkinPath(BPath *to)
{
	char buffer[B_FILE_NAME_LENGTH+1];
	BPath CLSPath;

	buffer[B_FILE_NAME_LENGTH] = '\0';
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &CLSPath) < B_OK)
		return B_ERROR;
	CLSPath.Append(CL_SETTINGS_NAME);
	BFile CLSettings(CLSPath.Path(), B_READ_ONLY);
	if (CLSettings.InitCheck() < B_OK)
		return CLSettings.InitCheck();
	ssize_t got = CLSettings.ReadAt(0x150LL, buffer, B_FILE_NAME_LENGTH);
	if (got < B_FILE_NAME_LENGTH)
		return EIO;
	to->SetTo(buffer);
	return B_OK;
}

status_t WinampSkinThemesAddon::SPSkinPath(BPath *to)
{
	status_t err;
	BMessage settings;
	BPath SPSPath;
	BString str;

	if (find_directory(B_USER_SETTINGS_DIRECTORY, &SPSPath) < B_OK)
		return B_ERROR;
	BString leaf(SP_SETTINGS_NAME);
	BString user(getenv("USER"));
	user.RemoveFirst("USER=");
	leaf.IReplaceFirst("$USER", user.String());
	SPSPath.Append(leaf.String());
	BFile SPSettings(SPSPath.Path(), B_READ_ONLY);
	if (SPSettings.InitCheck() < B_OK)
		return SPSettings.InitCheck();
	if (settings.Unflatten(&SPSettings) < B_OK)
		return EIO;
	err = settings.FindString("skinpath", &str);
	PRINT(("SPSP %s\n", str.String()));
	to->SetTo(str.String());
	return err;
}

status_t WinampSkinThemesAddon::SetCLSkin(BString *from)
{
	status_t err;
	BPath p;
	err = CLSkinPath(&p);
	if (err < B_OK)
		return err;
	p.Append(from->String());
	/* update the prefs file */
	BPath CLSPath;
	
	if (!from)
		return EINVAL;
	/* CL-Amp doesn't grok long names */
	if (strlen(p.Path()) >= B_FILE_NAME_LENGTH)
		return B_NAME_TOO_LONG;
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &CLSPath) < B_OK)
		return B_ERROR;
	CLSPath.Append(CL_SETTINGS_NAME);
	BFile CLSettings(CLSPath.Path(), B_WRITE_ONLY);
	if (CLSettings.InitCheck() < B_OK)
		return CLSettings.InitCheck();
	CLSettings.WriteAt(0x8LL, p.Path(), strlen(p.Path())+1);
	
	/* then tell the app */
	BMessenger msgr(CL_APP_SIG);
	BMessage msg('skin');
	msg.AddString("name", p.Path());
	msg.PrintToStream();
	msgr.SendMessage(&msg, (BHandler *)NULL, 500000);
	return B_OK;
}

status_t WinampSkinThemesAddon::SetSPSkin(BString *from)
{
	status_t err;
	BPath p;
	err = SPSkinPath(&p);
	if (err < B_OK)
		return err;
	p.Append(from->String());
	err = p.InitCheck();
	if (err < B_OK)
		return err;

	/* update the prefs file */
	BPath SPSPath;
	BMessage settings;
	
	if (!from)
		return EINVAL;
	if (from->Length() >= B_PATH_NAME_LENGTH)
		return B_NAME_TOO_LONG;
	
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &SPSPath) < B_OK)
		return B_ERROR;
	BString leaf(SP_SETTINGS_NAME);
	BString user(getenv("USER"));
	user.RemoveFirst("USER=");
	leaf.IReplaceFirst("$USER", user.String());
	SPSPath.Append(leaf.String());
	PRINT(("SPP %s\n", SPSPath.Path()));
	BFile SPSettings(SPSPath.Path(), B_READ_WRITE);
	PRINT(("SP:BFile\n"));
	if (SPSettings.InitCheck() < B_OK)
		return SPSettings.InitCheck();
	PRINT(("SP:BFile::InitCheck\n"));
	//SPSettings.WriteAt(0x8LL, from->String(), from->Length()+1);
	if (settings.Unflatten(&SPSettings) < B_OK)
		return EIO;
	PRINT(("SP:Unflatten\n"));
	settings.ReplaceString("skinname", p.Path());
	PRINT(("SP:Replace\n"));
	SPSettings.Seek(0LL, SEEK_SET);
	if (settings.Flatten(&SPSettings) < B_OK)
		return EIO;
	PRINT(("SP:Flatten\n"));
	
	/* then tell the app */
	/* first make sure the native BeOS is shown (the 'sk!n' message toggles, wonder Why TF...) */
	bool beosIfaceHidden;
	BMessenger msgr(SP_APP_SIG);
	
	BMessage command(B_GET_PROPERTY);
	BMessage answer;
	command.AddSpecifier("Hidden");
	command.AddSpecifier("Window", 0L); // TitleSpectrumAnalyzer does change the title from "SoundPlay"!
	err = msgr.SendMessage(&command, &answer,(bigtime_t)2E6,(bigtime_t)2E6);
	if(B_OK == err) {
		if (answer.FindBool("result", &beosIfaceHidden) != B_OK)
			beosIfaceHidden = false;
	} else {
		PRINT(("WinampSkinThemesAddon: couldn't talk to SoundPlay: error 0x%08lx\n", err));
		return EIO;
	}	

	BMessage msg('sk!n');
	msg.AddString("path", p.Path());
	msg.PrintToStream();
	if (!beosIfaceHidden) { // native GUI is active
		// -> sending the message to the App will make ituse the skin for all tracks
		PRINT(("SP:Sending to app\n"));
		msgr.SendMessage(&msg, (BHandler *)NULL, 500000);
	} else { // native GUI is hidden (= winamp GUI used)
		int32 wincnt;
		command.MakeEmpty();
		command.what = B_COUNT_PROPERTIES;
		answer.MakeEmpty();
		command.AddSpecifier("Window");
		err = msgr.SendMessage(&command, &answer,(bigtime_t)2E6,(bigtime_t)2E6);
		if(B_OK == err) {
			if (answer.FindInt32("result", &wincnt) != B_OK)
				wincnt = 1;
		} else {
			PRINT(("WinampSkinThemesAddon: couldn't talk to SoundPlay: (2) error 0x%08lx\n", err));
			return EIO;
		}
		// send to all windows but first one.
		for (int32 i = 1; i < wincnt; i++) {
			BMessage wmsg(msg);
			PRINT(("SP:Sending to window %ld\n", i));
			wmsg.AddSpecifier("Window", i);
			wmsg.PrintToStream();
			msgr.SendMessage(&wmsg, (BHandler *)NULL, 500000);
		}
	}
	return B_OK;
}

status_t WinampSkinThemesAddon::StripPath(BString *path)
{
	if (!path)
		return EINVAL;
	BPath p(path->String());
	*path = p.Leaf();
	return B_OK;
}



ThemesAddon *instantiate_themes_addon()
{
	return (ThemesAddon *) new WinampSkinThemesAddon;
}
